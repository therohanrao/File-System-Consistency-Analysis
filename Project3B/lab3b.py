#!/usr/bin/env python3

#NAME: Rohan Rao,Ethan Ngo
#EMAIL: raokrohan@gmail.com,ethan.ngo2019@gmail.com
#ID: 305339928,205416130

import sys
import csv

'''
PYTHON NOTES:

print("hello world", file=sys.stderr)
    To print to stderr do this


sys.argv[#] 
    to access command line args as you would in c

print(', '.join(row)) 
    to collapse a list into a string delimited by
    the specified string in ' '

global variablename
    use to reference and modify global variables inside functions
'''

#GLOBAL VARS--------------
exit_code = 0


#END GLOBAL VARS----------



class Superblock_Summary:
    def __init__(self):
        self.num_blocks = 0
        self.num_inodes = 0
        self.block_size = 0
        self.inode_size = 0
        self.blocks_per_group = 0
        self.inodes_per_group = 0
        self.f_nonres_inode = 0
    
    def initialize(self, nb, ni, bsz, isz, bpg, ipg, fni):
        self.num_blocks = nb
        self.num_inodes = ni
        self.block_size = bsz
        self.inode_size = isz
        self.blocks_per_group = bpg
        self.inodes_per_group = ipg
        self.f_nonres_inode = fni

class Group_Summary:
    def __init__(self):
        self.group_number = 0
        self.num_blocks = 0
        self.num_inodes = 0
        self.num_free_blocks = 0
        self.num_free_inodes = 0
        self.block_bitmap = 0
        self.inode_bitmap = 0
        self.f_inode_block = 0

    def initialize(self, gn, nb, ni, nfb, nfi, bb, ib, fib):
        self.group_number = gn
        self.num_blocks = nb
        self.num_inodes = ni
        self.num_free_blocks = nfb
        self.num_free_inodes = nfi
        self.block_bitmap = bb
        self.inode_bitmap = ib
        self.f_inode_block = fib

class Inode_Summary:
    def __init__(self):
        self.inode_number = 0
        self.file_type = 0
        self.mode = 0
        self.owner = 0
        self.group = 0
        self.link_count = 0
        self.time_create = 0
        self.time_modify = 0
        self.time_access = 0
        self.file_size = 0
        self.size_in_blocks = 0

    def initialize(self, inbr, ft, m, o, g, lc, tc, tm, ta, fs, sib):
        self.inode_number = inbr
        self.file_type = ft
        self.mode = m
        self.owner = o
        self.group = g
        self.link_count = lc
        self.time_create = tc
        self.time_modify = tm
        self.time_access = ta
        self.file_size = fs
        self.size_in_blocks = sib



