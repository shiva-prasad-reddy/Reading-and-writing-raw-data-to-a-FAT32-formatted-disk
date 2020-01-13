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

struct FSInfo {
	unsigned int Free_Count;
	unsigned int Nxt_Free;
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


	//readingPoint is used to skip these records in a directory
	// . --> points to the same directory
	// .. --> points to the parent directory
	for(int i = readingPoint; i < DirEntriesInACluster; i++)
	{
		//End Of Records
		if(buffer[32 * i] == 0x0) break;

		//Unused Record caused By Deletion
		if(buffer[32 * i] == 0xE5) continue;

		printf("%d  occupied\n", i);
		int attr = turn_to_integer(buffer + (32 * i), 11, 1);
		int X;
		printf("CHECK THESE ==> ");
		display_series_of_characters(buffer + (32 * i), 11);
		printf("\n");
		//LONG FILE NAME ENTRY
		if(attr == 0xF)
		{
			//Code to get the long file name
			//Read 2 Bytes at a time to get the character
			//1 -- 10
			for(int j = 1; j < 10; j += 2)
			{
				X = turn_to_integer(buffer + (32 * i), j, 2);
				if(X == 0xffff) break; //end of file name
				TEMP[++temp_stack] = X;
			}

			//14 -- 25
			for(int j = 14; j < 25; j += 2)
			{
				X = turn_to_integer(buffer + (32 * i), j, 2);
				if(X == 0xffff) break; //end of file name
				TEMP[++temp_stack] = X;
			}

			//28 -- 31
			for(int j = 28; j < 31; j += 2)
			{
				X = turn_to_integer(buffer + (32 * i), j, 2);
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
			temp = malloc(sizeof(struct DIRECTORY_ENTRY));
			temp->next = temp->dir = NULL;
			if(head == NULL) head = temp;
			else last->next = temp;
			last  = temp;
			int count = 0;
			for(int j = 0; j < 11; j++) temp->Name[count++] = buffer[ (32*i)+j ];
			//finds file name from above or below code
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

			if(attr == 0x10)
			{
				temp->dir = readDirecory(2, temp->StartCluster, disk, bpb);
			}
		}
		
	}

	free(buffer);
	return head;
}

//To display sectors allocated to a file or directory 
//but used to count the total sectors allocated
int displaySectorsAllocated(unsigned int *FAT, int start)
{
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
		printf("%sFileName = %s\n", tab, temp->Name);
		printf("%sFileSize = %ld Bytes\n", tab, temp->FileSize);
		printf("%sStartCluster = %d\n", tab, temp->StartCluster);
		printf("%sFileAttr = %x %s\n", tab, temp->Attr, (temp->Attr == 0x10 ? "(DIR)" : "(FILE)"));
		if(temp->StartCluster != 0) printf("%sTotalSectors = %d\n", tab, displaySectorsAllocated(FAT, temp->StartCluster));

		if(temp->dir != NULL)
		{
			char tab2[100];
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

void displayFSInfo(struct FSInfo *fsinfo)
{
	printf("\n+++  FSInfo +++\n");
	printf("Free Cluster Count = %d\n", fsinfo->Free_Count);
	printf("First Available free cluster = %d\n", fsinfo->Nxt_Free);
	printf("++++++++++++++\n");
}

void initializeFSInfo(int disk, struct FSInfo *fsinfo)
{
	unsigned char buffer[512];
	lseek(disk, 512, SEEK_SET);
	read(disk, buffer, 512);
	fsinfo->Free_Count = turn_to_integer(buffer, 488, 4);
	fsinfo->Nxt_Free = turn_to_integer(buffer, 492, 4);

	displayFSInfo(fsinfo);

}




void storeDirEntryInBuffer(char *buffer, char *Name, unsigned short int Attr, unsigned int N)
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

	buffer[28] = 0;
	buffer[29] = 0;
	buffer[30] = 0;
	buffer[31] = 0;
}

int findFreeCluster(int *FAT, int FATEntryCount)
{
	//I'm skipping first 10 and Last 20 entries
	for(int i = 10; i < (FATEntryCount - 20); i++)
		if(FAT[i] == 0)
			return i;
}




void createDir(int disk, char *Name, struct BPB *bpb, unsigned int *FAT, int FATEntryCount, int curDirClusNum)
{

	//code should be optimized for every directory and accessed byte wise or sector wise
	//and remove redundant buffers
	char buffer_1[32], buffer_2[32], buffer_3[32];

	int freeCluster = findFreeCluster(FAT, FATEntryCount);


	printf("Free Cluster = %d\n", freeCluster);
	//entry in root dir
	storeDirEntryInBuffer(buffer_1, Name, 0x10, freeCluster);

	printf("SEE THIS --> ");
	display_series_of_characters(buffer_1, 11);
	printf("\n");

	//entries in the created dir
	storeDirEntryInBuffer(buffer_2, ".", 0x10, freeCluster);
	storeDirEntryInBuffer(buffer_3, "..", 0x10, curDirClusNum); //curDirClusNum is 0 for root directory


	unsigned long int occupied = 268435455;
	unsigned char buffer_fat_entry[4];
	buffer_fat_entry[0] = (occupied) & 0xFF;
	buffer_fat_entry[1] = (occupied >> 8) & 0xFF;
	buffer_fat_entry[2] = (occupied >> 16) & 0xFF;
	buffer_fat_entry[3] = (occupied >> 24) & 0xFF;



	//make that freecluster entry in fat to occupied (both FAT's)
	lseek(disk, (bpb->RsvdSecCnt * bpb->BytsPerSec) + (freeCluster * 4), SEEK_SET);
	write(disk, buffer_fat_entry, 4);
	lseek(disk, bpb->RsvdSecCnt * bpb->BytsPerSec + (bpb->FATSz32 * bpb->BytsPerSec) + (freeCluster * 4), SEEK_SET);
	write(disk, buffer_fat_entry, 4);

	/*
	unsigned char *buffer = malloc((bpb->FATSz32 * bpb->BytsPerSec) * sizeof(unsigned char));
	read(disk, buffer, bpb->FATSz32 * bpb->BytsPerSec);
	int NumberOfFATEntries = (bpb->FATSz32 * bpb->BytsPerSec) / 4;
	
	*/

	//find a free entry in the curDir and copy the buffer_1







	int startSector = getFirstSectorOfCluster(curDirClusNum, bpb);
	//Root directory entries are of 32 Bytes
	//and their number of entries are limited to the size of cluster
	//and size is based on the FAT entries

	int DirEntriesInACluster = (bpb->SecPerClus * bpb->BytsPerSec) / 32;
	unsigned char *buffer = malloc((bpb->SecPerClus * bpb->BytsPerSec) * sizeof(unsigned char));

	lseek(disk, startSector * bpb->BytsPerSec, SEEK_SET);
	read(disk, buffer, (bpb->SecPerClus * bpb->BytsPerSec));

	//readingPoint is used to skip these records in a directory
	// . --> points to the same directory
	// .. --> points to the parent directory
	unsigned int fre;
	for(int i = 0; i < DirEntriesInACluster; i++)
	{
		//printf("Chekc this out == >");
		//display_series_of_characters(buffer + (32 * i), 11);
		//printf("\n");
		//End Of Records && Unused Record caused By Deletion
		if(buffer[32 * i] == 0x0 || buffer[32 * i] == 0xE5)
		{
			fre = i;
			break;
		}
	}

	printf("Free Entry = %d\n", fre);
	for(int i = 0; i < 32; i++)
	{
		buffer[(32 * fre) + i] = buffer_1[i];
	}
	
	lseek(disk, startSector * bpb->BytsPerSec, SEEK_SET);
	write(disk, buffer, (bpb->SecPerClus * bpb->BytsPerSec));






	//go that free cluster and copy 0 inside te whole cluster(not efficient)
	//and place the entries buffer_2 and buffer_3
	unsigned int X = getFirstSectorOfCluster(freeCluster, bpb);
	lseek(disk, X * bpb->BytsPerSec, SEEK_SET);
	//make all entries 0;
	unsigned char *cluster_buffer = calloc(bpb->SecPerClus * bpb->BytsPerSec, sizeof(unsigned char));
	//copy first two entries
	for(int i = 0; i < 32; i++)
	{
		cluster_buffer[i] = buffer_2[i];
		cluster_buffer[32+i] = buffer_3[i];
	}

	write(disk, cluster_buffer, bpb->SecPerClus * bpb->BytsPerSec);


	free(buffer);
	free(cluster_buffer);
	
}



int main()
{

	int disk = open("/dev/sdc1", O_RDWR);

	//BIOS parametric Block
	struct BPB bpb;
	initializeBPB(disk, &bpb);

	struct FSInfo fsinfo;
	initializeFSInfo(disk, &fsinfo);


	//Get FAT(file allocation table) into Memory
	int NumberOfFATEntries = (bpb.FATSz32 * bpb.BytsPerSec) / 4;
	unsigned int *FAT;
	FAT = (unsigned int *)calloc(NumberOfFATEntries, sizeof(unsigned int));
	initializeFAT32(disk, FAT, &bpb);

	
	/*printf("%d == %d\n",NumberOfFATEntries, 1988480);
	for(int i = 0; i < NumberOfFATEntries; i++)
	{
		printf("FatEntry[ %d ] = %x \n", i, FAT[i]);
	}
	*/

	struct DIRECTORY_ENTRY *DIR = readDirecory(0, bpb.RootClus, disk, &bpb);
	printf("==========  FILE SYSTEM ===========\n\n");
	char tab[100] = "";
	displayDIREntries(tab, DIR, FAT);



	//createDir(disk, "MACHO", &bpb, FAT, NumberOfFATEntries, bpb.RootClus);

	//DIR LIST TO BE CLEARED
	free(FAT);
	close(disk);

	return 0;
}