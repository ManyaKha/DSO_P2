
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
char imap [numInodes + PADDING_I]; //[Block_size*x] ????
char dbmap [numDataBlocks + PADDING_D]; //[Block_size*y] ???
InodeDiskType inodes[numInodes];
//numInodes_active numInodes[MAX_FILES];

//char block_buffer [BLOCK_SIZE];
//int status_FS = UNMOUNTED;

//Additional in-memory information
struct{
	int f_seek;
	int open;
} inode_x[numInodes];

int mounted = 0;

/*Auxiliary functions*/
/*int syncDisk(){
	int i;
	memset(&block_buffer, 0, sizeof(block_buffer));
  memmove(&block_buffer, &sBlock, sizeof(sBlock));
  if (bwrite("disk.dat", 0, block_buffer)<0) {
	printf("Error writing Superblock: bwrite\n");
	return -1;
	}
	memset(&block_buffer, 0, sizeof(block_buffer));
	memmove(&block_buffer, &listnumInodes[0], sizeof(listnumInodes[0])*20);
	if (bwrite("disk.dat", 1, block_buffer)<0) {
		printf("Error writing block with ID %i: bwrite\n", 1);
		return -1;
	}
	memset(&block_buffer, 0, sizeof(block_buffer));
	memmove(&block_buffer, &listnumInodes[20], sizeof(listnumInodes[i])*20);
	if (bwrite("disk.dat", 2, block_buffer)<0) {
		printf("Error al escribir el Bloque con ID %i: bwrite\n", 2);
		return -1;
	}
	return 0;
}*/

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

