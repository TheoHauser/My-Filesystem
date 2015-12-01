#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
	//initialize counters and pointers to the disk and clusters, reserved clusters/sectors
        int i, j, c;
	void *reserved[6];
        void *clusters[9760];
	//virtual disk
	void *disk = malloc(5000000);
        void *p = disk;
        for(i = 0, j=0; i <= 5000000; i += 512, j++){
		p = disk+i;
		if(j<6){
			reserved[j] = p;
		}
		else{
                	clusters[c] = p; c++;
		}
        }

	FILE *drive = fopen("drive", "rw+");
	
	//create bootblock in cluster 0 
	void *bootblock = reserved[0]; 
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
	fwrite(bootblock, 512, 1,drive);

	//create FAT and FAT2  in reserved clusters
			
	
		 
	free(disk);

}
