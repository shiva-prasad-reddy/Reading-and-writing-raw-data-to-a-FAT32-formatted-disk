#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<math.h>
#include<stdlib.h>
#include<string.h>

struct BPB {
	int BytsPerSec;
	int SecPerClus;
	int RsvdSecCnt;
	int NumFATs;
	int RootEntCnt;	
	int TotSec32;
	int FATSz32; //In Sectors
	int RootClus;
};

struct DIRECTORY_ENTRY {
	unsigned char Name[256];
	unsigned short int Attr;
	unsigned int StartCluster;
	long int FileSize; //In Bytes
	struct DIRECTORY_ENTRY *next;
	struct DIRECTORY_ENTRY *dir;
};

//To display a part of the buffer
void display_series_of_characters(char *buffer, int length)
{
	for(int i = 0; i < length; i++)
	{
		printf("%c", buffer[i]);
	}
}

//FAT32 stores in Little endian format
//Convert a series of bits to an integer
int turn_to_integer(char *buffer, int offset, int size)
{
	int p = 0;
	int sum = 0;
	unsigned char x;
	for(int i = 0; i < size; i++)
	{
		x = buffer[i+offset];
		for(int j = 0; j < 8; j++)
		{
			if( x & 1 )
			{
				sum = sum + (int)pow(2, p);
			}
			x = x >> 1;
			p++;
		}
	}

	return sum;
}

void display_bios_parameter_block_info(struct BPB *bpb)
{
	printf("\n+++  BIOS Parameter Block Info +++\n");
	printf("BytsPerSec = %d\n", bpb->BytsPerSec);
	printf("SecPerClus = %d\n", bpb->SecPerClus);
	printf("RsvdSecCnt = %d\n", bpb->RsvdSecCnt);
	printf("NumFATs = %d\n", bpb->NumFATs);
	printf("RootEntCnt %d\n", bpb->RootEntCnt);
	printf("TotSec32 = %d\n", bpb->TotSec32);
	printf("FATSz32 = %d\n", bpb->FATSz32);
	printf("RootClus = %d\n", bpb->RootClus);
	printf("++++++\n\n\n");
}

void initializeBPB(int disk, struct BPB *bpb)
{
	unsigned char buffer[512];
	read(disk, buffer, 512);
	bpb->BytsPerSec = turn_to_integer(buffer, 11, 2);
	bpb->SecPerClus = turn_to_integer(buffer, 13, 1);
	bpb->RsvdSecCnt = turn_to_integer(buffer, 14, 2);
	bpb->NumFATs = turn_to_integer(buffer, 16, 1);
	bpb->RootEntCnt = turn_to_integer(buffer, 17, 2);
	bpb->TotSec32 = turn_to_integer(buffer, 32, 4);
	bpb->FATSz32 = turn_to_integer(buffer, 36, 4); //In Sectors
	bpb->RootClus = turn_to_integer(buffer, 44, 4);

	display_bios_parameter_block_info(bpb);
}


void initializeFAT32(int disk, int *FAT, struct BPB *bpb)
{
	lseek(disk, bpb->RsvdSecCnt * bpb->BytsPerSec, SEEK_SET);
	unsigned char *buffer = malloc((bpb->FATSz32 * bpb->BytsPerSec) * sizeof(unsigned char));
	read(disk, buffer, bpb->FATSz32 * bpb->BytsPerSec);
	int NumberOfFATEntries = (bpb->FATSz32 * bpb->BytsPerSec) / 4;
	for(int i = 0; i < NumberOfFATEntries; i++)
	{
		FAT[i] = turn_to_integer(buffer, 4 * i, 4);
	}
	free(buffer);
}


int getFirstSectorOfCluster(int N /*clusterNumber*/, struct BPB *bpb)
{
	int FirstDataSector = bpb->RsvdSecCnt + (bpb->NumFATs * bpb->FATSz32);
	//first two entries in the FAT are not used
	return ((N - 2) * bpb->SecPerClus) + FirstDataSector;
}

//For normal directory . and .. are the first entries except root directory so skip those entries
struct DIRECTORY_ENTRY* readDirecory(int readingPoint, int clusterNumber, int disk, struct BPB *bpb)
{

	//stacks to store long names
	unsigned int TEMP[13], LONG_NAME[256];
	int temp_stack = -1, long_name_stack = -1;

	int rootDirStartSector = getFirstSectorOfCluster(clusterNumber, bpb);
	//Root directory entries are of 32 Bytes
	//and their number of entries are limited to the size of cluster
	//and size is based on the FAT entries

	int DirEntriesInACluster = (bpb->SecPerClus * bpb->BytsPerSec) / 32;
	unsigned char *buffer = malloc((bpb->SecPerClus * bpb->BytsPerSec) * sizeof(unsigned char));

	lseek(disk, rootDirStartSector * bpb->BytsPerSec, SEEK_SET);
	read(disk, buffer, (bpb->SecPerClus * bpb->BytsPerSec));

	//read each entry
	struct DIRECTORY_ENTRY *head, *temp, *last;
	head = temp = last = NULL;

