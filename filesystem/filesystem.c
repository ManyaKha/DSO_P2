
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	Last revision 01/04/2020
 *
 */

#include "filesystem/filesystem.h" // Headers for the core functionality
#include "filesystem/auxiliary.h"  // Headers for auxiliary functions
#include "filesystem/metadata.h"   // Type and structure declaration of the file system
#include "filesystem/blocks_cache.h"
#include <string.h>

/*ADDITIONAL STRUCTS*////////////////////////////////////////////////////////////////////////////////////
struct{
	int32_t f_seek;
	int8_t open;
}inodes_x[NUM_INODES];


/*SYSTEM STRUCTURE*//////////////////////////////////////////////////////////////////////////////////////
SuperblockType sBlock;
TypeInodeMap i_map;
TypeBlockMap b_map;
InodesDiskType inodes;


int8_t mounted = 0;

/*AUXILIARY FUNCTIONS*///////////////////////////////////////////////////////////////////////////////////
int metadata_fromDiskToMemory (void){
	char b[BLOCK_SIZE];
	 // To read 0 block from disk into sBlock
	bread(DEVICE_IMAGE, 0, b);
	memmove(&(sBlock), b, sizeof(SuperblockType));
	 // To read the i-node map from disk
	for (int i =0; i<sBlock.numBlocksInodeMap; i++)
		bread(DEVICE_IMAGE, 1+i, (char*)i_map + i*BLOCK_SIZE);


	 // To read the block map from disk
	for (int i =0; i<sBlock.numBlocksBlockMap; i++)
		bread(DEVICE_IMAGE, 1+i+sBlock.numBlocksInodeMap, (char*)b_map + i*BLOCK_SIZE);

	// To read the i-nodes to main memory (in this example each i-node requires one disk block)
	for (int i =0; i<(sBlock.numInodes*sizeof(InodeDiskType)/BLOCK_SIZE); i++)
	{
		bread(DEVICE_IMAGE, i+sBlock.rootInode, (char*)inodes + i*BLOCK_SIZE);
		memmove();
	}
	return 1;
}

int metadata_fromMemoryToDisk ( void ){
	char b[BLOCK_SIZE];

	// To write block 0 from sBlock into disk
	bwrite(DEVICE_IMAGE, 0, b);

	// To write the i-node map to disk
	for (int i=0; i<sBlock.numBlocksInodeMap; i++)
	bwrite(DEVICE_IMAGE, 1+i, ((char *)i_map + i*BLOCK_SIZE));
	// To write the block map to disk
	for (int i=0; i<sBlock.numBlocksBlockMap; i++)
	bwrite(DEVICE_IMAGE, 1+i+sBlock.numBlocksInodeMap, ((char *)b_map + i*BLOCK_SIZE));
	// To write the i-nodes to disk (in this example each i-node requires one disk block)
	for (int i=0; i<(sBlock.numInodes*sizeof(InodeDiskType)/BLOCK_SIZE); i++)
	bwrite(DEVICE_IMAGE, i+sBlock.rootInode, ((char *)inodes + i*BLOCK_SIZE));
	return 1;
}

int namei(char *path){
	int i;
	//Search inode with path
	for (i=0; i<sBlock.numInodes; i++) {
		if (!strcmp(inodes[i].name, path))
			return i;
	}
	return -1;
}

int ialloc ( void ){
	// to search for a free i-node
	int i;
	for (i=0; i<sBlock.numInodes; i++){
		if (i_map[i] == 0) {
			// i-node busy right now
			i_map[i] = 1;
			// default values for the i-node
			memset(&(inodes[i]),0,
			sizeof(InodeDiskType));
			// return the i-node indentification
			return i;
		}
	}
	return -1;
}

int alloc ( void ){
 char b[BLOCK_SIZE];
 int i;
 for (i=0; i<sBlock.numDataBlocks; i++){
 	if (b_map[i] == 0) {
 		// busy block right now
 		b_map[i] = 1;
 		// default values for the block
 		memset(b, 0, BLOCK_SIZE);
 		bwrite(DEVICE_IMAGE, sBlock.firstDataBlock+i, b);
 		// it returns the block id
 		return i;
 	}
 }
 return -1;
}

