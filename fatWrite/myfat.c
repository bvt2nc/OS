#include "myfat.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

int start = 0;
int bytesPerClus;
int rootDirSectors;
int firstDataSector;
int FATSz;
int numDataSec;
int totSec;
int numClusters; // number of DATA clusters
bpbFat32 bpb;
//int count = 0;
int fatType = 0;
uint32_t *fatTable;
//uint16_t fatTable[1000];
int EOC;
int cwdCluster; //current working directory cluster used throughout code to represent "ptr locaton"
FILE * fd;
char * dotName;
char * dotDotName;

void init();
char * makeUpper(char * path);
int firstClusterSector(int n);
void printDir(dirEnt dir);
void recurseThroughDir(FILE *fd, int offset);
unsigned int tableValue(int cluster);
void readFatTable(FILE *fd);
int clusterSize(int cluster, int size);
unsigned int * clusterChain(int cluster);
char * dirName(dirEnt dir, int file);
int cdAbsolute(const char * path);
dirEnt * openDir;
int * openDirOffset;
int * tempOpenDirOffset;
int opening;
int findEmptyCluster();
dirEnt writeDir(dirEnt dir, char * path, int cluster, int isDir);
int createFile(const char * path, int isDir);
char * goToEndOfFilePath(const char * path);
int removeFile(const char * path, int isDir);

int main() {

	//Sandbox for testing functions

	init();

    /*OS_cd("PEOPLE");
    OS_readDir(".");
    OS_cd("ABK2Y");
    OS_readDir(".");
    OS_cd("/PEOPLE/ABK2Y");
    OS_cd("~");
    OS_readDir(".");
    OS_readDir("/PEOPLE/ABK2Y");
    OS_open("CONGRATSTXT");
    OS_cd("MEDIA");
    OS_open("EPAA22~1JPG");
    OS_cd("../PEOPLE");*/
    //findEmptyCluster();
    OS_cd("media");
	int status = OS_mkdir("test2");
	printf("mkdir status: %d \n", status);
	status = OS_rmdir("test2");
	printf("rm status: %d \n", status);    
	dirEnt * currentDirs = OS_readDir(".");
    int i;
    for(i = 0; i < 128; i++)
    {
    	if(currentDirs[i].dir_name[0] == 0x00)
    		break;

    	printDir(currentDirs[i]);
    }
    printf("==========================================\n");
    /*currentDirs = OS_readDir("TEST1");
    for(i = 0; i < 128; i++)
    {
    	if(currentDirs[i].dir_name[0] == 0x00)
    		break;

    	printDir(currentDirs[i]);
    }*/
    status = OS_open("~/media/epaa22~1.jpg");
    printf("open status: %d \n", status);
    /*init();
    //readFatTable(fd);
    recurseThroughDir(fd, firstClusterSector(2) * bpb.bpb_bytesPerSec);*/

    return 0;
}

//Load the disk and set all the globals variables
//Loads bpb and FAT table into memory
//Allocates memory to global arrays used by core functions
void init()
{
    //fd = fopen("sampledisk32.raw", "rb+");
	//If you don't want to do the below, uncomment above statement
	//Assumes sampledisk32.raw is in the same directory as code
    fd = fopen(getenv("FAT_FS_PATH"), "rb+"); //get the env var FAT_FS_PATH
	//Assuming it has been set though...

    fseek(fd, 0, SEEK_SET);
	fread(&bpb, sizeof(bpbFat32), 1, fd);
    
    if(bpb.bpb_FATSz16 != 0)
    {
    	FATSz = bpb.bpb_FATSz16;
    	totSec = bpb.bpb_totSec16;
    	fatType = 16;
    }
   	else
   	{
   		FATSz = bpb.bpb_FATSz32;
   		totSec = bpb.bpb_totSec32;
   		fatType = 32;
   	}

    bytesPerClus = bpb.bpb_bytesPerSec * bpb.bpb_secPerClus;
    rootDirSectors = ((bpb.bpb_rootEntCnt * 32) + (bpb.bpb_bytesPerSec - 1)) / bpb.bpb_bytesPerSec;
   	firstDataSector = bpb.bpb_rsvdSecCnt + (bpb.bpb_numFATs * FATSz) + rootDirSectors;
   	numDataSec = totSec - (bpb.bpb_rsvdSecCnt + (bpb.bpb_numFATs * FATSz) + rootDirSectors);
   	numClusters = numDataSec / bpb.bpb_secPerClus;
   	openDir = (dirEnt*)malloc(sizeof(dirEnt) * 128);
   	openDirOffset = (int*)malloc(sizeof(int) * 128);
   	tempOpenDirOffset = (int*)malloc(sizeof(int) * 128);
   	opening = 0;

   	int i;
   	dotName = (char *)malloc(sizeof(char) * 11);
   	dotName[0] = '.';
   	for(i = 1; i < 11; i++)
   		dotName[i] = ' ';

   	dotDotName = (char *)malloc(sizeof(char) * 11);
   	dotDotName[0] = '.';
   	dotDotName[1] = '.';
   	for(i = 2; i < 11; i++)
   		dotDotName[i] = ' ';

   	//if init has been read, start = 1
   	//otherwise start = 0
   	start = 1;
   	cwdCluster = 2; //default to root directory
    readFatTable(fd);
   	
   	/*printf("end init \n");
   	dirEnt *dir = (dirEnt*)malloc(numClusters * bytesPerClus);
   	printf("reserved sector count: %d \n", bpb.bpb_rsvdSecCnt);
   	printf("fatsz: %d \n", FATSz);
   	printf("bytes per sec: %d \n", bpb.bpb_bytesPerSec);
   	printf("sec per cluster: %d \n", bpb.bpb_secPerClus);
   	printf("dirEnt size: %d \n", sizeof(dirEnt));

   	int offset = firstClusterSector(2) * bpb.bpb_bytesPerSec;
   	readFatTable(fd);
   	printf("FAT[0]: 0x%x \n", tableValue(0));
	printf("FAT[1]: 0x%x \n", tableValue(1));
   	EOC = 0x0FFFFFFF;
   	printf("EOC: 0x%x \n", EOC);
   	recurseThroughDir(fd, offset);*/

	/*char *c = "congrats.txt";
	printf("%s \n", c);
	char* p = c;
	while (*p)
	{
	    *p++;
	    printf("%d\n", strtoul(p, &p, 10));
	}
	//printf("%d \n", OS_cd(c));
	printf("Count: %d \n", count);*/
}

