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
dirEntry *createDirEntry(char *namep, char attributes, short time, short date, short stCluster, long fileSize);
dirEntry *createDirectory(char *path);
dirEntry *createFile(char *path);
dirEntry *openFile(char *path);
int writeFile(dirEntry *file, char *write);

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

	dirEntry *file;
	
	drive = fopen("drive", "rw+");
	char cmd;
	
	short *timedate = getTimeDate();

	//initialize boolean for input
        int b = 1;
	
	//create FAT, bootblock, and root directory table
	while(b){
		char *input = malloc(512);
		printf("Type command (f to clear drive and format, q to quit, d to create directory, c to create file, o to open and write to file)\n");
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
			printf("print path of file to write to\n");
			scanf("%s", input);
			file = openFile(input);
			char *p = malloc(5*BLOCKSIZE);
			p = strcat(p, "This is the file it should be where the inside of path at the first open cluster");
			write(file, p);
		}	
		clearInput();
		//free(input);
	}
}
//clears input for scanf
void clearInput(){
	char d;
	while(d!='\n')
		d = fgetc(stdin);
}

//finds first available cluster
int firstAvailable(){
	int i = 1;
	
	//go to FAT and read first entry into f
	fseek(drive, firstByte(FAT)+2, SEEK_SET);
	FATentry *f = malloc(sizeof(FATentry));
	fread(f, 2, 1, drive);
	for(;i<=9688;i++){
		if(f->next == 0){
			fseek(drive, firstByte(i+RESERVED), SEEK_SET);
			return i;
		}
		else
			fread(f, 2, 1, drive);
	}
	printf("No more filesystem space\n");
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
	createFATentry(1, 0xFFFF);  
}

//leaves space for directory table 
void createDirTable(){
	//update currentDirectory cluster pointer 
	currentDir = currentCluster;

	//root starts with one block and is dynamically allocated more as needed
	if(currentDir == ROOT)
		createFATentry(currentDir, 0xFFFF);
	fseek(drive, BLOCKSIZE, SEEK_CUR);
	currentCluster+=1;
	currentSpace -=1;
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
	int i, j, c;
	//create array of entries to read
	dirEntry *entry = malloc(16*sizeof(dirEntry));
	dirEntry *h = entry;

	//create return file descriptor
	dirEntry *dir;
	
	//array of each directory and the file 
	char *names[16];
	char entryName[12];
	
	//seperate path using the slash's
        names[0] = strtok(path, "/");
	for(i = 1; names[i-1]!= NULL && i < 16; i++){
		names[i] = strtok(NULL, "/");
	}
	i--;
	//seek to root directory table
	fseek(drive, firstByte(ROOT), SEEK_SET);
	for(j = 0 ; j < i ; entry++){
		//read entries until path is totally parsed
		fread(entry, 32, 1, drive);
		currentOffset+=32;

		//change entry->name to comparable, NULL terminated string
		if(entry->stCluster!=NULL||entry->stCluster!=0){
			c = 0; 
			while(entry->name[c]!= ' '&& entry->name[c]!=NULL){
				entryName[c] = (char)entry->name[c];
				c++;
			}
		}
		entryName[c] = '\0';
		char *e  = &entryName;
		//find directory to write new directory into
		if(i>j+1){
			if(strcmp(e, names[j])==0){
				fseek(drive,firstByte(entry->stCluster+RESERVED),SEEK_SET);
				currentCluster = entry->stCluster; 
				currentOffset = 0;
				parentDir = currentDir; 
				currentDir = currentCluster; 
				j++;
			}
			else if(entry->stCluster == 0){
				printf("path not found\n");
				return NULL;
			}	
		}
		else if(strcmp(e, names[j])==0){
			printf("File or directory already exists with this name\n");
			return NULL;
		}
		else if(currentOffset == 512){
			printf("No space in current directory table\n");
			return NULL;
		}
		else if(entry->stCluster == 0||entry->stCluster == NULL)
			i--;
		
	}
	//Do I need to create . and ..?
	
	//get time
	short *timeDate = getTimeDate();
	short date = *(timeDate+1);
	char attributes = 0x08;

	//create diectory entry and FAT entry
	currentCluster = firstAvailable();
	dir = createDirEntry(names[j], attributes, *timeDate, date, currentCluster, BLOCKSIZE);
	createFATentry(currentCluster, 0xFFFF);
	createDirTable();

	free(h);

	return dir;
}