	for(int i = readingPoint; i < DirEntriesInACluster; i++)
	{
		if(buffer[32 * i] == 0x0)
		{
			//printf("End Of Records\n");
			break;
		}
		if(buffer[32 * i] == 0xE5)
		{
			//printf("Unused Record caused By Deletion\n");
			continue;
		}

		int attr = turn_to_integer(buffer + (32 * i), 11, 1);
		int X;

		//LONG FILE NAME ENTRY
		if(attr == 0xF)
		{
			//Read to get the file name
			//Read 2 Bytes at a time to get the character
			//1 -- 10
			for(int j = 1; j < 10; j += 2)
			{
				X = turn_to_integer(buffer + (32 * i), j, 2);
				if(X == 0xffff) break;
				TEMP[++temp_stack] = X;
			}

			//14 -- 25
			for(int j = 14; j < 25; j += 2)
			{
				X = turn_to_integer(buffer + (32 * i), j, 2);
				if(X == 0xffff) break;
				TEMP[++temp_stack] = X;
			}

			//28 -- 31
			for(int j = 28; j < 31; j += 2)
			{
				X = turn_to_integer(buffer + (32 * i), j, 2);
				if(X == 0xffff) break;
				TEMP[++temp_stack] = X;
			}

			//POP AND PUSH INTO LONG NAME STACK
			while(temp_stack != -1) LONG_NAME[++long_name_stack] = TEMP[temp_stack--];

			continue;
		}

		

	
		
		//printf("ATTR == %x\n", attr);
		if(attr == 0x10 || attr == 0x20)
		{
			//printf("Normal Record Detected\n");
			temp = malloc(sizeof(struct DIRECTORY_ENTRY));
			temp->next = NULL;
			temp->dir = NULL;
			if(head == NULL) 
			{
				head = temp;
				last = temp;
			}
			else 
			{
				last->next = temp;
				last = temp;
			}

			int count = 0;
			for(int j = 0; j < 11; j++) temp->Name[count++] = buffer[ (32*i)+j ];

			//above or below
	
			if(long_name_stack != -1)
			{
				count = 0;
				for(int j = long_name_stack; j > -1; j--)
				{
					temp->Name[count++] = (char)LONG_NAME[j];
				}
				temp_stack = long_name_stack = -1;
			}
			temp->Name[count] = '\0';			


			temp->FileSize = turn_to_integer(buffer + (32 * i), 28, 4); //In Bytes
			temp->StartCluster = (turn_to_integer(buffer + (32 * i), 20, 2) * pow(2, 16)) + turn_to_integer(buffer + (32 * i), 26, 2);
			temp->Attr = attr;

			//printf("%.20s\t%.10ld\t%.10d\t%.10s\n", temp->Name, temp->FileSize, temp->StartCluster, temp->Type);
			if(attr == 0x10 /*&& temp->StartCluster != clusterNumber && temp->StartCluster != 0*/)
			{
				//printf("DIRECTORY DETECTED.\n");
				temp->dir = readDirecory(2, temp->StartCluster, disk, bpb);
			}
		}
		
	}

	free(buffer);
	return head;
}

int displaySectorsAllocated(unsigned int *FAT, int start)
{
	//Sector numbers can also be displayed
	int i = 1;
	//printf("%d, ", start);
	while(FAT[start] != 268435455)
	{
		start = FAT[start];
		//printf("%d, ", start);
		i++;
	}
	return i;
}

void displayDIREntries(char *tab, struct DIRECTORY_ENTRY *temp, unsigned int *FAT)
{
	while(temp != NULL)
	{
		char tab2[100];
		//printf("%p\n", temp);
		//printf("%c\n", tab);
		//printf("\n");
		printf("%sFileName = %s\n", tab, temp->Name);
		printf("%sFileSize = %ld Bytes\n", tab, temp->FileSize);
		printf("%sStartCluster = %d\n", tab, temp->StartCluster);
		printf("%sFileAttr = %x %s\n", tab, temp->Attr, (temp->Attr == 0x10 ? "(DIR)" : "(FILE)"));
		if(temp->StartCluster != 0) printf("%sTotalSectors = %d\n", tab, displaySectorsAllocated(FAT, temp->StartCluster));
		//printf("%c\n", tab);

		if(temp->dir != NULL)
		{
			strcpy(tab2, tab);
			strcat(tab2, "\t");
			printf("%s!\n", tab2);
			printf("%s!\n", tab2);
			displayDIREntries(tab2, temp->dir, FAT);
		}
		printf("\n");
		temp = temp->next;
	}
}

int main() {

	int disk = open("/dev/sdb1", O_RDONLY);

	//BIOS parametric Block
	struct BPB bpb;
	initializeBPB(disk, &bpb);


	//Get FAT(file allocation table) into Memory
	int NumberOfFATEntries = (bpb.FATSz32 * bpb.BytsPerSec) / 4;
	unsigned int *FAT;
	FAT = (unsigned int *)calloc(NumberOfFATEntries, sizeof(unsigned int));
	initializeFAT32(disk, FAT, &bpb);

	/*
	printf("%d == %d\n",NumberOfFATEntries, 1988480);
	for(int i = 0; i < NumberOfFATEntries; i++)
	{
		printf("FatEntry[ %d ] = %d \n", i, FAT[i]);
	}
	*/

	struct DIRECTORY_ENTRY *DIR = readDirecory(0, bpb.RootClus, disk, &bpb);
	printf("==========  FILE SYSTEM ===========\n\n");

	char tab[100] = "";

	displayDIREntries(tab, DIR, FAT);



	free(FAT);
	close(disk);

	return 0;
}