//Test function that recurses through each directory and their subdirectory
//Prints each directory and file info including cluster information
void recurseThroughDir(FILE * fd, int offset)
{
	int inc;
   	dirEnt dir;
   	for(inc = 0; inc < bytesPerClus - 64; inc += 32)
   	{
	   	fseek(fd, offset + inc, SEEK_SET);
	   	fread(&dir, sizeof(dirEnt), 1, fd);
	   	if(dir.dir_name[0] == 0x00)
	   		break;
	   	if(dir.dir_name[0] == 0xE5)
	   		continue;
	   	if(dir.dir_attr == 8 || dir.dir_attr == 15)
	   		continue;
		printf("===================================\n");
	   	printDir(dir);
	   	if(dir.dir_attr == 16)
	   	{
	   		//printf("===================================\n");
	   		//printf("recurring...\n");
	   		//printf("FAT table value: 0x%x\n", tableValue(dir.dir_fstClusLO));
	   		recurseThroughDir(fd, (firstClusterSector(dir.dir_fstClusLO) * bpb.bpb_bytesPerSec) + 64);
	   	}
	   	else
	   	{
	   		//unsigned int * chain = clusterChain(dir.dir_fstClusLO);
	   	}
	   	//printf("===================================\n");
	}
}

char * makeUpper(char * path)
{
	int i;
	char * ret = malloc(sizeof(char) * strlen(path));
	for(i = 0; i < strlen(path); i++)
	{
		ret[i] = toupper((char) path[i]);
	}
	return ret;
}

//As described in the FAT spec, returns the first sector of the specified cluster
//relative to the entire disk
int firstClusterSector(int n)
{
	return ((n - 2) * bpb.bpb_secPerClus) + firstDataSector;
}

//Print directory and file meta data
void printDir(dirEnt dir)
{
	int i, attr;
	attr = dir.dir_attr;
	char c;
	//Parse the filename
   	for(i = 0; i < 11; i++)
   	{
   		if(attr != 16 && attr != 8 && i == 8)
   			printf(".");
   		c = dir.dir_name[i];
   		printf("%c", c);
   	}
   	printf("\n");

   	printf("Attributes: %d \n", attr);
   	printf("NTRes: %d \n", dir.dir_NTRes);
   	unsigned int * chain = clusterChain(dir.dir_fstClusLO);
   	printf("First Cluster High: 0x%x \n", dir.dir_fstClusHI);
   	printf("First Cluster Low: 0x%x \n", dir.dir_fstClusLO);
   	printf("First Cluster Table Value: 0x%x \n", tableValue(dir.dir_fstClusLO));
   	if(dir.dir_fstClusLO < EOC)
   		printf("Next Cluster Table Value: 0x%x \n", tableValue(tableValue(dir.dir_fstClusLO)));
}

//returns the value at fatTable[cluster]
unsigned int tableValue(int cluster)
{
	/*unsigned int fatOffset;
	if(fatType == 16)
		fatOffset = cluster * 2;
	else
		fatOffset = cluster * 4;
	
	unsigned int fatSector = (fatOffset / bpb.bpb_bytesPerSec);
	unsigned int fatEntOffset = fatOffset % bpb.bpb_bytesPerSec;*/

	//Above code is actually irrelevant due to how the FAT table was read into memory
	//Code is there as those are the calculations specified by the FAT spec
	return *(unsigned int*)&fatTable[cluster] & 0x0FFFFFFF;
}

//Helper function
//Returns the length of a cluster chain
//Lazy workaround
//Should make the cluster chain into a struct that includes the chain and the length
int clusterChainSize(int cluster, int size)
{
	int value = tableValue(cluster);
	size++;

	if(value >= 0x0FFFFFF8)
	{
		return size;
	}

	return clusterChainSize(value, size);
}

//Returns an array of the cluster chain for a file
unsigned int * clusterChain(int cluster)
{
	int value = tableValue(cluster);
	int size = clusterChainSize(cluster, 0);
	unsigned int * chain = (unsigned int*)malloc(sizeof(unsigned int) * size);
	chain[0] = cluster;

	int i;
	for(i = 1; i < size; i++)
	{
		chain[i] = tableValue(chain[i - 1]);
		//printf("0x%x ", chain[i]);
	}
	//printf("\n");

	return chain;

}

