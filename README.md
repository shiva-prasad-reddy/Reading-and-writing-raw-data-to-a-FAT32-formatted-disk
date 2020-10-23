# Reading-and-writing-raw-data-to-a-FAT32-formatted-disk
A simple C program to read FAT tables and output the directory strcuture to the console and creating a new directory & files by writing to free FAT entries.
> Followed Microsoft FAT32 specification.

# Output
```sh
===  MENU ===
1. BPB Info
2. Display Directories
3. Create Directory
4. Exit
CHOICE ==> 1

+++  BIOS Parameter Block Info +++
BytsPerSec = 512
SecPerClus = 8
RsvdSecCnt = 1698
NumFATs = 2
RootEntCnt 0
TotSec32 = 15939943
FATSz32 = 15535
RootClus = 2
++++++


===  MENU ===
1. BPB Info
2. Display Directories
3. Create Directory
4. Exit
CHOICE ==> 2

==========  FILE SYSTEM ===========

FileName = folder_1
FileSize = 0 Bytes
StartCluster = 6
FileAttr = 10 (DIR)
	!
	!
	FileName = see
	FileSize = 0 Bytes
	StartCluster = 9
	FileAttr = 10 (DIR)
		!
		!
		FileName = RTY
		FileSize = 0 Bytes
		StartCluster = 14
		FileAttr = 10 (DIR)



FileName = one.tx
FileSize = 8 Bytes
StartCluster = 7
FileAttr = 20 (FILE)

FileName = CHUTKI     
FileSize = 0 Bytes
StartCluster = 8
FileAttr = 10 (DIR)

FileName = CHUI
FileSize = 0 Bytes
StartCluster = 10
FileAttr = 10 (DIR)

FileName = MACHO
FileSize = 0 Bytes
StartCluster = 11
FileAttr = 10 (DIR)

FileName = LINUX
FileSize = 0 Bytes
StartCluster = 12
FileAttr = 10 (DIR)

FileName = QWE
FileSize = 0 Bytes
StartCluster = 13
FileAttr = 10 (DIR)

===  MENU ===
1. BPB Info
2. Display Directories
3. Create Directory
4. Exit
CHOICE ==> 2

==========  FILE SYSTEM ===========

FileName = folder_1
FileSize = 0 Bytes
StartCluster = 6
FileAttr = 10 (DIR)
	!
	!
	FileName = see
	FileSize = 0 Bytes
	StartCluster = 9
	FileAttr = 10 (DIR)
		!
		!
		FileName = RTY
		FileSize = 0 Bytes
		StartCluster = 14
		FileAttr = 10 (DIR)



FileName = one.tx
FileSize = 8 Bytes
StartCluster = 7
FileAttr = 20 (FILE)

FileName = CHUTKI     
FileSize = 0 Bytes
StartCluster = 8
FileAttr = 10 (DIR)

FileName = CHUI
FileSize = 0 Bytes
StartCluster = 10
FileAttr = 10 (DIR)

FileName = MACHO
FileSize = 0 Bytes
StartCluster = 11
FileAttr = 10 (DIR)

FileName = LINUX
FileSize = 0 Bytes
StartCluster = 12
FileAttr = 10 (DIR)

FileName = QWE
FileSize = 0 Bytes
StartCluster = 13
FileAttr = 10 (DIR)

===  MENU ===
1. BPB Info
2. Display Directories
3. Create Directory
4. Exit
CHOICE ==> 3
Enter Directory Name ==> HELLO
Enter Cluster Number ==> 2
---- DIR CREATED ---
===  MENU ===
1. BPB Info
2. Display Directories
3. Create Directory
4. Exit
CHOICE ==> 2

==========  FILE SYSTEM ===========

FileName = folder_1
FileSize = 0 Bytes
StartCluster = 6
FileAttr = 10 (DIR)
	!
	!
	FileName = see
	FileSize = 0 Bytes
	StartCluster = 9
	FileAttr = 10 (DIR)
		!
		!
		FileName = RTY
		FileSize = 0 Bytes
		StartCluster = 14
		FileAttr = 10 (DIR)



FileName = one.tx
FileSize = 8 Bytes
StartCluster = 7
FileAttr = 20 (FILE)

FileName = CHUTKI     
FileSize = 0 Bytes
StartCluster = 8
FileAttr = 10 (DIR)

FileName = CHUI
FileSize = 0 Bytes
StartCluster = 10
FileAttr = 10 (DIR)

FileName = MACHO
FileSize = 0 Bytes
StartCluster = 11
FileAttr = 10 (DIR)

FileName = LINUX
FileSize = 0 Bytes
StartCluster = 12
FileAttr = 10 (DIR)

FileName = QWE
FileSize = 0 Bytes
StartCluster = 13
FileAttr = 10 (DIR)

FileName = HELLO
FileSize = 0 Bytes
StartCluster = 15
FileAttr = 10 (DIR)

===  MENU ===
1. BPB Info
2. Display Directories
3. Create Directory
4. Exit
CHOICE ==> 4
```