def process_csv(filename):
    with open(filename, newline='') as csvfile:
        filereader = csv.reader(csvfile, delimiter=',')

        #putting data in list variable
        csvin = list()
        for row in filereader:
            csvin.append(row)

        #REFERENCES TO 'exit_code' NOW REFER TO GLOBAL 'exit_code'
        global exit_code

        it_temp = -1
        #OBJECTS FROM CSV DATA
        superblock = Superblock_Summary()
        group = Group_Summary()
        bfree_list = list()
        ifree_list = list()

        #HELPER OBJECTS 
        breserved_list = list()
        ireserved_list = list()
        
        unreferenced_blocks = list()
        referenced_blocks = list()
        dup_ref_blocks = dict()

        allocated_inodes = list()

        dirent_inode_references = dict()

        ptoc = dict() #parent to child directory
        ctop = dict() #child to parent directory

        #ADDING SUPERBLOCK AND GROUP DESCRIPTOR TABLE TO RESERVED AND REFERENCED LISTS
        breserved_list.extend([1, 2])
        referenced_blocks.extend([1,2])

        for row in csvin:
            it_temp += 1
            #print(row)
            #checks for basic csv validity, might not need
            if it_temp == 0 and row[0] != "SUPERBLOCK":
                print("Invalid csv: First entry not SUPERBLOCK...", file=sys.stderr)
                exit(2)
            if it_temp == 1 and row[0] != "GROUP":
                print("Invalid csv: Second entry not Group...", file=sys.stderr)
                exit(2)

            #POPULATING SUPERBLOCK OBJECT AND REFERENCE LISTS
            if row[0] == "SUPERBLOCK":
                superblock.initialize(int(row[1]), int(row[2]), int(row[3]), int(row[4]),
                                      int(row[5]), int(row[6]), int(row[7]))
                ireserved_list.extend(list(range(0, superblock.f_nonres_inode)))
                unreferenced_blocks.extend(list(range(0,superblock.num_blocks))) #TOOK OUT PLUS 1

            #POPULATING GROUP OBJECT AND REFERENCE LISTS
            if row[0] == "GROUP":
                group.initialize(int(row[1]), int(row[2]), int(row[3]), int(row[4]), 
                                 int(row[5]), int(row[6]), int(row[7]), int(row[8]))
                breserved_list.extend([group.block_bitmap, group.inode_bitmap, group.f_inode_block])
                referenced_blocks.extend([group.block_bitmap, group.inode_bitmap, group.f_inode_block])
                for _ in range(group.f_inode_block, group.f_inode_block + ((superblock.inode_size*superblock.inodes_per_group)//superblock.block_size)):
                    if _ not in referenced_blocks:
                        referenced_blocks.append(_)

            #POPULATING FREE BLOCK LIST
            if row[0] == "BFREE":
                if row[1] not in bfree_list:
                    bfree_list.append(int(row[1]))
            #POPULATING FREE INODE LIST
            if row[0] == "IFREE":
                if row[1] not in ifree_list:
                    ifree_list.append(int(row[1]))
            #POPULATING REFERENCED BLOCKS
            if row[0] == "INODE" and row[2] != "s":
                for _ in row[12:]:
                    if int(_) not in referenced_blocks:
                        referenced_blocks.append(int(_))
            if row[0] == "INDIRECT":
                if int(row[5]) not in referenced_blocks:
                        referenced_blocks.append(int(row[5]))
            #POPULATING RESERVED INODES
            if row[0] == "INODE":
                if int(row[1]) not in allocated_inodes:
                    allocated_inodes.append(int(row[1]))

            #RESERVED AND INVALID BLOCKS ALLOCATED
            if row[0] == "INODE" and row[2] != "s":
                temp_ = -1
                for _ in row[12:]:
                    temp_ += 1
                    ind_block = "BLOCK"
                    offset = temp_
                    if temp_ == 12:
                        ind_block = "INDIRECT BLOCK"
                        offset = 12
                    elif temp_ == 13:
                        ind_block = "DOUBLE INDIRECT BLOCK"
                        offset = 268
                    elif temp_ == 14:
                        ind_block = "TRIPLE INDIRECT BLOCK"
                        offset = 65804
                    if int(_) in breserved_list:
                        print("RESERVED", ind_block, _, "IN INODE", row[1], "AT OFFSET", offset)
                        exit_code = 2
                    if int(_) < 0 or int(_) > superblock.num_blocks:
                        print("INVALID", ind_block, _, "IN INODE", row[1], "AT OFFSET", offset)
                        exit_code = 2
                    
            #ALLOCATED BLOCK ON FREE LIST
            if row[0] == "INODE" and row[2] != "s":
                for _ in row[12:]:
                    if int(_) in bfree_list:
                        print("ALLOCATED BLOCK", _, "ON FREELIST")
                        exit_code = 2
            
            #SUM # OF BLOCK REFERENCES FOR EACH BLOCK
            if row[0] == "INODE" and row[2] != "s":
                    for _ in row[12:]:
                        if int(_) not in dup_ref_blocks and int(_) != 0:
                            dup_ref_blocks[int(_)] = 1
                        elif int(_) != 0:
                            dup_ref_blocks[int(_)] += 1

            #POPULATE INODE REFERENCES FROM DIRENTS
            if row[0] == "DIRENT":
                if int(row[3]) not in dirent_inode_references:
                    dirent_inode_references[int(row[3])] = 1
                else:
                    dirent_inode_references[int(row[3])] += 1

            #CUR '.' DIRECTORY VALIDITY CHECK
            if row[0] == "DIRENT":
                if row[6] == "'.'" and row[1] != row[3]:
                    print("DIRECTORY INODE", row[1], "NAME '.' LINK TO INODE", row[3], "SHOULD BE", row[1])
                    exit_code = 2
                elif row[6] == "'..'":
                    ptoc[int(row[1])] = int(row[3])
                elif row[6] != "'.'":
                    ctop[int(row[3])] = int(row[1])
                if int(row[1]) == 2 and row[6] == "'..'" and row[1] != row[3]:
                    print("DIRECTORY INODE", row[1], "NAME '..' LINK TO INODE", row[3], "SHOULD BE", row[1])
                    exit_code = 2
            #IN FOR LOOP----------
        #END FOR LOOP----------

        #DUPLICATE REFERENCES
        for row in csvin:
            if row[0] == "INODE":
                temp_ = -1
                for _ in row[12:]:
                    temp_ += 1
                    if int(_) in dup_ref_blocks and dup_ref_blocks[int(_)] > 1:
                        ind_block = "BLOCK"
                        block_num = int(_)
                        offset = temp_
                        if temp_ == 12:
                            ind_block = "INDIRECT BLOCK"
                            offset = 12
                        elif temp_ == 13:
                            ind_block = "DOUBLE INDIRECT BLOCK"
                            offset = 268
                        elif temp_ == 14:
                            ind_block = "TRIPLE INDIRECT BLOCK"
                            offset = 65804
                        print("DUPLICATE", ind_block, block_num, "IN INODE", row[1], "AT OFFSET", offset)
                        exit_code = 2

        # UNREFERENCED BLOCKS
        for _ in referenced_blocks:
            if _ in unreferenced_blocks:
                unreferenced_blocks.remove(_)
        for _ in bfree_list:
            if _ in unreferenced_blocks:
                unreferenced_blocks.remove(_)
        for _ in unreferenced_blocks:
            print("UNREFERENCED BLOCK", _)
            exit_code = 2

        #INODE ALLOCATION AUDITS
        for _ in ifree_list:
            if _ in allocated_inodes:
                print("ALLOCATED INODE", _, "ON FREELIST")
                exit_code = 2
        for _ in range(superblock.f_nonres_inode, group.num_inodes):
            if _ not in allocated_inodes and _ not in ifree_list:
                print("UNALLOCATED INODE", _, "NOT ON FREELIST")
                exit_code = 2

        #INODE LINK COUNT
        for row in csvin:
            if row[0] == "INODE":
                if int(row[1]) in dirent_inode_references:
                    if dirent_inode_references[int(row[1])] != int(row[6]):
                        print("INODE", row[1], "HAS", dirent_inode_references[int(row[1])], "LINKS BUT LINKCOUNT IS", row[6])
                        exit_code = 2
                elif int(row[6]) != 0:
                    print("INODE", row[1], "HAS 0", "LINKS BUT LINKCOUNT IS", row[6])
                    exit_code = 2
        
        #INVALID AND UNALLOCATED DIRENT REFERENCE TO INODE
        for row in csvin:
            if row[0] == "DIRENT":
                if int(row[3]) < 0 or int(row[3]) > group.num_inodes:
                    print("DIRECTORY INODE", row[1],"NAME", row[6], "INVALID INODE", row[3])
                    exit_code = 2
                elif int(row[3]) not in allocated_inodes:
                    print("DIRECTORY INODE", row[1],"NAME", row[6], "UNALLOCATED INODE", row[3])
                    exit_code = 2

        #VALIDATE PARENT AND CHILD REFERENCES
        #THEORETICALLY WORKS BUT THERE ARE NO TEST CASES FOR IT
        for p in ptoc:
            if p == 2:
                continue
            elif p not in ctop:
                print("DIRECTORY INODE", ptoc[p], "NAME '..' LINK TO INODE", p, " SHOULD BE", ptoc[p])
                exit_code = 2
            elif ptoc[p] != ctop[p]:
                print("DIRECTORY INODE", p + " NAME '..' LINK TO INODE " + p + " SHOULD BE " + ptoc[p])
                exit_code = 2
    #END 'process_csv'
def main():
    print("Running File System check on:", sys.argv[1], file=sys.stderr)
    process_csv(sys.argv[1])
    exit(exit_code)

if __name__ == "__main__":
    main()