dirEntry *createDirEntry(char *namep, char attributes, short time, short date, short stCluster, long filesize){
	int i;
	dirEntry *entry = malloc(sizeof(dirEntry));
	dirEntry *check = malloc(sizeof(dirEntry));
	int numEntries;
	//get name and update entry
	for(i = 0; i <11; i++, namep++){
		if(*namep!=NULL){
			//move to extension
			if(*namep == '.'){
				namep++;
				for(;i<8;i++)
					entry->name[i] = ' ';
			}
			else if(i == 8 && *namep!='.'){
                        	printf("Create file or directory failed, file name too long");
                        	return NULL;
               		}
			entry->name[i] = *namep;
		}
		else{	
			for(; i < 11; i++)
				entry->name[i]= ' ';
		}
	}
	
	//set attributes
	if(attributes>=128){
		attributes -=128;
		entry->rdonly = 1;
	}
	else
		entry->rdonly = 0;
	if(attributes>=64){
		attributes -= 64;
		entry->hidden = 1;
	}
	else
		entry->hidden = 0;
	if(attributes>=32){
		attributes -= 32;
		entry->sysfil = 1;
	}
	else
		entry->sysfil = 0;
	if(attributes>=16){
		attributes -= 16;
		entry->volLabel = 1;
	}
	else
		entry->volLabel = 0;
	if(attributes>=8){
		attributes -= 8;
		entry->subdir = 1;
	}
	else
		entry->subdir = 0;
	if(attributes>=4){
		attributes -= 4;
		entry->archive = 1;
	}
	else
		entry->archive = 0;
	entry->bit = 0;
	entry->bit1 = 0;
		
	for(i = 0; i < 10; i++)
		entry->pad[i] = 0x00;

	//set time&date
	entry->time = time;
	entry->date = date;

	//set starting cluster and filesize
	entry->stCluster = stCluster;
	entry->fileSize = filesize;

	//find empty entry
	fseek(drive, firstByte(currentDir+RESERVED), SEEK_SET);
	fread(check, 32, 1, drive);
	while(check->stCluster!=0)
		fread(check, 32, 1, drive);
	
	//move back to empty entry and write to drive
	fseek(drive, -32, SEEK_CUR);
	fwrite(entry, 32, 1, drive);
	
	//seek back to previous position
	fseek(drive, firstByte((entry->stCluster+RESERVED)), SEEK_SET);
	return entry;
}

