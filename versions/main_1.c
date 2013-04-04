#include "disk-array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

int MAXSIZE = 35;
int verbose = 0; 
int level;
int strip;
int ndisks;
int nblocks; 
char * trace; 
int opt;
disk_array_t da;

void print_usage() {
    printf("use: raidsim -level [0|10|4|5] -strip num -disks num -size num -trace filename [-verbose]\n");
}

int main(int argc, char * argv[]) {

    if (argc < 5) {
        exit(1);
    }
	printf("sizeof(int) = %lu\n", sizeof(int));
    static struct option long_options[] = {
        {"level", required_argument, 0, 'l'},
        {"strip", required_argument, 0, 's'},
        {"disks", required_argument, 0, 'd'},
        {"size", required_argument, 0, 'z'},
        {"trace", required_argument, 0, 't'},
        {"verbose", optional_argument, 0, 'v'}
    };

    //printf("Going to test command line inputs.\n");
    int long_index = 0; 
    while ((opt = getopt_long_only(argc, argv, "", long_options,
                    &long_index)) != -1) 
    {
        switch(opt) {
            case 'l':
                level = atoi(optarg);
                break;
            case 's':
                strip = atoi(optarg);
                break;
            case 'd':
                ndisks = atoi(optarg);
                break;
            case 'z':
                nblocks = atoi(optarg);
                break;
            case 't':
                trace = strdup(optarg); 	
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                print_usage();
                exit(1); 
        }
    }	

    da = disk_array_create("myvirtualdisk", ndisks, nblocks);

    int hasFailed[ndisks];

    FILE * fd = fopen(trace, "r");
    if (fd == NULL) {
        print_usage();
        exit(1);
    }

    char * line;
    if ((line = malloc(sizeof(char)*MAXSIZE)) == NULL) {
        exit(1);
    }


    char buffer[BLOCK_SIZE];
    char * cmd; 
    int shouldExit = 0; 
	int i;
    while (fgets(line, MAXSIZE, fd) != NULL && !shouldExit) {
        if (line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = 0; 

        printf("%s\n", line);

        cmd = strtok(line, " ");

        if (strcmp(cmd, "READ") == 0) 
        {
            int LBA = atoi(strtok(NULL, " "));
            int SIZE = atoi(strtok(NULL, " "));

			if (level == 0) {
            for(i = LBA; i < LBA + SIZE; ++i)
            {
				// printf("LBA %d ndisks %d strip %d \n", LBA, ndisks, strip); 
                if(hasFailed[(i % (ndisks*strip))/strip] || 
					disk_array_read(da, (i % (ndisks*strip))/strip, strip *((i/strip)/ndisks) + i % strip, buffer) != 0) printf("ERROR");
                else {
					// printf("sizeof(buffer) = %lu\n", sizeof(buffer));
					printf("%c%c%c%c", buffer[0], buffer[1], buffer[2], buffer[3]);        
				}
                printf(" ");
            }
            printf("\n");
			} else if (level == 10) {
				/* 
				How to read the disks with level 10
				*/
                for(i = LBA; i < LBA + SIZE; ++i) 
                {
                    if(hasFailed[(i % (ndisks*strip))/strip] || 
                            disk_array_read(da, (i % (ndisks*strip))/strip, strip *((i/strip)/ndisks) + i % strip, buffer) != 0) printf("ERROR");
                    else {
                        printf("%c%c%c%c", buffer[0], buffer[1], buffer[2], buffer[3]);        
                    }
                        printf(" ");
        			
                }
                    printf("\n");
			}
        } 
        else if (strcmp(cmd, "WRITE") == 0) 
        {
            int LBA = atoi(strtok(NULL, " "));
            int SIZE = atoi(strtok(NULL, " "));
            char * VALUE = strtok(NULL, " ");

			// printf("VALUE = %s\n", VALUE);	
				
			for (i = 0; i < BLOCK_SIZE; i++) {
				buffer[i] = VALUE[i % 4];
			}
			
			// printf("LBA = %d SIZE = %d VALUE = %d\n", LBA, SIZE, VALUE);	
			// printf("%lu\n", sizeof(VALUE));
			// const char * VALUE = V;
            //  memset(buffer,VALUE,sizeof(buffer));

			// printf("buffer = %s\n", buffer);	
			if (level == 0) {
                for(i = LBA; i < LBA + SIZE; ++i)
                {
                    while(disk_array_write(da, (i % (ndisks*strip))/strip, strip * ((i/strip)/ndisks) + i % strip, buffer) != 0) { } 
                }
			} else if (level == 10) {


			} else if (level == 4) {


			} else if (level == 5) {


			}
        } 
        else if (strcmp(cmd, "FAIL") == 0) 
        {
            int DISK = atoi(strtok(NULL, " "));
            hasFailed[DISK] = 1;
            disk_array_fail_disk(da,DISK);
            //Recalculate parity
        } 
        else if (strcmp(cmd, "RECOVER") == 0) 
        {
            int DISK = atoi(strtok(NULL, " "));
            hasFailed[DISK] = 0;
            disk_array_recover_disk(da,DISK);
            //Rewrite values onto recovered disk and recalculate parity
        } 
        else if (strcmp(cmd, "END") == 0) {
            disk_array_print_stats(da);
            shouldExit = 1;
        }
    }
    free(line);
    return 0; 
}