//Read FAT table into memory
void readFatTable(FILE * fd)
{
	if(fatType == 32)
	{
		fatTable = (uint32_t *)malloc(FATSz * bpb.bpb_bytesPerSec);
		fseek(fd, bpb.bpb_rsvdSecCnt * bpb.bpb_bytesPerSec, SEEK_SET);
		fread(fatTable, sizeof(uint32_t), FATSz * bpb.bpb_bytesPerSec / sizeof(uint32_t), fd);
	}
	else if(fatType == 16)
	{
		fatTable = (uint32_t *)malloc(FATSz * bpb.bpb_bytesPerSec);
		fseek(fd, bpb.bpb_rsvdSecCnt * bpb.bpb_bytesPerSec, SEEK_SET);
		fread(fatTable, sizeof(uint32_t), FATSz * bpb.bpb_bytesPerSec / sizeof(uint32_t), fd);
	}

	printf("there's a problem...\n");
	/*
	printf("done reading \n");
	int i;
	printf("length: %d \n", sizeof(fatTable));
	for(i = 0; i < 1000; i++)
	{
		printf("FAT[%d]: 0x%x \n", i, *(unsigned int*)&fatTable[i] & 0x0FFFFFFF);
	}*/
}

//Changes working directory to path
//Path must either be an absolute path starting at the root directory
//or must be a relative path only to an immediate folder
//Use '~' to go back to the root directory
//Use .. to go to back to parent directory
//
//EX:	cd path
//		cd /PEOPLE/BVT2NC 		GOOD
//		cd PEOPLE				GOOD
//		cd ~					GOOD
//		cd ~/PEOPLE/BVT2NC		NOT IMPLEMENTED
int OS_cd(const char *path)
{	
	if(start == 0)
		init();

	path = makeUpper((char *)path);

	if(path[0] == '/' && path[1] == 0)
	{
		cwdCluster = 2;
		return 1;
	}

	int tempCWD = cwdCluster;
	int status;
	//If path is an absolute path starting at the root directory
	if(path[0] == '/')
	{
		cwdCluster = 2;
		status = cdAbsolute(path);
		if(status == -1)
		{
			cwdCluster = tempCWD;
			return -1;
		}
	}

	//If it is a long relative path leading to other directories
	if(strstr(path, "/"))
	{
		return cdAbsolute(path);
	}

	if(path[0] == '~')
	{
		cwdCluster = 2;
		return 1;
	}

	dirEnt * lsDir = OS_readDir("."); //Get all the directories in the cwd
	dirEnt dir;
	int i, compare, j;
	int terminate = 0;
	char * dirNamee;
	char * realPath = (char *)malloc(sizeof(char) * 8);

	//reparse string passed in the arguement 'path' to manipulate characters after null terminator 
	//EX:
	//Assume MEDIA = 1 2 3 4 5
	//Passing MEDIA to path, it is possible that path = 1 2 3 4 5 0 45 22
	//This for loop will change make realPath = 1 2 3 4 5 32 32 32 
	//To fit what we will read from dir.dir_name
	for(i = 0; i < 8; i++)
	{
		if(terminate == 1)
		{
			realPath[i] = 32;
			continue;
		}

		if(path[i] == 0)
		{
			realPath[i] = 32;
			terminate = 1;
		}
		else
			realPath[i] = path[i];
	}

	//Parse through all directories in cwd
	for(i = 0; i < 128; i++)
	{
		dir = lsDir[i];
		if(dir.dir_name[0] == 0) //If last entry
			break;

		dirNamee = dirName(dir, 0); //bad name... was a quick fix due to conflicting variable and function name
		compare = strcmp(realPath, dirNamee);
		//printf("compare: %d \n", compare); 
		if(compare == 0)
		{
			//printf("match! \n");
			cwdCluster = dir.dir_fstClusLO;

			//In the event we go back to the root directory
			if(cwdCluster == 0)
				cwdCluster = 2;

			return 1;
		}

	}

	return -1;
}

//Helper function to change to cwd to an absolute path
int cdAbsolute(const char * path)
{
	path = makeUpper((char *)path);

	int i, status;
	char * subPath = (char *)malloc(sizeof(char) * 8);
	int index = 0;

	//Parse through path by finding the 'subpath' (.../*THIS_IS_THE_SUBPATH*/...)
	for(i = 0; i < strlen(path); i++)
	{
		if(path[i] == '/') //reached beginning/end of subpath
		{
			//printf("subPath: %s \n", subPath);
			status = OS_cd(subPath);
			index = 0;
			//We are only looking for directories so we don't care about ext
			subPath = (char *)malloc(sizeof(char) * 8);

			if(status == -1 && i != 0)
			{
				return -1;
			}
		}
		else //keep finding more of the subpath
		{
			subPath[index] = path[i];
			index++;
			//printf("%s \n", subPath);
		}
	}

	status = OS_cd(subPath);
	if(status == -1)
		return -1;

	return 1;
}

//Returns the directory name in a compareable format 
//Will always be 8 bytes long padded with 32s (inherently as that is what is read from the FAT disk)
char * dirName(dirEnt dir, int file)
{
	int i;
	int max = 8;
	if(file == 1)
		max = 11;

	char * str = (char *)malloc(sizeof(char) * 8);

   	for(i = 0; i < max; i++)
   	{
   		str[i] = dir.dir_name[i];
   		//printf("%d ", str[i]);
   	}
   	return str;
}

