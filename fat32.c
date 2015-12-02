#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//constants
#define SECTORS 9765
#define DATASIZE 9688

#define FATSIZE 76
#define RESERVED 77

#define BLOCKSIZE 512

#define ROOT 78
#define FAT 2

//function declarations
void clearDrive();
int firstByte(int cluster);
void formatDrive();
void createDirTable();
void createFATentry(int cluster, short next);
FILE *createDirectory();


//each FAT entry is 2 bytes, holds next cluster in file or special character 
typedef struct FATentry{
	short next;
}FATentry;

//Each Directory table entry is 32 bytes
typedef struct dirEntry{
	char name[11];
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

FILE *drive;

int currentDir;
int currentCluster;
int currentSpace;
int currentOffset;
//void *reserved[77];
//void *clusters[9688];

int main(){
	currentDir = ROOT-RESERVED;
	currentCluster = currentDir;
	currentSpace = DATASIZE;
	
	drive = fopen("drive", "rw+");
	char *input = malloc(512);
	
	//initialize boolean for input
        int b = 1;
	
	/*int i, j, c;
	//create virtual disk
	void *disk = malloc(5000000);
        void *p = disk;
        
	for(i = 0, j=0; i <= 5000000; i += 512, j++){
		p = disk+i;
		if(j<77){
			reserved[j] = p;
		}
		else{
                	clusters[c] = p; c++;
		}
        }*/
	
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
		else if(*input == 'w'){
		
		}	
		else if(*input=='q')
			b = 0;
	}	 
	//free(disk);

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
}

int firstByte(int cluster){
	cluster = cluster*512;
	return (cluster-511);
}

void formatDrive(){
	//create bootblock in cluster 0
	void *bootblock = malloc(512); 
        //void *bootblock = reserved[0];
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
        fwrite(bootblock, 1, 512,drive);
	free(bootblock);

	//create FATs
	void *doubleFAT = malloc(512*FATSIZE);
        //void *doubleFAT = reserved[1];
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
}

FILE *createDirectory(char *path){
	int i;
	FILE *dir;
	char *names[15];
	names[0] = strtok(path, "/");
	for(i = 1; path != NULL && i < 15; i++){
		names[i] = strtok(NULL, "/");
	}
	return dir;
}