/*int ialloc(void){
	int i, control=-1;
	//Search for free inode
	for (i=0; i<sBlock.numInodes; i++) {
		//if (bitmap_getbit(sBlock.i_bitmap,i)==0) {
			control=0;
			return i;
		}
	}
	return control;
}

int alloc(void){
	int i, control=-1;
	for (i=0; i<sBlock.numDataBlocks; i++) {
		//if (bitmap_getbit(sBlock.b_bitmap,i)==0) {
			control=0;
			return i;
		}
	}
	return control;
}*/

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
	/*int numDataBlocks_system = deviceSize/BLOCK_SIZE;
	int i;

	memset(&block_buffer, 0, sizeof(block_buffer));
	for(i=0; i<numDataBlocks_system; i++){
		if(bwrite("disk.dat", i, block_buffer)<0){
			printf("Error initializing system: block %i\n", i);
			return -1;
		}
	}*/

	sBlocks[0].magicNumber = 0x000D5500;
	sBlocks[0].numInodes = MAX_NUM_FILES;
	sBlock[0].rootInode = 1;
	sBlock[0].numDataBlocks = (deviceSize/BLOCK_SIZE)-1-2;
	sBlock[0].firstDataBLock  = 3;
	sBlock[0].deviceSize = deviceSize;
	for(i=0; i<sBlocks[0].numInodes; i++){
		//bitmap_setbit(sBlock.i_bitmap, i, 0);
		imap[i]=0; //free
	}
	for(i=0; i<sBlocks[0].numDataBlocks; i++){
		//bitmap_setbit(sBlock.b_bitmap, i, 0);
		bmap[i]=0; //free
	}
	for(i=0; i<sBlocks[0].numInodes; i++){
		/*strcpy(listnumInodes[i].name, "");
		listnumInodes[i].size = 0;
		listnumInodes[i].type = T_FILE;
		listnumInodes[i].directBlock = -1;*/
		memset(&(inodes[i]), 0, sizeof(InodeDiskType));
	}
	//syncDisk();
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
	/*memset(&block_buffer, 0, sizeof(block_buffer));
	if(bread("disk.dat", 0, block_buffer)<0){
		printf("Error reading superlock: bread\n");
		return -1;
	}
	memmove(&sBlock, &block_buffer, sizeof(sBlock));
	if(sBlock.magicNumbern != 0x000D5500){
		return -1;
	}

	memset(&block_buffer, 0, sizeof(block_buffer));
	if (bread("disk.dat", 1, block_buffer)<0){
		printf("Error reading block with ID %i: bread\n", 1);
		return -1;
	}
	memmove(&listnumInodes[0], &block_buffer, sizeof(listnumInodes[0])*20);

	memset(&block_buffer, 0, sizeof(block_buffer));
	if (bread("disk.dat", 2, block_buffer)<0){
		printf("Error reading block with ID %i: bread\n", 1);
		return -1;
	}
	memmove(&listnumInodes[20], &block_buffer, sizeof(listnumInodes[0])*20);*/
	metadata_fromDiskToMemory();
	mounted = 1;
	return 1;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void){
	if(0 == mounted){
		printf("Error: FS already unmounted\n");
		return -1;
	}
	int i;
	for (i=0; i<sBlocks[0].numInodes; i++){
		if(1 ==  inode_x[i].open){
			printf("Error: There are open files\n");
			return -1;
			//if(closeFile(i)==0){
				//bitmap_setbit(sBlock.i_bitmap, i, 0);
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
	if(0==mounted){
		printf("Error: FS is unmounted");
		return -2;
	}
	if(strlen(fileName)>32){
		printf("Error: File name too long\n");
		return -2;
	}
	char pathID = namei(fileName);
	if(pathID>=0){
		return -2;
	}

	int newInodeID = ialloc();

	if(newInodeID<0){
		return -2;
	}

	//listnumInodes[newInodeID].type = T_FILE;
	//listnumInodes[newInodeID].size = 0;
	//strcpy(listnumInodes[newInodeID].name, path);
	//bitmap_setbit(sBlock.i_bitmap, newInodeID, 1);

	sBlocks.numDataBlocks = (sBlocks.deviceSize/BLOCK_SIZE)-1-2;
	int bdata_empty = alloc()
	if(bdata_empty == -1){
		ifree(newInodeID);
		printf("Error: There are no free data blocks\n");
		return -2;
	}
	//bitmap_setbit(sBlock.b_bitmap, bdata_empty, 1);
	listnumInodes[newInodeID].directBlock = bdata_empty;
	numInodes[newInodeID].open = 1;
	numInodes[newInodeID].currentbyte = 0;
	printf("FD FILE: %d\n", newInodeID);
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	if(status_FS==UNMOUNTED){
		printf("Error: FS already unmounted\n");
		return -2;
	}
	if(namei(fileName)==-1){
		printf("Error: File does not exist\n");
		return -1;
	}

	int inode;
	if(numInodes[inode].open==1){
		if(closeFile(inode)<0){
			return-2;
		}
	}

	free(listnumInodes[inode].directBlock);
	ifre(inode);
	return 0;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	if(0==mounted){
		printf("Error: FS is unmounted\n");
		return -2;
	}
	int inode_id;
	inode_id =  namei(filename);
	/*if(numInodes[namei(fileName)].open == 1){
		printf("File already opened\n", );
		return -2;
	}
	numInodes[namei(fileName)].currentbyte = 0
	numInodes[namei(fileName)].open= 1;
	return namei(fileName);*/
	if(inode_id<0){
		return inode_id;
	}
	inode_x[inode_id.f_seek = 0 ;
	inode_x[inode_id].open = 1;
	return inode_id;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	/*if(fileDescriptor<0 || fileDescriptor>sBlock.numInodes-1){
		return -1;
	}
	numInodes[fileDescriptor].currentbyte = 0;
	numInodes[fileDescriptor].open = 0;
	return 0;*/
	if (fileDescriptor < 0){
		return fileDescriptor ;
	}
 inode_x[fileDescriptor].f_seek = 0;
 inode_x[fileDescriptor].open = 0;
 return 1;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	/*if(fileDescriptor<0 || fileDescriptor>sBlock.numInodes){
		return -1;
	}
	if (!numInodes[fileDescriptor].open){
		return -1;
	}
	int bytesRead=0;
	if(numInodes[fileDescriptor].currentbyte+numBytes > BLOCK_SIZE){
		bytesRead=BLOCK_SIZE-numInodes[fileDescriptor].currentbyte;
	}
	else if (bytesRead<0) {
		return 0;
	}
	else {
		bytesRead = numBytes;
	}
	int DataBLockID = listnumInodes[fileDescriptor].directBlock;
	memset(block_buffer, 0, BLOCK_SIZE);
	bread("disk.dat", DataBLockID, block_buffer);
	memmove(buffer, block_buffer+numInodes[fileDescriptor].currentbyte, sizeof(&buffer));
	numInodes[fileDescriptor].currentbyte +=bytesRead;
	return bytesRead;*/
	char b[BLOCK_SIZE];
	int b_id;
	if(inode_x[fileDescriptor].f_seek+numBytes > inodes[fileDescriptor].numBytes){
		numBytes = inodes[fileDescriptor].numBytes - inode_x[fileDescriptor].f_seek;
	}
	if(numBytes<=0){
		return 0;
	}
	b_id = bmap(fileDescriptor, inode_x[fileDescriptor].f_seek);
	bread("disk.dat", b_id; b);
	memmove(buffer, b+inode_x[fileDescriptor].f_seek, numBytes);

	inode_x[fileDescriptor].f_seek += numBytes;
	return numBytes;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	/*if(fileDescriptor<0 || fileDescriptor>sBlock.numInodes){
		return -1;
	}
	if (!numInodes[fileDescriptor].open){
		return -1;
	}
	int bytesWriten=0;
	if(numInodes[fileDescriptor].currentbyte+numBytes > BLOCK_SIZE){
		bytesWriten=BLOCK_SIZE-numInodes[fileDescriptor].currentbyte;
	}
	else if (bytesWriten<0){
		return 0;
	}
	else {
		bytesWriten = numBytes;
	}
	int DataBlockID = listnumInodes[fileDescriptor].directBlock;
	memset(block_buffer, 0, BLOCK_SIZE);
	bread("disk.dat", DataBlockID, block_buffer);
	memmove(block_buffer+numInodes[fileDescriptor].currentbyte, buffer, bytesWriten);
	bwrite("disk.dat", DataBlockID, block_buffer);
	numInodes[fileDescriptor].currentbyte +=bytesWriten;
	listnumInodes[fileDescriptor].size+=bytesWriten;
	syncDisk();
	return bytesWriten;*/
	char b[BLOCK_SIZE],
	int b_id;
	if(inode_x[fileDescriptor].f_seek+numBytes > BLOCK_SIZE){
		numBytes = BLOCK_SIZE - inode_x[fileDescriptor].f_seek;
	}
	if(numBytes<=0){
		return 0;
	}
	b_id = bmap(fileDescriptor, inode_x[fileDescriptor].f_seek);
 	bread("disk.dat", b_id, b);
 	memmove(b+inode_x[fileDescriptor].f_seek, buffer, numBytes);
 	bwrite("disk.dat", b_id, b);
 	inode_x[fileDescriptor].f_seek += numBytes;
 	inode[fileDescriptor].size += numBytes;
 	return numBytes;
	}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	if(numInodes[fileDescriptor].open==0){
			printf("The file is closed");
			return -1;
		}
	if(whence == FS_SEEK_CUR){
		if((numInodes[fileDescriptor].currentbyte+offset)>=0 &&
		(numInodes[fileDescriptor].currentbyte+offset)<BLOCK_SIZE){
				numInodes[fileDescriptor].currentbyte += offset;
			}
		else{
			if(offset>=0){
				numInodes[fileDescriptor].currentbyte=2047;
			}
			else{
				numInodes[fileDescriptor].currentbyte=0;
			}
		}
	}
	else if (whence == FS_SEEK_BEGIN){
		numInodes[fileDescriptor].currentbyte=0;
	}
	else if (whence == FS_SEEK_END){
		numInodes[fileDescriptor].currentbyte=2047;
	}
	return 0;
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