//Functions in the exact same way as ls
//Path must either be an absolute path starting at the root directory
//or must be a relative path only to an immediate folder
//Use '~' to go back to the root directory
//By default (if dirname is left blank) the shell will make dirname = "."
//
//EX:	ls dirname
//		ls /PEOPLE/BVT2NC 		GOOD
//		ls PEOPLE				GOOD
//		ls ~					GOOD
//		ls ~/PEOPLE/BVT2NC		NOT IMPLEMENTED
dirEnt * OS_readDir(const char *dirname)
{
	if(start == 0)
		init();

	dirname = makeUpper((char *)dirname);
	dirEnt* ls = (dirEnt*)malloc(sizeof(dirEnt) * 256);

	int tempCWD = 0;
	int status = 0;
	//If absolute path, save the cwdcluster to be loaded back again at end of function and cd
	if(dirname[0] != '.')
	{
		tempCWD = cwdCluster;
		//printf("tempCWD: %d \n", tempCWD);
		status = OS_cd(dirname);
		if(status == -1)
		{
			cwdCluster = tempCWD;
			return ls;
		}
	}

	int inc, offset;
	int count = 0;
   	dirEnt dir;
   	//printf("cwdCluster %d \n", cwdCluster);
   	offset = firstClusterSector(cwdCluster) * bpb.bpb_bytesPerSec;

   	//Loop through each entry (32 bytes long)
   	for(inc = 0; inc < bytesPerClus ; inc += 32)
   	{
	   	fseek(fd, offset + inc, SEEK_SET);
	   	fread(&dir, sizeof(dirEnt), 1, fd);
	   	if(dir.dir_name[0] == 0x00) //last entry
	   		break;
	   	if(dir.dir_name[0] == 0xE5) 
	   		continue;
	   	if(dir.dir_attr == 8 || dir.dir_attr == 15) //special case
	   		continue;

	   	ls[count] = dir;
	   	if(opening == 1)
		   	tempOpenDirOffset[count] = offset + inc; 
	   	count++;
	   	//printDir(dir);
	}

	//If there is a cluster chain for the directory
	dirEnt cwd;
	offset = firstClusterSector(cwdCluster) * bpb.bpb_bytesPerSec;
	fseek(fd, offset, SEEK_SET);
	fread(&cwd, sizeof(dirEnt), 1, fd);

	int length = clusterChainSize(cwd.dir_fstClusLO, 0);
	if(length > 1)
	{
		inc = 0;
		unsigned int * chain = clusterChain(cwd.dir_fstClusLO);
		offset = firstClusterSector(chain[1]) * bpb.bpb_bytesPerSec;
	   	//Loop through each entry (32 bytes long)
	   	for(inc = 0; inc < bytesPerClus ; inc += 32)
	   	{
		   	fseek(fd, offset + inc, SEEK_SET);
		   	fread(&dir, sizeof(dirEnt), 1, fd);
		   	//printf("first character: %d \n", dir.dir_name[0]);
		   	if(dir.dir_name[0] == 0x00) //last entry
		   		break;
		   	if(dir.dir_name[0] == 0xE5) 
		   		continue;
		   	if(dir.dir_attr == 8 || dir.dir_attr == 15) //special case
		   		continue;

		   	//printDir(dir);
		   	//printf("========================\n");
		   	ls[count] = dir;
		   	if(opening == 1)
		   		tempOpenDirOffset[count] = offset + inc; 
		   	count++;
		   	//printDir(dir);
		}
	}

	//If absolute path was given, set cwdCluster to original
	if(tempCWD != 0)
		cwdCluster = tempCWD;

	//printf("after cwd: %d \n", cwdCluster);

	return ls;
}

//Loads specified file in 'path' as a dirEnt to the global 'openDir'
int OS_open(const char *path)
{
	if(start == 0)
		init();

	path = makeUpper((char *)path);

	int tempCWD = cwdCluster;
	opening = 1;
	tempOpenDirOffset = (int*)malloc(sizeof(int) * 128);

	//If long relative path (leading to ther directories other than current)
	if(strstr(path, "/"))
	{
		int i, status;
		char * subPath = (char *)malloc(sizeof(char) * 8);
		int index = 0;

		//Parse through path by finding the 'subpath' (.../*THIS_IS_THE_SUBPATH*/...)
		for(i = 0; i < strlen(path); i++)
		{
			if(path[i] == '/') //reached beginning/end of subpath
			{
				//printf("subPath: %s \n", subPath);
				status = OS_cd(subPath);
				index = 0;
				//We are only looking for directories so we don't care about ext
				subPath = (char *)malloc(sizeof(char) * 8);
			}
			else //keep finding more of the subpath
			{
				subPath[index] = path[i];
				index++;
				//printf("%s \n", subPath);
			}
		}

		path = subPath;
		//printf("%s \n", path);
	}

	dirEnt * dir = OS_readDir(".");
	int i, j;
	int terminate = 0;
	opening = 0;

	char * realPath = (char *)malloc(sizeof(char) * 11);
	int index = 0;
	int extFound = 0;
	char * ext = (char *)malloc(sizeof(char) * 3);

	//See cd
	//tldr; Reparse the path to get rid of garbage at the end if the short name is smaller than the max allowed
	for(i = 0; i < 12; i++)
	{
		if(index == 11)
			break;

		if(path[i] == '.')
		{
			if(i > 8)
			{
				cwdCluster = tempCWD;
				return -1;
			}
			extFound = 1;
			ext[0] = path[i + 1];
			ext[1] = path[i + 2];
			ext[2] = path[i + 3];
			continue;
		}

		if(extFound == 1 && i < 9)
		{
			realPath[index] = 32;
			index++;
			continue;
		}

		if(extFound == 1)
		{
			realPath[index] = ext[0];
			realPath[index + 1] = ext[1];
			realPath[index + 2] = ext[2];
			break;
		}

		if(terminate == 1)
		{
			realPath[index] = 32;
			index++;
			continue;
		}
		if(path[i] == 0)
		{
			realPath[index] = 32;
			index++;
			terminate = 1;
		}
		else
		{
			realPath[index] = path[i];
			index++;
		}
	}

	//Loop through all the directories in cwd
	for(i = 0; i < 128; i++)
	{
		if(dir[i].dir_name[0] == 0x00)
		{
			break;
		}

		//printf("dirName: %s \n", dirName(dir[i], 1));
		if(strcmp(realPath, dirName(dir[i], 1)) == 0)
		{

			//Linear search through the global 'openDir' for first open space
			for(j = 0; j < 128; j++)
			{
				if(openDir[j].dir_name[0] == 0x00)
				{
					openDir[j] = dir[i];
					openDirOffset[j] = tempOpenDirOffset[i];
					cwdCluster = tempCWD;
					return j;
				}
			}
			//If code gets here, then there were no empty spaces left in the array
			//That means user attempted to load 128 files into memory without freeing up space
			//which isn't a good idea anyway

			printf("Too many files opened. Please close a file\n");
			break;
		}
	}

	cwdCluster = tempCWD;
	return -1;
}

