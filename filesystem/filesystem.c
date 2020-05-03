
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

/*System structure*/
SuperblockType sBlocks[1];
//char imap [numInodes]; //[Block_size*x] ????
char imap [MAX_NUM_FILES]; //[Block_size*x] ????
char dbmap [numDataBlocks]; //[Block_size*y] ???
//InodeDiskType inodes[numInodes];
InodeDiskType inodes[MAX_NUM_FILES];
//numInodes_active numInodes[MAX_FILES];

//char block_buffer [BLOCK_SIZE];
//int status_FS = UNMOUNTED;

//Additional in-memory information
struct{
	int f_seek;
	int open;
} //inode_x[numInodes];
inode_x[MAX_NUM_FILES];

int mounted = 0;

/*Auxiliary functions*/

int metadata_fromDiskToMemory (void){
	 // To read 0 block from disk into sBlocks[0]
	bread("disk.dat", 0, &(sBlocks[0]));
	 // To read the i-node map from disk
	for (int i =0; i<sBlocks[0].numBlocksInodeMap; i++)
		bread("disk.dat", 1+i, (char*)imap + i*BLOCK_SIZE);
	 // To read the block map from disk
	for (int i =0; i<sBlocks[0].numBlocksBlockMap; i++)
		bread("disk.dat", 1+i+sBlocks[0].numBlocksInodeMap, (char*)dbmap + i*BLOCK_SIZE);
	// To read the i-nodes to main memory (in this example each i-node requires one disk block)
	for (int i =0; i<(sBlocks[0].numInodes*sizeof(InodeDiskType)/BLOCK_SIZE); i++)
		bread("disk.dat", i+sBlocks[0].rootInode, (char*)inodes + i*BLOCK_SIZE);
	return 1;
}

int metadata_fromMemoryToDisk ( void )
{
 // To write block 0 from sBlocks[0] into disk
 bwrite("disk.dat", 0, &(sBlocks[0]));
 // To write the i-node map to disk
 for (int i=0; i<sBlocks[0].numBlocksInodeMap; i++)
 bwrite("disk.dat", 1+i, ((char *)imap + i*BLOCK_SIZE));
 // To write the block map to disk
 for (int i=0; i<sBlocks[0].numBlocksBlockMap; i++)
 bwrite("disk.dat", 1+i+sBlocks[0].numBlocksInodeMap, ((char *)dbmap + i*BLOCK_SIZE));
 // To write the i-nodes to disk (in this example each i-node requires one disk block)
 for (int i=0; i<(sBlocks[0].numInodes*sizeof(InodeDiskType)/BLOCK_SIZE); i++)
 bwrite("disk.dat", i+sBlocks[0].rootInode, ((char *)inodes + i*BLOCK_SIZE));
 return 1;
}

int namei(char *path){
	int i;
	//Search inode with path
	for (i=0; i<sBlocks[0].numInodes; i++) {
		if (!strcmp(inodes[i].name, path))
			return i;
	}
	return -1;
}

int ialloc ( void ){
 // to search for a free i-node
 for (int i=0; i<sBlocks[0].numInodes; i++){
 	if (imap[i] == 0) {
 		// i-node busy right now
 		imap[i] = 1;
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
 for (int i=0; i<sBlocks[0].numDataBlocks; i++){
 	if (bmap[i] == 0) {
 		// busy block right now
 		bmap[i] = 1;
 		// default values for the block
 		memset(b, 0, BLOCK_SIZE);
 		bwrite("disk.dat", i, b);
 		// it returns the block id
 		return i;
 	}
 }
 return -1;
}

int ifree(int inode_id){
	//check inode_id vality
	if(inode_id>sBlocks[0].numInodes)
		return -1;
	//free inode
	imap[inode_id] =  0;
	return -1;
}

int free(int block_id){
	//check inode_id vality
	if(block_id > sBlocks[0].numDataBlocks)
		return -1;
	//free block
	bmap[block_id] = 0;
	return -1;
}

int bmap ( int inode_id, int offset )
{
 int b[BLOCK_SIZE/4] ;
 int logic_block ;
 logic_block = offset / BLOCK_SIZE;
 if (logic_block > (1+BLOCK_SIZE/4))
 	return -1;
 // return the associated direct block reference
 if (0 == logic_block)
 	return inodes[inode_id].directBlock;
 // return the associated reference in the indirect block
 bread("disk.dat", sBlocks[0].firstDataBlock +
 inodes[inode_id].indirectBlock, b);
 return b[logic_block - 1] ; // 1 direct block -> x-1
}

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{
	if(deviceSize<MIN_DISK_SIZE || deviceSize>MAX_DISK_SIZE){
		return -1;
	}
	sBlocks[0].magicNumber = 0x000D5500;
	sBlocks[0].numInodes = MAX_NUM_FILES;
	sBlock[0].rootInode = 1;
	sBlock[0].numDataBlocks = deviceSize/BLOCK_SIZE;/*?*/
	sBlock[0].firstDataBLock  = 3;
	sBlock[0].deviceSize = deviceSize;
	for(i=0; i<sBlocks[0].numInodes; i++){
		imap[i]=0; //free
	}
	for(i=0; i<sBlocks[0].numDataBlocks; i++){
		bmap[i]=0; //free
	}
	for(i=0; i<sBlocks[0].numInodes; i++){
		memset(&(inodes[i]), 0, sizeof(InodeDiskType));
	}
	metadata_fromMemoryToDisk();
	return 1;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
