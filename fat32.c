#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//constants
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
	char *name;
	int rdonly : 1;
	int hidden : 1;
	int sysfil : 1;
	int volLabel : 1;
	int subdir : 1;
	int archive : 1;
	char pad[10];
	short time;
	short date;
	short stCluster;
	long fileSize;
}dirEntry;  

//function declarations
void clearDrive();
int firstByte(int cluster);
short *getTimeDate();
int firstAvailable();
void formatDrive();
void createDirTable();
void createFATentry(int cluster, short next);
dirEntry *createDirEntry(char *namep, char attributes, short time, short date, short stCluster, long fileSize);
dirEntry *createDirectory(char *path);

//initialize file pointer
FILE *drive;

//initialize cluster "pointers"
int currentDir;
int parentDir;
int currentCluster;
int currentSpace;
int currentOffset;

//initialize time variables
time_t rawtime;
struct tm *timeinfo;


int main(){
	currentDir = ROOT-RESERVED;
	currentCluster = currentDir;
	currentSpace = DATASIZE;
	
	drive = fopen("drive", "rw+");
	char *input = malloc(512);
	
	short *timedate = getTimeDate();

	//initialize boolean for input
        int b = 1;
	
	//create FAT, bootblock, and root directory table
	while(b){
		printf("Type command (f to clear drive and format, q to quit, d to create directory)\n");
		*input = getchar();
		if(*input=='f'){
			//fill drive with NULL
			clearDrive();
			//create FAT, bootblock, and root directory table
			formatDrive();
			printf("drive reformatted\n");
		}
		else if(*input == 'd'){
			printf("Enter directory path without spaces\n");
			scanf("%s", input);
			createDirectory(input);
		}	
		else if(*input=='q')
			b = 0;
	}	 

}

//finds first available cluster
int firstAvailable(){
	fseek(drive, firstByte(FAT)+2, SEEK_SET);
	
	return;
}

void clearDrive(){
	int i = 0;
	char *nul = malloc(512);
	char *p = nul;
	for(i=0; i<512; i++, p++)
		*p = 0x00;
	fseek(drive, 0, SEEK_SET);
	for(i=0; i<SECTORS; i++)
		fwrite(nul, 512,1,drive);
	fseek(drive,0, SEEK_SET);
}

int firstByte(int cluster){
	cluster = (cluster-1)*512;
	return (cluster);
}

short *getTimeDate(){
	short *timedate = malloc(2*sizeof(short));
	short *p = timedate;
	//get time
	time(&rawtime);
        timeinfo = localtime(&rawtime);

	//format time
        short time = (short)(timeinfo->tm_hour<<11);
        time = time+(timeinfo->tm_min<<5);
        time = time +(timeinfo->tm_sec);
	*p = time; p++;

	//format date
	short date = (short)((timeinfo->tm_year-80)<<9);
	date = date + (timeinfo->tm_mon<<5);
	date = date + (timeinfo->tm_mday);
	*p = date; p++;
	return timedate;

}

void formatDrive(){
	fseek(drive, 0, SEEK_SET);
	currentDir = ROOT-RESERVED;
        currentCluster = currentDir;
        currentSpace = DATASIZE;

	//create bootblock in cluster 0
	void *bootblock = malloc(512); 
        char *boot = (char*)bootblock;
        boot = strcat(boot,"My FAT32Lin");
        boot+=11;
        //byte 11-12: num bytes per sector(512)
        *boot = 0x02;
        //byte 13: sectors per cluster(1)
        boot+=2; *boot = 0x01;
        //byte 14-15: number of reserved sectors(1)
        boot+=2; *boot = 0x01;
        //byte 16: number of FAT copies (2)
        boot+=1; *boot = 0x02;
        //byte 17-18: number of root directory entries(0 because FAT32)
        boot+=2;
        //byte 19-20: total number of sectors in filesystem(9760)
        boot+=1; *boot = 0x26; boot++; *boot = 0x20;
        //byte 21: media descriptor type(f8: hard disk?)
        boot+=1; *boot = 0xF8;
        //byte 22-23: sectors per FAT(0 for FAT32)
        boot+=2;
        //byte 24-25: numbers of sectors per track(12?)
        boot+=2; *boot = 0x0C;
        //byte 26-27: nuber of heads(2?)
        boot+=2; *boot = 0x02;
        //byte 28-29: number of hidden sectors(0)
        boot+=2;
        //byte 30-509: would be bootstrap
        boot+=1; boot = strcat(boot, "This would be the bootstrap");
        //byte 510-511: signature 55 aa
        boot+=480; *boot = 0x55; boot++; *boot = 0xaa;

        //write bootblock to drive
	fseek(drive, 0, SEEK_SET);
        fwrite(bootblock, 1, 512,drive);
	free(bootblock);

	//create FATs
	void *doubleFAT = malloc(512*FATSIZE);
        fseek(drive, (BLOCKSIZE*FATSIZE) , SEEK_CUR);
        free(doubleFAT);
	
	//create root directory table
	createDirTable();  
}

