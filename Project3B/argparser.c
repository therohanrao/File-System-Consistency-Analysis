//NAME: Rohan Rao,Ethan Ngo
//EMAIL: raokrohan@gmail.com,ethan.ngo2019@gmail.com
//ID: 305339928,205416130

#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h> //standard c file
#include <stdio.h> //standard c file for io
#include <getopt.h> //for getopt_long
#include <sys/types.h> //for open/creat
#include <sys/stat.h> //for open/creat
#include <fcntl.h> //for open/creat
#include <string.h> //for strerror
#include <errno.h> //for error reporting from strerror

int main(int argc, char* argv[])
{

    if(argc > 2 || argc == 1)
    {
        fprintf(stderr, "Incorrect argument provided\n");
        fprintf(stderr, "Usage: ./lab3b FileSystemReport.csv\n");
        exit(1);
    }

    int fd = 0;
    if((fd = open(argv[1], O_RDONLY)) == -1)
    {
        fprintf(stderr, "Failed to open '%s'\n%s\n",argv[1], strerror(errno));
        exit(1);
    }

    size_t len = strlen(argv[1]);
    if(len < 4 || argv[1][len-4] != '.' || argv[1][len-3] != 'c' || argv[1][len-2] != 's' || argv[1][len-1] != 'v')
    {
        fprintf(stderr, "Incorrect argument provided\n");
        fprintf(stderr, "Usage: ./lab3b FileSystemReport.csv\n");
        exit(1);
    }

}