//Replaces openDir[fd] with a blank dir that can be used to load new files into memory
int OS_close(int fd)
{
	if(start == 0)
		init();

	dirEnt dir = openDir[fd];
	if(dir.dir_name[0] == 0x00) //If it is already empty, nothing to do
		return -1;
	dirEnt *emptyDir = (dirEnt*)malloc(sizeof(dirEnt)); //empty dirEnt
	openDir[fd] = *emptyDir;
	openDirOffset[fd] = 0;

	return 1;
}

//Reads the file specifed by the file descriptor fildes
//The file to be read has its dirEnt stored in openDir
//The file MUST HAVE BEEN PREVIOUSLY OPENED USING OS_open!!!!!
//
//Reads the file starting at the offset and up to nbytes into buf
int OS_read(int fildes, void *buf, int nbyte, int offset)
{
	if(start == 0)
		init();
	
	dirEnt dir = openDir[fildes];
	if(dir.dir_name[0] == 0x00) //If file location is empty, throw shell error
		return -1;

	//Get the cluster chain and its length
	unsigned int * chain = clusterChain(dir.dir_fstClusLO);
	int length = clusterChainSize(dir.dir_fstClusLO, 0);

	//If the offset or nbytes to be read is larger than the size (in bytes)
	//the cluster link occupies, send signal to throw error
	if(nbyte + offset > dir.dir_fileSize)
	{
		printf("dir file size: %d \n", dir.dir_fileSize);
		return -1;
	}

	int bytesRead = 0;
	int firstCluster = offset / bytesPerClus;
	int firstClusterOffset = offset % bytesPerClus;
	int bytesToRead = nbyte; //var used to hold specifically how many bytes to read

	//If there are more bytes to read than what a cluster holds, only read
	//up to the end of the cluster
	if(nbyte > bytesPerClus - firstClusterOffset)
		bytesToRead = bytesPerClus - firstClusterOffset;

	/*printf("firstCluster: %d \n", firstCluster);
	printf("firstClusterOffset: %d \n", firstClusterOffset);
	printf("bytesToRead: %d \n", bytesToRead);
	printf("firstChainCluster: 0x%x \n", (int)chain[firstCluster]);*/

	//Read the data at the offset of the first cluster (after adding in the offset) relative to the file
	fseek(fd, (firstClusterSector((int)chain[firstCluster]) * bpb.bpb_bytesPerSec) + firstClusterOffset, SEEK_SET);
	fread(buf, bytesToRead, 1, fd);
	//Increment local variables to continue reading (if needed) the next cluster(s) in the chain
	bytesRead += bytesToRead;
	firstCluster++;
	bytesToRead = nbyte - bytesRead;

	//while there are still bytes left to be read
	while(bytesRead < nbyte || bytesToRead > 0)
	{
		//Make sure we aren't reading in more bytes there are in a cluster again
		if(bytesToRead > bytesPerClus)
		{
			bytesToRead = bytesPerClus;
		}

		//read the data of that cluster into buffer
		fseek(fd, firstClusterSector((int)chain[firstCluster]) * bpb.bpb_bytesPerSec, SEEK_SET);
   		fread(buf + bytesRead, bytesToRead, 1, fd);
   		bytesRead += bytesToRead;
   		bytesToRead = nbyte - bytesRead;
   		firstCluster++;
	}

   	return nbyte;
}




//========================================================WRITE=================================================

int findEmptyCluster()
{

	int size = FATSz * bpb.bpb_bytesPerSec / sizeof(uint32_t);
	int i;

	for(i = 0; i < size; i++)
	{
		if(tableValue(i) == 0x0)
		{
			//printf("Free cluster at: 0x%x \n", i);
			return i;
		}
	}

	return -1;
}

