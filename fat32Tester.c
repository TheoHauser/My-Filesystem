#include "fat32.h"
#include <string.h> 
#include <stdio.h>
#include <stdlib.h>

int main(){
        dirEntry *file;
        char *p = malloc(5*BLOCKSIZE);

        drive = (FILE*)fopen("drive", "rw+");
        char cmd;

        short *timedate = getTimeDate();

        /*//initialize boolean for input
        int b = 1;

        //create FAT, bootblock, and root directory table
        while(b){
                char *input = malloc(512);
                printf("Type command (f to clear drive and format, q to quit, d to create directory, c to create file, o to open file, w to write to file, x to delete file, z to close file, and r to read file)\n");
                cmd = getchar();
                if(cmd=='f'){
                        //fill drive with NULL
                        clearDrive();
                        //create FAT, bootblock, and root directory table
                        formatDrive();
                        printf("drive reformatted\n");
                }
                else if(cmd == 'd'){
                        clearInput();
                        printf("Enter directory path without spaces\n");
                        scanf("%s", input);
                        createDirectory(input);
                }
                else if(cmd == 'c'){
                        clearInput();
                        printf("Enter file path with extension eg. '.txt'\n");
                        scanf("%s", input);
                        createFile(input);
                }
                else if(cmd=='q')
                        b = 0;
                else if(cmd == 'o'){
                        clearInput();
                        printf("print path of file to open\n");
                        scanf("%s", input);
                        file = openFile(input);
                }
                else if(cmd == 'w'){
                        strcpy(p, "This is the file it should be where the inside of path at the first open cluster");
                        int s = writeFile(file, p);
                        printf("write succesful: %d\n", s);
		}
		else if(cmd == 'x'){
                        clearInput();
                        printf("Enter file path to delete\n");
                        scanf("%s", input);
                        deleteFile(input);
                }
                else if(cmd == 'z'){
                        closeFile(file);
                }
                else if(cmd == 'r'){
                        char *d = readFile(file);
                        if(strcmp(d, p)==0){
                                printf("The strings:\n%s\nand\n%s\nMatch\n", d, p);
                        }
                }
                clearInput();
                //free(input);
        }*/
	int i;
	char *directory = malloc(11*10);
	char *file1 = malloc(11*10);
	char *file2 = malloc(11*10);

	strcpy(file1, "/path/file.txt/");
	strcpy(file2, file1);
	strcpy(directory, "/path/");
	
	clearDrive();
	formatDrive();
	createDirectory(directory);
	createFile(file1);
	file = openFile(file2);
	strcpy(p, "This is the text to be written and compared with what is read");
	for(i = 0; i < 512; i++)
		strcat(p, "i");
	strcat(p, "512");
	writeFile(file, p);
	char *d = readFile(file);
        if(strcmp(d, p)==0){
        	printf("The strings:\n%s\nand\n%s\nMatch\n", d, p);
       	}
	else
                printf("The strings:\n%s\nand\n%s\ndont match\n", d, p);

	
	
	
}