dirEntry *createFile(char *path){
	int i, j, c;
	dirEntry *entry = malloc(sizeof(dirEntry));
	dirEntry *h = entry;
	dirEntry *fil;
	char *names[16];
	char entryName[12];
	char *p = path;
	char *e;
	while(*p!= 0){p++;}
	*p = '\0';
	p = path;
	if(strchr(p, '.')!=NULL){
		i = 0;
		while(*p!='.'){
			p++;
			i++;
			if(*p == '/')
				i = 0;
		}
		p = p+(9-i);
		while(i<12){
			*p = *(p+1);
			i++;
			p++;
		}
	}
	names[0] = strtok(path, "/");
        //seperate path using the slash's
        for(i = 1; names[i-1]!= NULL && i < 16; i++){
                names[i] = strtok(NULL, "/");
        }
        i--;
	
        fseek(drive, firstByte(ROOT), SEEK_SET);
        for(j = 0 ; j < i ; ){
                //read entries until path is totally parsed
                fread(entry, 32, 1, drive);
                currentOffset+=32;

                //change entry->name to comparable, NULL terminated string
                if(entry->stCluster!= 0 ||entry->stCluster!='\0'){
                        c = 0;
                        while(entry->name[c]!= ' '&& c < 12){
                                entryName[c] = (char)entry->name[c];
                                c++;
                        }
                }
                entryName[c] = '\0';
                e  = (char*)&entryName;
                //find directory to write new directory into
                if(i>j+1){
                        if(strcmp(e, names[j])==0){
                                fseek(drive,firstByte(entry->stCluster+RESERVED),SEEK_SET);
                                currentCluster = entry->stCluster;
                                currentOffset = 0;
                                parentDir = currentDir;
                                currentDir = currentCluster;
                                j++;
                        }
                        else if(entry->stCluster == 0){
                                printf("path not found\n");
                                return NULL;
                        }
                }
                else if(strcmp(e, names[j])==0){
                        printf("File or directory already exists with this name\n");
                        return NULL;
                }
                else if(currentOffset == 512){
                        printf("No space in current directory table\n");
                        return NULL;
                }
                else if(entry->stCluster == 0||entry->stCluster == NULL)
                        i--;
        }

	//get time 
	short *timeDate = getTimeDate();
        short date = *(timeDate+1);

	char attributes = 0x00;

        currentCluster = firstAvailable();
        fil = createDirEntry(names[j], attributes, *timeDate, date, currentCluster, BLOCKSIZE*2);
        createFATentry(currentCluster, currentCluster+1);
	createFATentry(currentCluster+1, 0xFFFF);

	free(h);
	return fil;
}

dirEntry *openFile(char *path){
	int i, j, c, k;
	dirEntry *entry = malloc(sizeof(dirEntry));
	dirEntry *file = malloc(sizeof(dirEntry));
	char *names[16];
	char entryName[12];
	char *e;
		
	names[0] = strtok(path, "/");
        //seperate path using the slash's
        for(i = 1; names[i-1]!= NULL && i < 16; i++){
                names[i] = strtok(NULL, "/");
        }
        i--;

	fseek(drive, firstByte(ROOT), SEEK_SET);
        for(j = 0 ; j < i ; ){
                //read entries until path is totally parsed
                fread(entry, 32, 1, drive);
                currentOffset+=32;
                //change entry->name to comparable, NULL terminated string
                if(entry->stCluster!= 0){
                        c = 0;
                        while(entry->name[c]!= ' '&& c < 12){
                                entryName[c] = (char)entry->name[c];
                                c++;
                        }
			if(c < 12 && i>j+1){
				entryName[c] = '.';
				k = c+1;
				c = 9;
				for(; c < 12; c++, k++)
					entryName[k] = entry->name[c];
			}
			entryName[k] = '\0';
                }
		e = (char*)&entryName;
                if(i>j+1){
                        if(strcmp(e, names[j])==0){
                                fseek(drive,firstByte(entry->stCluster+RESERVED),SEEK_SET);
                                currentCluster = entry->stCluster;
                                currentOffset = 0;
                                j++;
                        }
                        else if(entry->stCluster == 0){
                                printf("path not found\n");
                                return NULL;
                        }
                }
		else{
			
			if(strcmp(e, names[j])==0){
				file = entry;
				i--;
			}
			else if(entry->stCluster == 0 || currentOffset == 512){
				printf("file not found\n");
				return NULL;
			}
		}
	}
	return file;
}

int writeFile(dirEntry *file, char *write){
	int i = 0;
	char cluster[512];
	char *c;
	if(file->stCluster!= 0){
		//go to end of file
		while(c!='\0'){
			fread(c, 1, 1, drive);
 		}
		fseek(drive, -1, SEEK_CUR);
		while(*write != '\0'){
			for(i = 0; i < 512 && *write == '\0'; i++, write++)
				cluster[i] = *write;
			fwrite((char*)&cluster, i, 1, drive);
		}
		return 0;
	}
	else
		return -1;
}
