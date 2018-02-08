#include "myfat.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct __attribute__ ((packed)) {

        uint8_t bs_jmpBoot[3];          // jmp instr to boot code
        uint8_t bs_oemName[8];          // indicates what system formatted this field, default=MSWIN4.1
        uint16_t bpb_bytesPerSec;       // Count of bytes per sector
        uint8_t bpb_secPerClus;         // no.of sectors per allocation unit
        uint16_t bpb_rsvdSecCnt;        // no.of reserved sectors in the resercved region of the volume starting at 1st sector
        uint8_t bpb_numFATs;            // The count of FAT datastructures on the volume
        uint16_t bpb_rootEntCnt;        // Count of 32-byte entries in root dir, for FAT32 set to 0
        uint16_t bpb_totSec16;          // total sectors on the volume
        uint8_t bpb_media;              // value of fixed media
        uint16_t bpb_FATSz16;           // count of sectors occupied by one FAT
        uint16_t bpb_secPerTrk;         // sectors per track for interrupt 0x13, only for special devices
        uint16_t bpb_numHeads;          // no.of heads for intettupr 0x13
        uint32_t bpb_hiddSec;           // count of hidden sectors
        uint32_t bpb_totSec32;          // count of sectors on volume
        uint32_t bpb_FATSz32;           // define for FAT32 only
        uint16_t bpb_extFlags;          // Reserved for FAT32
        uint16_t bpb_FSVer;             // Major/Minor version num
        uint32_t bpb_RootClus;          // Clus num of 1st clus of root dir
        uint16_t bpb_FSInfo;            // sec num of FSINFO struct
        uint16_t bpb_bkBootSec;         // copy of boot record
        uint8_t bpb_reserved[12];       // reserved for future expansion
        uint8_t bs_drvNum;              // drive num
        uint8_t bs_reserved1;           // for ue by NT
        uint8_t bs_bootSig;             // extended boot signature
        uint32_t bs_volID;              // volume serial number
        uint8_t bs_volLab[11];          // volume label
        uint8_t bs_fileSysTye[8];       // FAT12, FAT16 etc
} bpbFat321 ;

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
int cwdCluster;
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
//int openDirSize;

