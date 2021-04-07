//NAME: Rohan Rao,Ethan Ngo
//EMAIL: raokrohan@gmail.com,ethan.ngo2019@gmail.com
//ID: 305339928,205416130

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//for checking inode mode
#include <sys/stat.h>

//for time translation
#include <time.h>

//incudes for open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//includes for pread
#include <unistd.h>

//for errno
#include <errno.h>

//for ext2 structs
#include "ext2_fs.h"

//So we don't have to type struct every single time
typedef struct ext2_super_block ext2_super_block;
typedef struct ext2_group_desc ext2_group_desc;
typedef struct ext2_inode ext2_inode;

/*
    GLOBAL VARS GO HERE:
*/
unsigned int block_size;

unsigned int block_offset(unsigned int block)
{
    return 1024 + (block_size * (block - 1));
}

int is_block_used(int bno, char * bitmap)
{
    int index = 0, offset = 0;
    if (bno == 0)
        return 1;
    index = (bno - 1)/8; //which byte within the bitmap stores the info of this block#
    offset = (bno - 1)%8;
    return ((bitmap[index] & (1 << offset)) );
}

void epoch_to_gmt(char buf[], int bufsize, unsigned int t)
{
    time_t xtime = t;
    struct tm x_ts; 
    if(gmtime_r(&xtime, &x_ts) == NULL)
    {
        fprintf(stderr, "Error converting time...%s",strerror(errno));
        exit(2);
    }
    strftime(buf, bufsize, "%D %T", &x_ts);
}

