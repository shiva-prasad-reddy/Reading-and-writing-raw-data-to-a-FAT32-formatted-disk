#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<math.h>
#include<stdlib.h>
#include<string.h>

#define char unsigned char
//#define int unsigned int //caused error in stacks used for long name(Bus Error)
#define DIR_RECORD_SIZE 32 //32 Bytes
#define FAT_RECORD_SIZE 4	//4 Bytes
#define EOC 0x0fffffff //END OF CLUSTER CHAIN

int disk;

//BIOS Parameter Block
struct BPB {
	int BytsPerSec;
	int SecPerClus;
	int RsvdSecCnt;
	int NumFATs;
	int RootEntCnt;	
	int TotSec32;
	int FATSz32; //In Sectors
	int RootClus;
}*bpb;

struct DIRECTORY {
	char Name[256];
	int Attr;
	int StartCluster;
	int FileSize; //In Bytes
	struct DIRECTORY *next;
	struct DIRECTORY *dir;
};


//To display a part of the buffer
void print_text(char *buffer, int lenInBytes)
{
	for(int i = 0; i < lenInBytes; i++) printf("%c", buffer[i]);
}

//FAT32 stores in Little endian format
//Convert a series of bits to an integer
int little_endian_to_integer(char *buffer, int offset, int size)
{
	int power, sum;
	power = sum = 0;

	char X;
	for(int i = 0; i < size; i++)
	{
		X = buffer[i + offset];
		for(int j = 0; j < 8; j++)
		{
			if( X & 1 )
				sum = sum + (int)pow(2, power);
			
			X = X >> 1;
			power++;
		}
	}

	return sum;
}

void print_BPB()
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

void init_BPB()
{
	//First 512 Bytes contain the information of BPB
	bpb = malloc(sizeof(struct BPB));
	char buffer[512];
	read(disk, buffer, 512);
	bpb->BytsPerSec = little_endian_to_integer(buffer, 11, 2);
	bpb->SecPerClus = little_endian_to_integer(buffer, 13, 1);
	bpb->RsvdSecCnt = little_endian_to_integer(buffer, 14, 2);
	bpb->NumFATs = little_endian_to_integer(buffer, 16, 1);
	bpb->RootEntCnt = little_endian_to_integer(buffer, 17, 2);
	bpb->TotSec32 = little_endian_to_integer(buffer, 32, 4);
	bpb->FATSz32 = little_endian_to_integer(buffer, 36, 4); //In Sectors
	bpb->RootClus = little_endian_to_integer(buffer, 44, 4);
}


void init_FAT32(int *FAT)
{
	//There are 2 FAT tables
	//FAT32 -- 32 bits (4 Bytes) but only 28 bits are used and high 4 bits are reserved
	
	int startByteOfFatTables = bpb->RsvdSecCnt * bpb->BytsPerSec;
	int fatSizeInBytes = bpb->FATSz32 * bpb->BytsPerSec;
	int numberOfFATEntries = fatSizeInBytes / FAT_RECORD_SIZE;

	char *buffer = malloc(fatSizeInBytes * sizeof(char));

	lseek(disk, startByteOfFatTables, SEEK_SET);
	read(disk, buffer, fatSizeInBytes);
	
	for(int i = 0; i < numberOfFATEntries; i++) 
		FAT[i] = little_endian_to_integer(buffer, 4 * i, 4);

	free(buffer);
}

void print_FAT32(int *FAT, int count)
{
	for(int i = 0; i < count; i++) 
		printf("FatEntry[ %d ] = %d \n", i, FAT[i]);
}



int first_sector_of_cluster(int N /*clusterNumber*/)
{
	//Remeber N starts from value 2
	//because the first 2 entries in the FAT table are not used
	int FirstDataSector = bpb->RsvdSecCnt + (bpb->NumFATs * bpb->FATSz32);
	return ((N - 2) * bpb->SecPerClus) + FirstDataSector;
}

//For normal directory . and .. are the first entries except root directory so skip those entries
struct DIRECTORY* read_directory(int clusterNumber, int recordStartingPoint, int *FAT)
{

	//stacks to store long names
	int TEMP[13], LONG_NAME[256];
	int temp_stack = -1, long_name_stack = -1;