int ifree(int inode_id){
	//check inode_id vality
	if(inode_id>=sBlock.numInodes)
		return -1;
	//free inode
	i_map[inode_id] =  0;
	return 0;
}

int free(int block_id){
	//check inode_id vality
	if(block_id >= sBlock.numDataBlocks)
		return -1;
	//free block
	b_map[block_id] = 0;
	return 0;
}

int bmap ( int inode_id, int offset )
{
	int b[BLOCK_SIZE/4] ;
	int logic_block ;
	if(inode_id>sBlock.numInodes){
		return -1;
	}
	logic_block = offset / BLOCK_SIZE;
	if (logic_block > (1+BLOCK_SIZE/4))
		return -1;
	// return the associated direct block reference
	if (0 == logic_block)
		return inodes[inode_id].directBlock[0];
	//NOSOTRAS NO TENEMOS INDIRECTOS. HAY QUE CAMBIARLO.
	// return the associated reference in the indirect block
	bread(DEVICE_IMAGE, sBlock.firstDataBlock+inodes[inode_id].directBlock, b);
	return b[logic_block - 1] ; // 1 direct block -> x-1
}


/*REQUIRED FUNCTIONS*///////////////////////////////////////////////////////////////////////////////////////////

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{
	if(deviceSize<MIN_DISK_SIZE || deviceSize>MAX_DISK_SIZE){
		return -1;
	}
	sBlock.magicNumber = 100366919;
	sBlock.numInodes = MAX_NUM_FILES;
	sBlock.rootInode = 1;
	sBlock.numDataBlocks = deviceSize/BLOCK_SIZE;/*?*/
	sBlock.firstDataBLock  = 3;
	sBlock.deviceSize = deviceSize;
	for(int i=0; i<sBlock.numInodes; i++){
		i_map[i]=0; //free
	}
	for(int i=0; i<sBlock.numDataBlocks; i++){
		b_map[i]=0; //free
	}
	for(int i=0; i<sBlock.numInodes; i++){
		memset(&(inodes[i]), 0, sizeof(InodeDiskType));
	}
	metadata_fromMemoryToDisk();
	return 1;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{
	if(1==mounted){
		printf("Error: System file already mounted.\n Execute unmountFS() to continue\n");
		return -1;
	}
	metadata_fromDiskToMemory();
	mounted = 1;
	return -1;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	if(0 == mounted){
		printf("Error: FS already unmounted\n");
		return -1;
	}
	for (int i=0; i<sBlock.numInodes; i++){
		if(i ==  inodes_x[i].open){
			printf("Error: There are open files\n");
			return -1;
		}
	}
	metadata_fromMemoryToDisk();
	mounted = 0;
	return 1;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	return -1;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	return -1;
}

/*
 * @brief	Checks the integrity of the file.
 * @return	0 if success, -1 if the file is corrupted, -2 in case of error.
 */

int checkFile (char * fileName)
{
    return -2;
}

/*
 * @brief	Include integrity on a file.
 * @return	0 if success, -1 if the file does not exists, -2 in case of error.
 */

int includeIntegrity (char * fileName)
{
    return -2;
}

/*
 * @brief	Opens an existing file and checks its integrity
 * @return	The file descriptor if possible, -1 if file does not exist, -2 if the file is corrupted, -3 in case of error
 */
int openFileIntegrity(char *fileName)
{

    return -2;
}

/*
 * @brief	Closes a file and updates its integrity.
 * @return	0 if success, -1 otherwise.
 */
int closeFileIntegrity(int fileDescriptor)
{
    return -1;
}

/*
 * @brief	Creates a symbolic link to an existing file in the file system.
 * @return	0 if success, -1 if file does not exist, -2 in case of error.
 */
int createLn(char *fileName, char *linkName)
{
    return -1;
}

/*
 * @brief 	Deletes an existing symbolic link
 * @return 	0 if the file is correct, -1 if the symbolic link does not exist, -2 in case of error.
 */
int removeLn(char *linkName)
{
    return -2;
}