int main(int argc, char* argv[])
{

    if(argc > 2 || argc == 1)
    {
        fprintf(stderr, "Incorrect arguments provided\n");
        fprintf(stderr, "Usage: %s FileSystemImageName.img \n", argv[0]);
        exit(1);
    }

    int fd = 0;
    if((fd = open(argv[1], O_RDONLY)) == -1)
    {
        fprintf(stderr, "Failed to open '%s'\n%s",argv[1], strerror(errno));
        exit(1);
    }

    //SUPERBLOCK DATA-------------
    unsigned int blocks_count = 0, inodes_count = 0, inode_size = 0, 
                blocks_per_group = 0, inodes_per_group = 0, first_non_res_inode = 0;
    block_size = 0;

    ext2_super_block super;

    if(pread(fd, &super, sizeof(super), 1024) == -1)
    {
        fprintf(stderr, "Problem with reading from fd...\n%s", strerror(errno));
        exit(1);
    }

    if(super.s_magic !=  EXT2_SUPER_MAGIC) 
    {
        fprintf(stderr, "Magic num != EXT2_SUPER_MAGIC...\n\
        File system may be corrupted or an invalid File System was given\n");
        exit(1);
    }

    blocks_count = super.s_blocks_count;
    inodes_count = super.s_inodes_count;
    block_size = 1024 << super.s_log_block_size; /* calculate block size in bytes */
    inode_size = super.s_inode_size;
    blocks_per_group = super.s_blocks_per_group;
    inodes_per_group = super.s_inodes_per_group;
    first_non_res_inode = super.s_first_ino;
    //END SUPERBLOCK DATA-------------


    //GROUP DESCRIPTORS DATA-------------
    unsigned int num_groups = blocks_count / blocks_per_group;
    if(!num_groups)
        num_groups = 1;

    ext2_group_desc* group_desc_table = 
    (ext2_group_desc*)malloc(num_groups*sizeof(ext2_group_desc));

    pread(fd, group_desc_table, num_groups*sizeof(ext2_group_desc), 1024 + block_size);
    //END GROUP DESCRIPTORS DATA-------------

    /*
    1.SUPERBLOCK *
    2.total number of blocks (decimal) *
    3.total number of i-nodes (decimal) *
    4.block size (in bytes, decimal) *
    5.i-node size (in bytes, decimal)
    6.blocks per group (decimal)
    7.i-nodes per group (decimal)
    8.first non-reserved i-node (decimal)
    */
    printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", blocks_count,inodes_count,block_size,
                                                inode_size,blocks_per_group,
                                                inodes_per_group,first_non_res_inode);
    /*
    1.GROUP
    2.group number (decimal, starting from zero)
    3.total number of blocks in this group (decimal)
    4.total number of i-nodes in this group (decimal)
    5.number of free blocks (decimal)
    6.number of free i-nodes (decimal)
    7.block number of free block bitmap for this group (decimal)
    8.block number of free i-node bitmap for this group (decimal)
    9.block number of first block of i-nodes in this group (decimal)
    */
   unsigned int temp_blocks_per_group = 0;
   unsigned int temp_inodes_per_group = 0;
   for(unsigned int i = 0; i < num_groups; i++)
   {
        //account for lesser blocks in last group
        if(i == num_groups - 1)
           temp_blocks_per_group = blocks_count - blocks_per_group*(num_groups-1);
        //account for lesser inodes in last group
        if(i == num_groups - 1)
           temp_inodes_per_group = inodes_count - inodes_per_group*(num_groups-1);
           
        printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", i, temp_blocks_per_group,temp_inodes_per_group,
                                            group_desc_table[i].bg_free_blocks_count,
                                            group_desc_table[i].bg_free_inodes_count,
                                            group_desc_table[i].bg_block_bitmap,
                                            group_desc_table[i].bg_inode_bitmap,
                                            group_desc_table[i].bg_inode_table);

        //BLOCK BITMAP------------------
        //1 = used
        //0 = free
        unsigned int block_bitmap;
        block_bitmap = group_desc_table[i].bg_block_bitmap;
        char* block_bits = (char*) malloc(block_size);
        
        pread(fd, block_bits, block_size, block_offset(block_bitmap));
        
        for(unsigned int j = 0; j < temp_blocks_per_group; j++)
            if(!is_block_used(j, block_bits))
                printf("BFREE,%d\n", j);
        //END BLOCK BITMAP---------------

        //INODE BITMAP------------------
        //1 = used
        //0 = free
        unsigned int inode_bitmap;
        inode_bitmap = group_desc_table[i].bg_inode_bitmap;
        char* inode_bits = (char*) malloc(block_size);
        pread(fd, inode_bits, block_size, block_offset(inode_bitmap));
        
        //made <= because first inode is reserved...
        //still need to iterate temp_inode_per_group times
        for(unsigned int j = 1; j <= temp_inodes_per_group; j++)
            if(!is_block_used(j, inode_bits))
                printf("IFREE,%d\n", j);
        //END INODE BITMAP---------------


        //INODE SUMMARY---------------
        /*
            1.INODE
            2.inode number (decimal)
            3.file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
            4.mode (low order 12-bits, octal ... suggested format "%o")
            5.owner (decimal)
            6.group (decimal)
            7.link count (decimal)
            8.time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
            9.modification time (mm/dd/yy hh:mm:ss, GMT)
            10/time of last access (mm/dd/yy hh:mm:ss, GMT)
            11.file size (decimal)
            12.number of (512 byte) blocks of disk space (decimal) taken up by this file
        */
        ext2_inode *inode_table = 
        (ext2_inode*)malloc(temp_inodes_per_group*sizeof(ext2_inode));
        pread(fd, inode_table, temp_inodes_per_group*sizeof(ext2_inode), block_offset(group_desc_table[i].bg_inode_table));

        for(unsigned int j = 1; j <= temp_inodes_per_group; j++)
            if(is_block_used(j, inode_bits))
            {
                ext2_inode cur_inode = inode_table[j];
                //only process if node and link count are non-zero
                if(cur_inode.i_mode == 0 || cur_inode.i_links_count == 0)
                    continue;
                //get filetype
                char filetype = '?';
                if(S_ISREG(cur_inode.i_mode))
                    filetype = 'f';
                if(S_ISDIR(cur_inode.i_mode))
                    filetype = 'd';
                if(S_ISLNK(cur_inode.i_mode))
                    filetype = 's';
                //get 12 lower bits in octal
                int octmask = (1<<12)-1;
                int octmode = cur_inode.i_mode & octmask;
                //format times (mm/dd/yy hh:mm:ss, GMT):
                char cbuf[30];
                char mbuf[30];
                char abuf[30];
                epoch_to_gmt(cbuf, 30, cur_inode.i_ctime);
                epoch_to_gmt(mbuf, 30, cur_inode.i_mtime);
                epoch_to_gmt(abuf, 30, cur_inode.i_atime);
                printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
                        j+1,filetype,octmode,cur_inode.i_uid,cur_inode.i_gid,cur_inode.i_links_count,cbuf,mbuf,abuf,
                        cur_inode.i_size,cur_inode.i_blocks);
                
                //print if a file, directory, or a symlink bigger than 60 bytes
                if(filetype == 'f' || filetype == 'd' || (filetype == 's' && cur_inode.i_size > 60))
                    for(int k = 0; k < 15; k++)
                        printf(",%u",cur_inode.i_block[k]);
                printf("\n");

                //DIRECTORY ENTRIES---------------
                /*
                1.DIRENT
                2.parent inode number (decimal) ... the I-node number of the directory that contains this entry
                3.logical byte offset (decimal) of this entry within the directory
                4.inode number of the referenced file (decimal)
                5.entry length (decimal)
                6.name length (decimal)
                7.name (string, surrounded by single-quotes). Don't worry about escaping, we promise there will 
                    be no single-quotes or commas in any of the file names.
                */
                
                struct ext2_dir_entry dir_entry;
                unsigned int iter;
                for(int k = 0; k < 12; k++)
                {
                    if(cur_inode.i_block[k] != 0 && filetype == 'd')
                    {
                        iter = 0;
                        while(iter < block_size) 
                        {
                            pread(fd, &dir_entry, sizeof(dir_entry), block_offset(cur_inode.i_block[k]) + iter);
                            if(dir_entry.inode != 0)
                            { 
                                memset(&dir_entry.name[dir_entry.name_len], 0, 256 - dir_entry.name_len);
                                printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                                j+1,iter,dir_entry.inode,dir_entry.rec_len, dir_entry.name_len, dir_entry.name);
                            }
                            iter += dir_entry.rec_len;
                        }
                    }
                }
                //END DIRECTORY ENTRIES---------------
                
                //INDIRECT BLOCK REFERENCES---------------
                /*
                1.INDIRECT
                2.I-node number of the owning file (decimal)
                3.(decimal) level of indirection for the block being scanned... 1 for single indirect, 2 for double indirect, 3 for triple
                4.logical block offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the 
                logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is 
                the same as the logical offset of the first data block to which it refers.
                5.block number of the (1, 2, 3) indirect block being scanned (decimal)... not the highest level block (in the recursive scan), 
                but the lower level block that contains the block reference reported by this entry.
                6.block number of the referenced block (decimal)
                */
                
                
                //single indirect block
                if(cur_inode.i_block[12] != 0)
                {
                    unsigned int* block_ptrs = malloc(block_size);
                    unsigned int num_ptrs = block_size/sizeof(unsigned int);

                    pread(fd, block_ptrs, block_size, block_offset(cur_inode.i_block[12]));
                    for(unsigned int k = 0; k < num_ptrs; k++)
                    {
                        struct ext2_dir_entry dir_entry;
                        unsigned int iter;
                        if(block_ptrs[k] != 0)
                        {
                            if(filetype == 'd')
                            {
                                iter = 0;
                                while(iter < block_size) 
                                {
                                    pread(fd, &dir_entry, sizeof(dir_entry), block_offset(block_ptrs[k]) + iter);
                                    if(dir_entry.inode != 0)
                                    { 
                                        memset(&dir_entry.name[dir_entry.name_len], 0, 256 - dir_entry.name_len);
                                        printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                                        j+1,iter,dir_entry.inode,dir_entry.rec_len, dir_entry.name_len, dir_entry.name);
                                    }
                                    iter += dir_entry.rec_len;
                                }
                            }

                            printf("INDIRECT,%d,%d,%d,%d,%d\n",j+1,1,k+12,cur_inode.i_block[12],block_ptrs[k]);
                        }
                    }
                    free(block_ptrs);
                }

                //double indirect block
                if(cur_inode.i_block[13] != 0)
                {
                    unsigned int* indir_block_ptrs = malloc(block_size);
                    unsigned int num_ptrs = block_size/sizeof(unsigned int);

                    pread(fd, indir_block_ptrs, block_size, block_offset(cur_inode.i_block[13]));
                    for(unsigned int k = 0; k < num_ptrs; k++)
                    {
                        //double indirect printout
                        if(indir_block_ptrs[k] != 0)
                            printf("INDIRECT,%d,%d,%d,%d,%d\n",j+1,2,256+k+12,cur_inode.i_block[13],indir_block_ptrs[k]);

                        //single indirect inside of double
                        unsigned int* block_ptrs = malloc(block_size);

                        pread(fd, block_ptrs, block_size, block_offset(indir_block_ptrs[k]));
                        for(unsigned int l = 0; l < num_ptrs; l++)
                        {
                            struct ext2_dir_entry dir_entry;
                            unsigned int iter;
                            if(block_ptrs[l] != 0)
                            {
                                if(filetype == 'd')
                                {
                                    iter = 0;
                                    while(iter < block_size) 
                                    {
                                        pread(fd, &dir_entry, sizeof(dir_entry), block_offset(block_ptrs[l]) + iter);
                                        if(dir_entry.inode != 0)
                                        { 
                                            memset(&dir_entry.name[dir_entry.name_len], 0, 256 - dir_entry.name_len);
                                            printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                                            j+1,iter,dir_entry.inode,dir_entry.rec_len, dir_entry.name_len, dir_entry.name);
                                        }
                                        iter += dir_entry.rec_len;
                                    }
                                }

                                printf("INDIRECT,%d,%d,%d,%d,%d\n",j+1,1,256+l+12,indir_block_ptrs[k],block_ptrs[l]);
                            }
                        }
                        free(block_ptrs);
                    }
                    free(indir_block_ptrs);
                }

                //triple indirect block
                if(cur_inode.i_block[14] != 0)
                {
                    unsigned int* indir_v2_block_ptrs = malloc(block_size);
                    unsigned int num_ptrs = block_size/sizeof(unsigned int);

                    pread(fd, indir_v2_block_ptrs, block_size, block_offset(cur_inode.i_block[14]));
                    for(unsigned int m = 0; m < num_ptrs; m++)
                    {
                        //double indirect printout
                        if(indir_v2_block_ptrs[m] != 0)
                            printf("INDIRECT,%d,%d,%d,%d,%d\n",j+1,3,65536+256+m+12,cur_inode.i_block[14],indir_v2_block_ptrs[m]);

                        unsigned int* indir_block_ptrs = malloc(block_size);
                        unsigned int num_ptrs = block_size/sizeof(unsigned int);

                        pread(fd, indir_block_ptrs, block_size, block_offset(indir_v2_block_ptrs[m]));
                        for(unsigned int k = 0; k < num_ptrs; k++)
                        {
                            //double indirect printout
                            if(indir_block_ptrs[k] != 0)
                                printf("INDIRECT,%d,%d,%d,%d,%d\n",j+1,2,65536+256+k+12,indir_v2_block_ptrs[m],indir_block_ptrs[k]);

                            //single indirect inside of double
                            unsigned int* block_ptrs = malloc(block_size);

                            pread(fd, block_ptrs, block_size, block_offset(indir_block_ptrs[k]));
                            for(unsigned int l = 0; l < num_ptrs; l++)
                            {
                                struct ext2_dir_entry dir_entry;
                                unsigned int iter;
                                if(block_ptrs[l] != 0)
                                {
                                    if(filetype == 'd')
                                    {
                                        iter = 0;
                                        while(iter < block_size) 
                                        {
                                            pread(fd, &dir_entry, sizeof(dir_entry), block_offset(block_ptrs[l]) + iter);
                                            if(dir_entry.inode != 0)
                                            { 
                                                memset(&dir_entry.name[dir_entry.name_len], 0, 256 - dir_entry.name_len);
                                                printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                                                j+1,iter,dir_entry.inode,dir_entry.rec_len, dir_entry.name_len, dir_entry.name);
                                            }
                                            iter += dir_entry.rec_len;
                                        }
                                    }

                                    printf("INDIRECT,%d,%d,%d,%d,%d\n",j+1,1,65536+256+l+12,indir_block_ptrs[k],block_ptrs[l]);
                                }
                            }
                            free(block_ptrs);
                        }
                        free(indir_block_ptrs);
                    }
                    free(indir_v2_block_ptrs);
                }
                //END INDIRECT BLOCK REFERENCES---------------
            }
        
        //END INODE SUMMARY---------------
   }

    exit(0);
}