	int sector = first_sector_of_cluster(clusterNumber);
	int clusterSizeInBytes = bpb->SecPerClus * bpb->BytsPerSec;
	//Root directory entries are of 32 Bytes
	//and their number of entries are limited to the size of cluster
	//and size is based on the FAT entries
	//Say Directory Entries as records
	int totalRecords = (bpb->SecPerClus * bpb->BytsPerSec) / DIR_RECORD_SIZE;
	char *buffer = malloc(clusterSizeInBytes * sizeof(char));

	lseek(disk, sector * bpb->BytsPerSec, SEEK_SET);
	read(disk, buffer, clusterSizeInBytes);

	//read each entry
	struct DIRECTORY *head, *temp, *last;
	head = temp = last = NULL;


	//readingPoint is used to skip these records in a directory
	// . --> points to the same directory
	// .. --> points to the parent directory
	for(int i = recordStartingPoint; i < totalRecords; i++)
	{
		//End Of Records
		if(buffer[32 * i] == 0x0) break;

		//Unused Record caused By Deletion
		if(buffer[32 * i] == 0xE5) continue;

		int attr = little_endian_to_integer(buffer + (32 * i), 11, 1);
		int X; //temprary variable
		//LONG FILE NAME ENTRY
		if(attr == 0xF)
		{
			//Code to get the long file name
			//Read 2 Bytes at a time to get the character
			//1 -- 10
			for(int j = 1; j < 10; j += 2)
			{
				X = little_endian_to_integer(buffer + (32 * i), j, 2);
				if(X == 0xffff) break; //end of file name
				TEMP[++temp_stack] = X;
			}

			//14 -- 25
			for(int j = 14; j < 25; j += 2)
			{
				X = little_endian_to_integer(buffer + (32 * i), j, 2);
				if(X == 0xffff) break; //end of file name
				TEMP[++temp_stack] = X;
			}

			//28 -- 31
			for(int j = 28; j < 31; j += 2)
			{
				X = little_endian_to_integer(buffer + (32 * i), j, 2);
				if(X == 0xffff) break; //end of file name
				TEMP[++temp_stack] = X;
			}

			//POP AND PUSH INTO LONG NAME STACK
			while(temp_stack != -1) LONG_NAME[++long_name_stack] = TEMP[temp_stack--];

			continue; //Because a Long file name entry detected so skip
		}
		//Only files and directories are considered
		//and other system related files are ignored
		//based on Attr
		if(attr == 0x10 || attr == 0x20)
		{
			temp = malloc(sizeof(struct DIRECTORY));
			temp->next = temp->dir = NULL;

			if(head == NULL) 
				head = temp;
			else 
				last->next = temp;
			
			last = temp;

			int count = 0; //used to count characters
			for(int j = 0; j < 11; j++) 
				temp->Name[count++] = buffer[ (32*i)+j ];
			//finds file name from above or below code
			if(long_name_stack != -1)
			{
				count = 0;
				for(int j = long_name_stack; j > -1; j--)
					temp->Name[count++] = (char)LONG_NAME[j];

				temp_stack = long_name_stack = -1;
			}
			temp->Name[count] = '\0';			

			temp->FileSize = little_endian_to_integer(buffer + (32 * i), 28, 4); //In Bytes
			temp->StartCluster = (little_endian_to_integer(buffer + (32 * i), 20, 2) * pow(2, 16)) + little_endian_to_integer(buffer + (32 * i), 26, 2);
			temp->Attr = attr;

			//to read directories in directories
			if(attr == 0x10)
				temp->dir = read_directory(temp->StartCluster, 2, FAT); //2 to skip . and .. dir entries
			
			//if the current directory has a chain of clusters
			//to traverse the next cluster in chain
			if(FAT[clusterNumber] != EOC)
				last->next = read_directory(FAT[clusterNumber], recordStartingPoint, FAT);
		}
	}

	free(buffer);
	return head;
}

void display_cluster_chain(int *FAT, int start)
{
	printf("%d, ", start);
	while(FAT[start] != 0x0fffffff)
	{
		start = FAT[start];
		printf("%d, ", start);
	}
}

