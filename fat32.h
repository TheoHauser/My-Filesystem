#ifndef FAT32_C
#define FAT32_C

#include <stdio.h>

#define SECTORS 9765
#define DATASIZE 9688

#define FATSIZE 76
#define RESERVED 77

#define BLOCKSIZE 512

#define ROOT 78
#define FAT 2

//each FAT entry is 2 bytes, holds next cluster in file or special character 
typedef struct FATentry{
        short next;
}FATentry;

//Each Directory table entry is 32 bytes
typedef struct dirEntry{
        char name[11];
        unsigned int rdonly : 1;
        unsigned int hidden : 1;
        unsigned int sysfil : 1;
        unsigned int volLabel : 1;
        unsigned int subdir : 1;
        unsigned int archive : 1;
        unsigned int bit : 1;
        unsigned int bit1 : 1;
        char pad[10];
        short time;
        short date;
        short stCluster;
        long fileSize;
}dirEntry;  

//function declarations
void clearInput();
void clearDrive();
int firstByte(int cluster);
short *getTimeDate();
int firstAvailable();
void formatDrive();
void createDirTable();
void createFATentry(int cluster, short next);
int getNextCluster(int cluster);
dirEntry *createDirEntry(char *namep, char attributes, short time, short date, short stCluster, long fileSize);
dirEntry *createDirectory(char *path);
dirEntry *createFile(char *path);
dirEntry *openFile(char *path);
int closeFile(dirEntry *file);
int writeFile(dirEntry *file, char *write);
char *readFile(dirEntry *file);
int deleteFile(char *path);

FILE *drive;

#endif 