int main() {

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

void init()
{
    fd = fopen("sampledisk32.raw", "rb");

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
   	//openDirSize = 1;

   	start = 1;
   	cwdCluster = 2; //default to root directory
   	
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

void recurseThroughDir(FILE * fd, int offset)
{
	int inc;
   	dirEnt dir;
   	//printf("offset: %d\n", offset);
   	for(inc = 0; inc < bytesPerClus - 64; inc += 32)
   	{
   		//printf("inc: %d\n", inc);
	   	fseek(fd, offset + inc, SEEK_SET);
	   	//printf("offset first cluster: %d \n", firstClusterSector(3));
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

int firstClusterSector(int n)
{
	return ((n - 2) * bpb.bpb_secPerClus) + firstDataSector;
}

void printDir(dirEnt dir)
{
	int i, attr;
	attr = dir.dir_attr;
	char c;
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

unsigned int tableValue(int cluster)
{
	unsigned int fatOffset;
	if(fatType == 16)
		fatOffset = cluster * 2;
	else
		fatOffset = cluster * 4;
	
	//printf("fatOffset: %d \n", fatOffset);
	//unsigned int fatSector = bpb.bpb_rsvdSecCnt + (fatOffset / bpb.bpb_bytesPerSec);
	unsigned int fatSector = (fatOffset / bpb.bpb_bytesPerSec);
	//printf("fatSector: %d \n", fatSector);
	//printf("fatSector offset: %d \n", fatSector * bpb.bpb_bytesPerSec / 32);
	unsigned int fatEntOffset = fatOffset % bpb.bpb_bytesPerSec;
	//printf("fatEntOffset: %d \n", fatEntOffset);

	//printf("tableValue: 0x%x: ", *(unsigned int*)&fatTable[fatOffset]);
	//return *(unsigned int*)&fatTable[(fatSector / 16) + fatOffset] & 0x0FFFFFFF;
	return *(unsigned int*)&fatTable[cluster] & 0x0FFFFFFF;
}

int clusterChainSize(int cluster, int size)
{
	//if(size >= 10)
	//	return size;

	int value = tableValue(cluster);
	//printf("table value: 0x%x: \n", value);
	size++;

	if(value >= 0x0FFFFFF8)
	{
		return size;
	}

	return clusterChainSize(value, size);
}

unsigned int * clusterChain(int cluster)
{
	int value = tableValue(cluster);
	int size = clusterChainSize(cluster, 0);
	//printf("size: %d", size);
	unsigned int * chain = (unsigned int*)malloc(sizeof(unsigned int) * size);
	chain[0] = tableValue(cluster);
	printf("cluster chain: 0x%x ", cluster);
	printf("0x%x ", chain[0]);

	int i;
	for(i = 1; i < size; i++)
	{
		chain[i] = tableValue(chain[i - 1]);
		//printf("0x%x ", chain[i]);
	}
	printf("\n");

	return chain;

}

void readFatTable(FILE * fd)
{
	fatTable = (uint32_t *)malloc(FATSz * bpb.bpb_bytesPerSec);
	//printf("size of fatTable: %d \n", sizeof(fatTable));
	//printf("bytes in fat table: %d \n", bpb.bpb_numFATs * FATSz * bpb.bpb_bytesPerSec);
	


	fseek(fd, bpb.bpb_rsvdSecCnt * bpb.bpb_bytesPerSec, SEEK_SET);
	//fatTable = (uint16_t *)malloc(sizeof(uint16_t*) * 5000);
	//fread(fatTable, sizeof(uint16_t *), 5000, fd);

	//read(fd, fatTable, FATSz * bpb.bpb_bytesPerSec / 4);

	fread(fatTable, sizeof(uint32_t), FATSz * bpb.bpb_bytesPerSec / sizeof(uint32_t), fd);
	/*fread(&fatTable, 4, 500, fd);
	printf("done reading \n");
	int i;
	printf("length: %d \n", sizeof(fatTable));
	for(i = 0; i < 1000; i++)
	{
		printf("FAT[%d]: 0x%x \n", i, *(unsigned int*)&fatTable[i] & 0x0FFFFFFF);
	}*/
}

int OS_cd(const char *path)
{	
	if(start == 0)
		init();

	if(path[0] == '/')
	{
		//printf("absolute \n");
		cwdCluster = 2;
		return cdAbsolute(path);
	}

	if(path[0] == '~')
	{
		cwdCluster = 2;
		return 1;
	}

	dirEnt * lsDir = OS_readDir(".");
	dirEnt dir;
	int i, compare, j;
	int terminate = 0;
	char * dirNamee;
	char * realPath = (char *)malloc(sizeof(char) * 8);

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

	/*for(i = 0; i < 8; i++)
	{
		printf("%d ", realPath[i]);
	}
	printf("\n");*/

	for(i = 0; i < 128; i++)
	{
		dir = lsDir[i];
		if(dir.dir_name[0] == 0)
		{
			printf("cannot find, at index: %d \n", i);
			break;
		}

		dirNamee = dirName(dir, 0);

		compare = strcmp(realPath, dirNamee);
		//printf("compare: %d \n", compare); 
		if(compare == 0)
		{
			printf("match! \n");
			cwdCluster = dir.dir_fstClusLO;

			if(cwdCluster == 0)
				cwdCluster = 2;

			/*printDir(dir);
			printf("cwdCluster after cd: %d \n", cwdCluster);
			printf("===================\n");*/
			return 1;
		}

	}

	//printf("failed\n");

	return -1;
}

int cdAbsolute(const char * path)
{
	//printf("path: %s \n", path);

	int i, status;
	char * subPath = (char *)malloc(sizeof(char) * 8);
	int index = 0;

	for(i = 1; i < strlen(path); i++)
	{
		if(path[i] == '/')
		{
			//printf("subPath: %s \n", subPath);
			status = OS_cd(subPath);
			index = 0;
			subPath = (char *)malloc(sizeof(char) * 8);

			if(status == -1)
			{
				return -1;
			}
		}
		else
		{
			subPath[index] = path[i];
			index++;
			//printf("%s \n", subPath);
		}
	}

	//printf("subPath: %s \n", subPath);
	status = OS_cd(subPath);
	if(status == -1)
		return -1;

	return 1;
}

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

dirEnt * OS_readDir(const char *dirname)
{
	//Assume this acts like ls
	int tempCWD = 0;

	if(dirname[0] != '.')
	{
		//printf("ls absolute path \n");
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

	   	ls[count] = dir;
	   	count++;
	   	//printDir(dir);
	}

	if(tempCWD != 0)
		cwdCluster = tempCWD;

	//printf("after cwd: %d \n", cwdCluster);

	return ls;
}

int OS_open(const char *path)
{

	if(start == 0)
		init();

	dirEnt * dir = OS_readDir(".");
	int i, j;
	int terminate = 0;
	char * realPath = (char *)malloc(sizeof(char) * 8);

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
	printf("realPath: %s \n", realPath);

	for(i = 0; i < 128; i++)
	{
		if(dir[i].dir_name[0] == 0x00)
		{
			break;
		}

		//printf("dirName: %s \n", dirName(dir[i], 1));
		if(strcmp(realPath, dirName(dir[i], 1)) == 0)
		{

			for(j = 0; j < 128; j++)
			{
				if(openDir[j].dir_name[0] == 0x00)
				{
					openDir[j] = dir[i];
					return j;
				}
			}

			printf("Too many files opened. Please close a file\n");
			break;
		}
	}

	return -1;
}

int OS_close(int fd)
{
	if(start == 0)
		init();

	dirEnt dir = openDir[fd];
	if(dir.dir_name[0] == 0x00)
		return -1;
	dirEnt *emptyDir = (dirEnt*)malloc(sizeof(dirEnt));
	openDir[fd] = *emptyDir;

	return 1;
}

int OS_read(int fildes, void *buf, int nbyte, int offset)
{

	if(start == 0)
		init();
	
	dirEnt dir = openDir[fildes];
	if(dir.dir_name[0] == 0x00)
		return -1;

	unsigned int * chain = clusterChain(dir.dir_fstClusLO);
	printf("finished cluster chaining \n");

	//if(nbyte > (chain * bytesPerClus) || offset > (chain * bytesPerClus))
	//	return -1;

	int bytesRead = 0;
	int firstCluster = offset / bytesPerClus;
	int firstClusterOffset = offset % bytesPerClus;
	int bytesToRead = nbyte;

	if(nbyte > bytesPerClus - firstClusterOffset)
		bytesToRead = bytesPerClus - firstClusterOffset;

	printf("firstCluster: %d \n", firstCluster);
	printf("firstClusterOffset: %d \n", firstClusterOffset);
	printf("bytesToRead: %d \n", bytesToRead);

	fseek(fd, (firstClusterSector((int)chain[firstCluster]) * bpb.bpb_bytesPerSec) + firstClusterOffset, SEEK_SET);
	fread(buf, bytesToRead, 1, fd);
	bytesRead += bytesToRead;

	while(bytesRead < nbyte)
	{
		return -1;
		fseek(fd, firstClusterSector(dir.dir_fstClusLO) * bpb.bpb_bytesPerSec, SEEK_SET);
   		fread(buf, nbyte, 1, fd);
	}

   	return 1;

	//return -1;
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