void print_directory(struct DIRECTORY *dir, char *tab) //tab is only used for printing purpose.
{
	while(dir)
	{
		printf("%sFileName = %s\n", tab, dir->Name);
		printf("%sFileSize = %d Bytes\n", tab, dir->FileSize);
		printf("%sStartCluster = %d\n", tab, dir->StartCluster);
		printf("%sFileAttr = %x %s\n", tab, dir->Attr, (dir->Attr == 0x10 ? "(DIR)" : "(FILE)"));
		if(dir->dir != NULL)
		{
			char tab2[100];
			strcpy(tab2, tab);
			strcat(tab2, "\t");
			printf("%s!\n", tab2);
			printf("%s!\n", tab2);
			print_directory(dir->dir, tab2);
		}
		printf("\n");
		dir = dir->next;
	}
}

//create a directory record and copy into buffer to write back into memory
void create_directory_record(char *buffer, char *Name, int Attr, int N/*cluster number*/)
{
	//Name
	int nameLength = sizeof(Name)/sizeof(char);
	for(int i = 0; i < 11; i++)
	{
		if(i < nameLength)
			buffer[i] = Name[i];
		else
			buffer[i] = 32; //trailing space padded
	}

	//Attr
	buffer[11] = Attr;

	//NTRes
	buffer[12] = 0;

	//CrtTimeTenth
	buffer[13] = 0;

	//CrtTime
	buffer[14] = 0;
	buffer[15] = 0;

	//CrtDate
	buffer[16] = 0;
	buffer[17] = 0;

	//LstAccDate
	buffer[18] = 0;
	buffer[19] = 0;

	//FstClusHI
	buffer[20] = (N >> 16) & 0xFF;
	buffer[21] = (N >> 24) & 0xFF;

	//WrtTime
	buffer[22] = 0;
	buffer[23] = 0;

	//WrtDate
	buffer[24] = 0;
	buffer[25] = 0;

	//FstClusLO
	buffer[26] = N & 0xFF;
	buffer[27] = (N >> 8) & 0xFF;

	//FileSize
	buffer[28] = 0;
	buffer[29] = 0;
	buffer[30] = 0;
	buffer[31] = 0;
}

int get_free_cluster(int *FAT)
{
	//I'm skipping first 10 and Last 20 entries
	int count = (bpb->FATSz32 * bpb->BytsPerSec) / FAT_RECORD_SIZE;
	for(int i = 10; i < (count - 20); i++)
		if(FAT[i] == 0)
			return i;
}

void write_to_fat_tables(int freeCluster)
{
	char fatEntry[4];
	fatEntry[0] = (EOC) & 0xFF;
	fatEntry[1] = (EOC >> 8) & 0xFF;
	fatEntry[2] = (EOC >> 16) & 0xFF;
	fatEntry[3] = (EOC >> 24) & 0xFF;

	//Occupying the FAT Entry
	//FAT1 fat entry
	lseek(disk, (bpb->RsvdSecCnt * bpb->BytsPerSec) + (freeCluster * 4), SEEK_SET);
	write(disk, fatEntry, 4);
	//FAT2 fat entry
	lseek(disk, bpb->RsvdSecCnt * bpb->BytsPerSec + (bpb->FATSz32 * bpb->BytsPerSec) + (freeCluster * 4), SEEK_SET);
	write(disk, fatEntry, 4);
}

void write_to_cur_dir_cluster(char *newDirEntry, int curDirClusNum, int *FAT)
{
	//find a free entry in the curDir and copy the newDirEntry
	int startSector = first_sector_of_cluster(curDirClusNum);
	//Root directory entries are of 32 Bytes
	//and their number of entries are limited to the size of cluster

	int clusterSizeInBytes = bpb->SecPerClus * bpb->BytsPerSec;
	int recordCount = clusterSizeInBytes / DIR_RECORD_SIZE;
	char *buffer = malloc(clusterSizeInBytes * sizeof(char));

	lseek(disk, startSector * bpb->BytsPerSec, SEEK_SET);
	read(disk, buffer, clusterSizeInBytes);

	int freeRecord;
	for(int i = 0; i < recordCount; i++)
	{
		//End Of Records or Unused Records caused By Deletion
		if(buffer[32 * i] == 0x0 || buffer[32 * i] == 0xE5)
		{
			freeRecord = i;
			break;
		}
	}

	if((freeRecord + 1) == recordCount)
	{
		//go to the next cluster in cluster chain for creating the entry
		//because cur dir is FULL
		free(buffer);
		return write_to_cur_dir_cluster(newDirEntry, FAT[curDirClusNum], FAT);

	}
	lseek(disk, (startSector * bpb->BytsPerSec) + (freeRecord * DIR_RECORD_SIZE), SEEK_SET);
	write(disk, newDirEntry, DIR_RECORD_SIZE);
	free(buffer);
}