char * goToEndOfFilePath(const char *path)
{
	int i, status;
	char * subPath = (char *)malloc(sizeof(char) * 11);
	int index = 0;

	if(path[0] == '/')
		cwdCluster = 2;

	//Parse through path by finding the 'subpath' (.../*THIS_IS_THE_SUBPATH*/...)
	for(i = 0; i < strlen(path); i++)
	{
		if(path[i] == '/') //reached beginning/end of subpath
		{
			status = OS_cd(subPath);

			if(status == -1 && i != 0)
			{
				subPath[0] = -1;
				return subPath;
			}

			index = 0;
			subPath = (char *)malloc(sizeof(char) * 11);
		}
		else //keep finding more of the subpath
		{
			subPath[index] = path[i];
			index++;
			//printf("%s \n", subPath);
		}
	}

	return subPath;
}

int OS_rmdir(const char *path)
{
	return removeFile(path, 1);
}
int OS_mkdir(const char *path)
{
	return createFile(path, 1);
}

int createFile(const char *path, int isDir)
{
	if(start == 0)
		init();

	path = makeUpper((char *)path);

	//save originalPath in case we need to cluster chain
	int i;
	char * originalPath = (char*)malloc(sizeof(char) * strlen(path));
	for(i = 0; i < strlen(path); i++)
	{
		originalPath[i] = path[i];
	}

	int terminate = 0;
	int tempCWD = cwdCluster;
	//If long relative path (leading to ther directories other than current)
	if(strstr(path, "/"))
	{
		path = goToEndOfFilePath(path);
		if(path[0] == -1)
		{
			cwdCluster = tempCWD;
			return -1;
		}
	}

	int len = 11;
	int parseLen = 12;
	if(isDir == 1)
	{	
		len = 8;
		parseLen = 8;
	}

	char * realPath = (char *)malloc(sizeof(char) * len);
	int index = 0;
	int extFound = 0;
	char * ext = (char *)malloc(sizeof(char) * 3);

	//See cd
	//tldr; Reparse the path to get rid of garbage at the end if the short name is smaller than the max allowed
	for(i = 0; i < parseLen; i++)
	{
		if(index == len)
			break;

		if(path[i] == '.')
		{
			if(i > 8)
			{
				cwdCluster = tempCWD;
				return -1;
			}
			extFound = 1;
			ext[0] = path[i + 1];
			ext[1] = path[i + 2];
			ext[2] = path[i + 3];
			continue;
		}

		if(extFound == 1 && i < 9)
		{
			realPath[index] = 32;
			index++;
			continue;
		}

		if(extFound == 1)
		{
			realPath[index] = ext[0];
			realPath[index + 1] = ext[1];
			realPath[index + 2] = ext[2];
			break;
		}

		if(terminate == 1)
		{
			realPath[index] = 32;
			index++;
			continue;
		}
		if(path[i] == 0)
		{
			realPath[index] = 32;
			index++;
			terminate = 1;
		}
		else
		{
			realPath[index] = path[i];
			index++;
		}
	}

	int inc, offset, cmp, iDirName;
   	dirEnt dir, cwd;
   	int emptyCluster = findEmptyCluster();
   	offset = firstClusterSector(cwdCluster) * bpb.bpb_bytesPerSec;
	fseek(fd, offset, SEEK_SET);
	fread(&cwd, sizeof(dirEnt), 1, fd);
	int length = clusterChainSize(cwd.dir_fstClusLO, 0);
	unsigned int * chain = clusterChain(cwd.dir_fstClusLO);
	if(chain[0] == 0)
		chain[0] = 2;

	for(i = 0; i < length; i++)
	{
		offset = firstClusterSector(chain[i]) * bpb.bpb_bytesPerSec;
		//Loop through each entry (32 bytes long)
	   	for(inc = 0; inc < bytesPerClus ; inc += 32)
	   	{
		   	fseek(fd, offset + inc, SEEK_SET);
		   	fread(&dir, sizeof(dirEnt), 1, fd);
		   	cmp = strcmp(realPath, dirName(dir, !isDir));
		   	if(cmp == 0) //Directory already exists
		   	{
		   		cwdCluster = tempCWD;
		   		return -2;
		   	}

		   	if(dir.dir_name[0] == 0xE5 || dir.dir_name[0] == 0x00) 
		   	{
		   		dir = writeDir(dir, realPath, emptyCluster, isDir);
		   		fseek(fd, offset + inc, SEEK_SET);
		   		fwrite(&dir, sizeof(dirEnt), 1, fd);

		   		if(isDir)
		   		{
		   			offset = firstClusterSector(dir.dir_fstClusLO) * bpb.bpb_bytesPerSec;
		   			for(iDirName = 0; iDirName < 11; iDirName++)
		   				dir.dir_name[iDirName] = dotName[iDirName];
		   			fseek(fd, offset, SEEK_SET);
		   			fwrite(&dir, sizeof(dirEnt), 1, fd);
		   			for(iDirName = 0; iDirName < 11; iDirName++)
		   				dir.dir_name[iDirName] = dotDotName[iDirName];
		   			dir.dir_fstClusLO = cwdCluster;
		   			fseek(fd, offset + 32, SEEK_SET);
		   			fwrite(&dir, sizeof(dirEnt), 1, fd);
		   		}

		   		cwdCluster = tempCWD;
		   		return 1;
		   	}
		}
	}

	//There is no space left... need to cluster chain
	int nextEmptyCluster = findEmptyCluster();
	fatTable[chain[length - 1]] = emptyCluster;
	fatTable[nextEmptyCluster] = 0xFFFFFFF;
   	fseek(fd, bpb.bpb_rsvdSecCnt * bpb.bpb_bytesPerSec, SEEK_SET);
	fwrite(fatTable, sizeof(uint32_t), FATSz * bpb.bpb_bytesPerSec / sizeof(uint32_t), fd);

	cwdCluster = tempCWD;
	return createFile(originalPath, isDir);
}

