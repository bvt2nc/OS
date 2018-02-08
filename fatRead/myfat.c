#include "myfat.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

void init();
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

int main() {

	//Sandbox for testing functions

    OS_cd("PEOPLE");
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
    /*init();
    readFatTable(fd);
    recurseThroughDir(fd, firstClusterSector(2) * bpb.bpb_bytesPerSec);*/

    return 0;
}

//Load the disk and set all the globals variables
//Loads bpb and FAT table into memory
//Allocates memory to global arrays used by core functions
void init()
{
    //fd = fopen("sampledisk32.raw", "rb");
	//If you don't want to do the below, uncomment above statement
	//Assumes sampledisk32.raw is in the same directory as code
    fd = fopen(getenv("FAT_FS_PATH"), "rb"); //get the env var FAT_FS_PATH
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
	   		unsigned int * chain = clusterChain(dir.dir_fstClusLO);
	   	}
	   	//printf("===================================\n");
	}
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
   	/*printf("First Cluster High: 0x%x \n", dir.dir_fstClusHI);
   	printf("First Cluster Low: 0x%x \n", dir.dir_fstClusLO);
   	printf("First Cluster Table Value: 0x%x \n", tableValue(dir.dir_fstClusLO));
   	if(dir.dir_fstClusLO < EOC)
   		printf("Next Cluster Table Value: 0x%x \n", tableValue(tableValue(dir.dir_fstClusLO)));*/
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
	fatTable = (uint32_t *)malloc(FATSz * bpb.bpb_bytesPerSec);
	fseek(fd, bpb.bpb_rsvdSecCnt * bpb.bpb_bytesPerSec, SEEK_SET);
	fread(fatTable, sizeof(uint32_t), FATSz * bpb.bpb_bytesPerSec / sizeof(uint32_t), fd);
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

	//If path is an absolute path starting at the root directory
	if(path[0] == '/')
	{
		cwdCluster = 2;
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
	int i, status;
	char * subPath = (char *)malloc(sizeof(char) * 8);
	int index = 0;

	//Parse through path by finding the 'subpath' (.../*THIS_IS_THE_SUBPATH*/...)
	for(i = 1; i < strlen(path); i++)
	{
		if(path[i] == '/') //reached beginning/end of subpath
		{
			//printf("subPath: %s \n", subPath);
			status = OS_cd(subPath);
			index = 0;
			//We are only looking for directories so we don't care about ext
			subPath = (char *)malloc(sizeof(char) * 8);

			if(status == -1)
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
	char * str = (char *)malloc(sizeof(char) * 8);
	int i;

	int max = 8;
	if(file == 1)
		max = 11;

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
	int tempCWD = 0;
	//If absolute path, save the cwdcluster to be loaded back again at end of function and cd
	if(dirname[0] != '.')
	{
		tempCWD = cwdCluster;
		//printf("tempCWD: %d \n", tempCWD);
		OS_cd(dirname);
	}

	dirEnt* ls = (dirEnt*)malloc(sizeof(dirEnt) * 128);

	if(start == 0)
		init();

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
	   	count++;
	   	printDir(dir);
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
		   	count++;
		   	printDir(dir);
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

	dirEnt * dir = OS_readDir(".");
	int i, j;
	int terminate = 0;
	char * realPath = (char *)malloc(sizeof(char) * 8);

	//See cd
	//tldr; Reparse the path to get rid of garbage at the end if the short name is smaller than the max allowed
	for(i = 0; i < 11; i++)
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
	if(nbyte > (length * bytesPerClus) || offset > (length * bytesPerClus))
		return -1;

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

   	return 1;
}




//========================================================WRITE=================================================






int OS_rmdir(const char *path)
{
	return -1;
}
int OS_mkdir(const char *path)
{
	return -1;
}
int OS_rm(const char *path)
{
	return -1;
}
int OS_creat(const char *path)
{
	return -1;
}
int OS_write(int fildes, const void *buf, int nbyte, int offset)
{
	return -1;
}