void write_to_new_dir_cluster(char *_firstEntry, char *_secondEntry, int freeCluster)
{
	//Go to that free cluster and copy 0x0 inside the whole cluster(not efficient)
	//and place the entries _firstEntry and _secondEntry

	int clusterSizeInBytes = bpb->SecPerClus * bpb->BytsPerSec;
	char *buffer = calloc(clusterSizeInBytes, sizeof(char));
	//copy first two entries
	for(int i = 0; i < 32; i++)
	{
		buffer[i] = _firstEntry[i];
		buffer[32 + i] = _secondEntry[i];
	}

	int startSector = first_sector_of_cluster(freeCluster);
	lseek(disk, startSector * bpb->BytsPerSec, SEEK_SET);
	write(disk, buffer, clusterSizeInBytes);
	free(buffer);
}

void create_directory(char *Name, int *FAT, int curDirClusNum)
{

	//code should be optimized for every directory and accessed byte wise or sector wise
	//and remove redundant buffers
	char newDirEntry[DIR_RECORD_SIZE], _firstEntry[DIR_RECORD_SIZE], _secondEntry[DIR_RECORD_SIZE];

	int freeCluster = get_free_cluster(FAT);
	//Update In Memory FAT
	FAT[freeCluster] = EOC;

	//entry in root dir
	create_directory_record(newDirEntry, Name, 0x10, freeCluster);

	//entries to be created in the new directory
	create_directory_record(_firstEntry, ".", 0x10, freeCluster);
	create_directory_record(_secondEntry, "..", 0x10, curDirClusNum); //curDirClusNum is 0 for root directory


	write_to_fat_tables(freeCluster);
	write_to_cur_dir_cluster(newDirEntry, curDirClusNum, FAT);
	write_to_new_dir_cluster(_firstEntry, _secondEntry, freeCluster);
	
}

int main()
{

	disk = open("/dev/sdb1", O_RDWR);

	init_BPB();

	//Get FAT(file allocation table) into Memory
	int numberOfFATEntries = (bpb->FATSz32 * bpb->BytsPerSec) / FAT_RECORD_SIZE;
	int *FAT = calloc(numberOfFATEntries, sizeof(int));
	init_FAT32(FAT);
	//print_FAT32(FAT, numberOfFATEntries);


	struct DIRECTORY *DIR = NULL;
	char tab[100] = "";
	char name[8];


	int choice, clus;
	do
	{
		printf("===  MENU ===\n");
		printf("1. BPB Info\n");
		printf("2. Display Directories\n");
		printf("3. Create Directory\n");
		printf("4. Exit\n");
		printf("CHOICE ==> ");
		scanf("%d", &choice);
		switch(choice)
		{
			case 1:
				print_BPB();
				break;
			case 2:
				DIR = read_directory(bpb->RootClus, 0, FAT);
				//display files and directories
				printf("\n==========  FILE SYSTEM ===========\n\n");
				print_directory(DIR, tab);
				//free dir after printing
				break;
			case 3:
				printf("Enter Directory Name ==> ");
				scanf("%s", name);
				//fgets(name, 8, stdin);
				printf("Enter Cluster Number ==> ");
				scanf("%d", &clus);
				//My constraints
				//Names should be only of 8 characters long
				//Names should be only in Capital Letters
				create_directory(name, FAT, clus);
				printf("---- DIR CREATED ---\n");
				break;
			case 4:
				break;
			default:
				printf("Invalid Choice.\n");
		}

	} while(choice != 4);



	//DIR LIST TO BE CLEARED
	free(FAT);
	free(bpb);
	close(disk);

	return 0;
}