dirEnt writeDir(dirEnt dir, char* path, int cluster, int isDir)
{
	int nameLen = 8;
	dir.dir_attr = 0x10;
	char * ext = (char *)malloc(sizeof(char) * 3);
	if(isDir == 0) //writing a file
	{
		nameLen = 11;
		dir.dir_attr = 0x00;
	}

	int i;
	for(i = 0; i < 11; i++)
	{
		if(i >= nameLen)
		{
			dir.dir_name[i] = 32;
			continue;
		}
		dir.dir_name[i] = path[i];
	}

	dir.dir_NTRes = 0;
	dir.dir_fstClusHI = 0x00;
	dir.dir_fstClusLO = cluster;
	dir.dir_fileSize = 0;

	fatTable[cluster] = 0xFFFFFFF;
	fseek(fd, bpb.bpb_rsvdSecCnt * bpb.bpb_bytesPerSec, SEEK_SET);
	fwrite(fatTable, sizeof(uint32_t), FATSz * bpb.bpb_bytesPerSec / sizeof(uint32_t), fd);

	return dir;
}

int removeFile(const char * path, int isDir)
{
	if(start == 0)
		init();

	path = makeUpper((char *)path);

	int i;
	int terminate = 0;
	int tempCWD = cwdCluster;
	//If long relative path (leading to ther directories other than current)
	if(strstr(path, "/"))
	{
		path = goToEndOfFilePath(path);
		if(path[0] == -1)
		{
			cwdCluster = tempCWD;
			return -1;
		}
	}

	char * realPath = (char *)malloc(sizeof(char) * 11);
	int index = 0;
	int extFound = 0;
	char * ext = (char *)malloc(sizeof(char) * 3);

	//See cd
	//tldr; Reparse the path to get rid of garbage at the end if the short name is smaller than the max allowed
	for(i = 0; i < 12; i++)
	{
		if(index == 11)
			break;

		if(path[i] == '.')
		{
			if(i > 8)
			{
				cwdCluster = tempCWD;
				return -1;
			}
			extFound = 1;
			ext[0] = path[i + 1];
			ext[1] = path[i + 2];
			ext[2] = path[i + 3];
			continue;
		}

		if(extFound == 1 && i < 9)
		{
			realPath[index] = 32;
			index++;
			continue;
		}

		if(extFound == 1)
		{
			realPath[index] = ext[0];
			realPath[index + 1] = ext[1];
			realPath[index + 2] = ext[2];
			break;
		}

		if(terminate == 1)
		{
			realPath[index] = 32;
			index++;
			continue;
		}
		if(path[i] == 0)
		{
			realPath[index] = 32;
			index++;
			terminate = 1;
		}
		else
		{
			realPath[index] = path[i];
			index++;
		}
	}

	int inc, offset, clusterOffset, cmp;
   	dirEnt dir, cwd;
   	int emptyCluster = findEmptyCluster();
   	unsigned int * fileChain;
   	int fileChainSize;
   	dirEnt *emptyDir; //empty dirEnt
   	offset = firstClusterSector(cwdCluster) * bpb.bpb_bytesPerSec;
	fseek(fd, offset, SEEK_SET);
	fread(&cwd, sizeof(dirEnt), 1, fd);
	int length = clusterChainSize(cwd.dir_fstClusLO, 0);
	unsigned int * chain = clusterChain(cwd.dir_fstClusLO);
	if(chain[0] == 0)
		chain[0] = 2;

	for(i = 0; i < length; i++)
	{
		offset = firstClusterSector(chain[i]) * bpb.bpb_bytesPerSec;

	   	//Loop through each entry (32 bytes long)
	   	for(inc = 0; inc < bytesPerClus ; inc += 32)
	   	{
		   	fseek(fd, offset + inc, SEEK_SET);
		   	fread(&dir, sizeof(dirEnt), 1, fd);
		   	cmp = strcmp(realPath, dirName(dir, 1));
		   	if(cmp == 0) //File Found!
		   	{
		   		cwdCluster = tempCWD;

		   		if(dir.dir_attr == 0x10) // is a directory
		   		{
		   			if(isDir == 0) //we specified we were removing a file
		   				return -2;
		   		}
		   		if(isDir && dir.dir_attr != 0x10) //specified we were removing a dir and it is not a dir
		   			return -2;

		   		fileChainSize = clusterChainSize(dir.dir_fstClusLO, 0);
		   		if(fileChainSize > 1)
		   		{
		   			fileChain = clusterChain(dir.dir_fstClusLO);
		   			for(i = 0; i < fileChainSize; i++)
		   			{
		   				fatTable[fileChain[i]] = 0xFFFFFFF;
		   				clusterOffset = firstClusterSector(fileChain[i]) * bpb.bpb_bytesPerSec;
		   				fseek(fd, clusterOffset, SEEK_SET);
		   				emptyDir = (dirEnt*)malloc(sizeof(dirEnt)); //empty dirEnt
		   				fwrite(emptyDir, sizeof(dirEnt), 1, fd);
		   			}
		   		}
		   		fatTable[dir.dir_fstClusLO] = 0x0;
		   		fseek(fd, bpb.bpb_rsvdSecCnt * bpb.bpb_bytesPerSec, SEEK_SET);
				fwrite(fatTable, sizeof(uint32_t), FATSz * bpb.bpb_bytesPerSec / sizeof(uint32_t), fd);

				if(isDir) //if we are removing a directory, we must remove the dot and dotdot files
				{
					clusterOffset = firstClusterSector(dir.dir_fstClusLO) * bpb.bpb_bytesPerSec;
					emptyDir = (dirEnt*)malloc(sizeof(dirEnt)); //empty dirEnt
					fseek(fd, clusterOffset, SEEK_SET);
					fwrite(emptyDir, sizeof(dirEnt), 1, fd); //delete dot entry
					emptyDir = (dirEnt*)malloc(sizeof(dirEnt)); //empty dirEnt
					fseek(fd, clusterOffset + 32, SEEK_SET);
					fwrite(emptyDir, sizeof(dirEnt), 1, fd); //delete dot dot entry
				}

				emptyDir = (dirEnt*)malloc(sizeof(dirEnt)); //empty dirEnt
				(*emptyDir).dir_name[0] = 0xE5;
				fseek(fd, offset + inc, SEEK_SET);
		   		fwrite(emptyDir, sizeof(dirEnt), 1, fd);
		   		return 1;
		   	}
		}
	}

	cwdCluster = tempCWD;
	return -1;
}