//leaves space for directory table 
void createDirTable(){
	//update currentDirectory cluster pointer 
	currentDir = currentCluster;

	//root starts with one block and is dynamically allocated more as needed
	if(currentDir == ROOT)
		createFATentry(currentDir, 0xFFFF);

	fseek(drive, BLOCKSIZE*2, SEEK_CUR);
	currentCluster+=2;
	currentSpace -=2;
}
//creates fat entry and returns pointer to previous position
void createFATentry(int cluster, short next){
	FATentry *new = malloc(sizeof(FATentry));
	new->next = next;
	//seek to FAT entry for cluster and write next
	fseek(drive, firstByte(FAT)+(2*cluster), SEEK_SET);
	fwrite(new, 2, 1, drive);
	//seek to second FAT and write in entry again
	fseek(drive, 38*BLOCKSIZE-2, SEEK_CUR);
	fwrite(new, 2, 1, drive);
	//seek back to previous location
	fseek(drive, firstByte(cluster)+currentOffset, SEEK_SET);
	free(new);
}

dirEntry *createDirectory(char *path){
	int i, j;
	dirEntry *entry = malloc(16*sizeof(dirEntry));
	dirEntry *h = entry;
	dirEntry *dir = malloc(sizeof(dirEntry));
	char *names[17];
	names[0] = strtok(path, "/");
	for(i = 1; names[i]!= NULL && i < 16; i++){
		names[i] = strtok(NULL, "/");
		if(names[i]==NULL)
			i--;
	}
	printf("i = %d", i);
	fseek(drive, firstByte(ROOT), SEEK_SET);
	for(j = 0 ; j < i-1 ; entry++){
		fread(entry, 32, 1, drive);
		currentOffset+=32;
		if(entry->name==names[j]){
			fseek(drive,firstByte(entry->stCluster-currentCluster),SEEK_CUR);
			currentCluster = entry->stCluster; currentOffset = 0;
			parentDir = currentDir; currentDir = currentCluster; 
			j++;
		}
		else if(entry->stCluster == 0 && j!=i-1){
			printf("path not found\n");
			return NULL;
		}
		else if(entry->subdir == 0 && entry->name == names[j]){
			printf("path is a file and not a subdirectory\n");
			return NULL;
		}
		else if(currentOffset == 512){
			printf("No space in current directory table");
			return NULL;
		}
	}
	//Do I need to create . and ..?
	
	//get time
	short *timeDate = getTimeDate();
	short date = *(timeDate+1);
	char attributes = 0x08;

	dir = createDirEntry(names[j], attributes, *timeDate, date, currentCluster, BLOCKSIZE*10);
	createDirTable();

	free(h);

	return dir;
}

dirEntry *createDirEntry(char *namep, char attributes, short time, short date, short stCluster, long filesize){
	dirEntry *entry = malloc(sizeof(dirEntry));
	dirEntry *check = malloc(sizeof(dirEntry));
	int numEntries;
	//get name and update entry
	char *name = namep;	
	entry->name = name;

	//set attributes
	if(attributes>=128){
		attributes -=128;
		entry->rdonly = 1;
	}
	if(attributes>=64){
		attributes -= 64;
		entry->hidden = 1;
	}
	if(attributes>=32){
		attributes -= 32;
		entry->sysfil = 1;
	}
	if(attributes>=16){
		attributes -= 16;
		entry->volLabel = 1;
	}
	if(attributes>=8){
		attributes -= 8;
		entry->subdir = 1;
	}
	if(attributes>=4){
		attributes -= 4;
		entry->archive = 1;
	}
	
	//set time&date
	entry->time = time;
	entry->date = date;

	//set starting cluster and filesize
	entry->stCluster = stCluster;
	entry->fileSize = filesize;

	//seek to currentdir to wirte entry
	fseek(drive, firstByte(currentDir+RESERVED), SEEK_SET);

	//find open entry
	fread(check, 32, 1, drive);
	while(check->stCluster!=0)
		fread(check, 32, 1, drive);
	fwrite(entry, 32, 1, drive);
	fseek(drive, firstByte((entry->stCluster+RESERVED)), SEEK_SET);
	return entry;
}