int OS_rm(const char *path)
{
	return removeFile(path, 0);
}

int OS_creat(const char *path)
{
	return createFile(path, 0);
}
int OS_write(int fildes, const void *buf, int nbyte, int offset)
{
	if(start == 0)
		init();
	
	dirEnt dir = openDir[fildes];
	if(dir.dir_name[0] == 0x00) //If file location is empty, throw shell error
		return -1;

	//Get the cluster chain and its length
	unsigned int * chain = clusterChain(dir.dir_fstClusLO);
	int length = clusterChainSize(dir.dir_fstClusLO, 0);
	int clusterLengthOfBuffer = ((nbyte + offset) / bytesPerClus) + 1;
	int emptyCluster;
	int i;

	if(clusterLengthOfBuffer > length) //if we need to write beyond the dedicated cluster chain
	{
		while(length < clusterLengthOfBuffer)
		{
			emptyCluster = findEmptyCluster();
			fatTable[chain[length - 1]] = emptyCluster;
			fatTable[emptyCluster] = 0xFFFFFFF;
			chain = clusterChain(dir.dir_fstClusLO);
			length++;
		}
   		fseek(fd, bpb.bpb_rsvdSecCnt * bpb.bpb_bytesPerSec, SEEK_SET);
		fwrite(fatTable, sizeof(uint32_t), FATSz * bpb.bpb_bytesPerSec / sizeof(uint32_t), fd);

		//update the size of the file
		dir.dir_fileSize = offset + nbyte;
		openDir[fildes] = dir;
		fseek(fd, openDirOffset[fildes], SEEK_SET);
		fwrite(&dir, sizeof(dirEnt), 1, fd);
	}

	if(offset + nbyte > dir.dir_fileSize)
	{
		//update the size of the file
		dir.dir_fileSize = offset + nbyte;
		openDir[fildes] = dir;
		fseek(fd, openDirOffset[fildes], SEEK_SET);
		fwrite(&dir, sizeof(dirEnt), 1, fd);
	}


	int bytesWritten = 0;
	int firstCluster = offset / bytesPerClus;
	int firstClusterOffset = offset % bytesPerClus;
	int bytesToWrite = nbyte; //var used to hold specifically how many bytes to read

	/*printf("firstCluster: %d \n", firstCluster);
	printf("chain[firstCluster]: %d \n", chain[firstCluster]);
	printf("firstClusterOffset: %d \n", firstClusterOffset);
	printf("bytesToWrite: %d \n", bytesToWrite);
	printf("firstChainCluster: 0x%x \n", (int)chain[firstCluster]);*/

	//Write the data at the offset of the first cluster (after adding in the offset) relative to the file
	fseek(fd, (firstClusterSector((int)chain[firstCluster]) * bpb.bpb_bytesPerSec) + firstClusterOffset, SEEK_SET);
	fwrite(buf, bytesToWrite, 1, fd);
	//Increment local variables to continue writing (if needed) the next cluster(s) in the chain
	bytesWritten += bytesToWrite;
	firstCluster++;
	bytesToWrite = nbyte - bytesWritten;

	//while there are still bytes left to be write
	while(bytesWritten < nbyte || bytesToWrite > 0)
	{
		//Make sure we aren't writing more bytes there are in a cluster again
		if(bytesToWrite > bytesPerClus)
		{
			bytesToWrite = bytesPerClus;
		}

		//read the data of that cluster into buffer
		fseek(fd, firstClusterSector((int)chain[firstCluster]) * bpb.bpb_bytesPerSec, SEEK_SET);
   		fwrite(buf + bytesWritten, bytesToWrite, 1, fd);
   		bytesWritten += bytesToWrite;
   		bytesToWrite = nbyte - bytesWritten;
   		firstCluster++;
	}

   	return nbyte;
}
