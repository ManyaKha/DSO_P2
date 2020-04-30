
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

#define MOUNTED 1
#define UNMOUNTED 0

/*System structure*/
Superblock sBlock;
Struct_inode listInodes[MAX_FILES];

inodes_active inodes[MAX_FILES];

char block_buffer [BLOCK_SIZE];
int status_FS = UNMOUNTED;

/*Auxiliary functions*/
int syncDisk(){
	int i;
	memset(&block_buffer, 0, sizeof(block_buffer));
  memmove(&block_buffer, &sBlock, sizeof(sBlock));
  if (bwrite("disk.dat", 0, block_buffer)<0) {
	printf("Error writing Superblock: bwrite\n");
	return -1;
	}
	memset(&block_buffer, 0, sizeof(block_buffer));
	memmove(&block_buffer, &listInodes[0], sizeof(listInodes[0])*20);
	if (bwrite("disk.dat", 1, block_buffer)<0) {
		printf("Error writing block with ID %i: bwrite\n", 1);
		return -1;
	}
	memset(&block_buffer, 0, sizeof(block_buffer));
	memmove(&block_buffer, &listInodes[20], sizeof(listInodes[i])*20);
	if (bwrite("disk.dat", 2, block_buffer)<0) {
		printf("Error al escribir el Bloque con ID %i: bwrite\n", 2);
		return -1;
	}
	return 0;
}

int namei(char *path){
	int i;
	for (i=0; i<sBlock.inodes; i++) {
		if (strcmp(listInodes[i].name, path) == 0) return i;
	}
	return -1;
}

int ialloc(void){
	int i, control=-1;
	for (i=0; i<sBlock.inodes; i++) {
		if (bitmap_getbit(sBlock.i_bitmap,i)==0) {
			control=0;
			return i;
		}
	}
	return control;
}

int alloc(void){
	int i, control=-1;
	for (i=0; i<sBlock.num_blocks; i++) {
		if (bitmap_getbit(sBlock.b_bitmap,i)==0) {
			control=0;
			return i;
		}
	}
	return control;
}

int ifree(int inodo_id){
	if(inodo_id<0 || inodo_id>sBlock.inodes){
		return -1;
	}
	bitmap_setbit(sBlock.i_bitmap, inodo_id, 0);
	return 0;
}

int dfree(int bloque_id){
	if(bloque_id<0 || bloque_id<sBlock.num_blocks){
		return -1;
	}
	bitmap_setbit(supbl.b_bitmap, bloque_id, 0);
	return 0;
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
	int num_blocks_system = deviceSize/BLOCK_SIZE;
	int i;

	memset(&block_buffer, 0, sizeof(block_buffer));
	for(i=0; i<num_blocks_system; i++){
		if(bwrite("disk.dat", i, block_buffer)<0){
			printf("Error initializing system: block %i\n", i);
			return -1;
		}
	}

	sBlock.magic_num = 0x000D5500;
	sBlock.inodes = MAX_NUM_FILES;
	sBlock.root_node = 1;
	sBlock.num_blocks = (deviceSize/BLOCK_SIZE)-1-2;
	sBlock.first_block  = 3;
	sBlock.device_size = deviceSize;
	for(i=0; i<sBlock.inodes; i++){
		bitmap_setbit(sBlock.i_bitmap, i, 0);
	}
	for(i=0; i<sBlock.inodes; i++){
		bitmap_setbit(sBlock.b_bitmap, i, 0);
	}
	for(i=0; i<sBlock.inodes; i++){
		strcpy(listInodes[i].name, "");
		listInodes[i].file_size = 0;
		listInodes[i].type = T_FILE;
		listInodes[i].direct_block = -1;
	}
	syncDisk();
	return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{
	if(status_FS==MOUNTED){
		printf("Error: System file already mounted.\n Execute unmountFS() to continue\n");
		return -1;
	}
	memset(&block_buffer, 0, sizeof(block_buffer));
	if(bread("disk.dat", 0, block_buffer)<0){
		printf("Error reading superlock: bread\n");
		return -1;
	}
	memmove(&sBlock, &block_buffer, sizeof(sBlock));
	if(sBlock.magic_numn != 0x000D5500){
		return -1;
	}

	memset(&block_buffer, 0, sizeof(block_buffer));
	if (bread("disk.dat", 1, block_buffer)<0){
		printf("Error reading block with ID %i: bread\n", 1);
		return -1;
	}
	memmove(&listInodes[0], &block_buffer, sizeof(listInodes[0])*20);

	memset(&block_buffer, 0, sizeof(block_buffer));
	if (bread("disk.dat", 2, block_buffer)<0){
		printf("Error reading block with ID %i: bread\n", 1);
		return -1;
	}
	memmove(&listInodes[20], &block_buffer, sizeof(listInodes[0])*20);
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	if(status_FS == UNMOUNTED){
		printf("Error: FS already unmounted\n", );
		return -1;
	}
	int i;
	for (i=0; i<sBlock.inodes; i++){
		if(inodes[i].open == 1){
			if(closeFile(i)==0){
				bitmap_setbit(sBlock.i_bitmap, i, 0);
			}
		}
	}
	status_FS = UNMOUNTED;
	retrun syncDisk();
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{
	if(status_FS==UNMOUNTED){
		printf("Error: FS is unmounted");
		return -2;
	}
	if(strlen(fileName)>32){
		printf("Error: File name too long\n");
		return -2;
	}
	pathID = namei(fileName);
	if(pathID>=0){
		return -2;
	}

	int newInodeID = ialloc();

	if(newInodeID<0){
		return -2;
	}

	listInodes[newInodeID].type = T_FILE;
	listInodes[newInodeID].file_size = 0;
	strcpy(listInodes[newInodeID].name, path);
	bitmap_setbit(sBlock.i_bitmap, newInodeID, 1);

	sBlock.num_blocks = (sBlock.device_size/BLOCK_SIZE)-1-2;
	int bdata_empty = alloc()
	if(bdata_empty == -1){
		ifree(newInodeID);
		printf("Error: There are no free data blocks\n");
		return -2;
	}
	bitmap_setbit(sBlock.b_bitmap, bdata_empty, 1);
	listInodes[newInodeID].direct_block = bdata_empty;
	inodes[newInodeID].open = 1;
	inodes[newInodeID].currentbyte = 0;
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
		printf("Error: File does not exist\n", );
		return -1;
	}

	int inode;
	if(inodes[inode].open==1){
		if(closeFile(inode)<0){
			return-2;
		}
	}

	dfree(listInodes[inode].direct_block);
	ifre(inode);
	return 0;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	if(stateFS==UNMOUNTED){
		printf("Error: FS already unmounted\n");
		return -2;
	}
	if(inodes[namei(fileName)].open == 1){
		printf("File already opened\n", );
		return -2;
	}
	inodes[namei(fileName)].currentbyte = 0
	inodes[namei(fileName)].open= 1;
	return namei(fileName);
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	if(fileDescriptor<0 || fileDescriptor>sBlock.inodes-1){
		return -1;
	}
	inodes[fileDescriptor].currentbyte = 0;
	inodes[fileDescriptor].open = 0;
	return 0;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	if(fileDescriptor<0 || fileDescriptor>sBlock.inodes){
		return -1;
	}
	if (!inodes[fileDescriptor].open){
		return -1;
	}
	int bytesRead=0;
	if(inodes[fileDescriptor].currentbyte+numBytes > BLOCK_SIZE){
		bytesRead=BLOCK_SIZE-inodes[fileDescriptor].currentbyte;
	}
	else if (bytesRead<0) {
		return 0;
	}
	else {
		bytesRead = numBytes;
	}
	int DataBLockID = listInodes[fileDescriptor].direct_block;
	memset(block_buffer, 0, BLOCK_SIZE);
	bread("disk.dat", DataBLockID, block_buffer);
	memmove(buffer, block_buffer+inodes[fileDescriptor].currentbyte, sizeof(&buffer));
	inodes[fileDescriptor].currentbyte +=bytesRead;
	return bytesRead;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	if(fileDescriptor<0 || fileDescriptor>sBlock.inodes){
		return -1;
	}
	if (!inodes[fileDescriptor].open){
		return -1;
	}
	int bytesWriten=0;
	if(inodes[fileDescriptor].currentbyte+numBytes > BLOCK_SIZE){
		bytesWriten=BLOCK_SIZE-inodes[fileDescriptor].currentbyte;
	}
	else if (bytesWriten<0){
		return 0;
	}
	else {
		bytesWriten = numBytes;
	}
	int DataBlockID = listInodes[fileDescriptor].direct_block;
	memset(block_buffer, 0, BLOCK_SIZE);
	bread("disk.dat", DataBlockID, block_buffer);
	memmove(block_buffer+inodes[fileDescriptor].currentbyte, buffer, bytesWriten);
	bwrite("disk.dat", DataBlockID, block_buffer);
	inodes[fileDescriptor].currentbyte +=bytesWriten;
	listInodes[fileDescriptor].file_size+=bytesWriten;
	syncDisk();
	return bytesWriten;
	}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	if(inodes[fileDescriptor].open==0){
			printf("The file is closed");
			return -1;
		}
	if(whence == FS_SEEK_CUR){
		if((inodes[fileDescriptor].currentbyte+offset)>=0 &&
		(inodes[fileDescriptor].currentbyte+offset)<BLOCK_SIZE){
				inodes[fileDescriptor].currentbyte += offset;
			}
		else{
			if(offset>=0){
				inodes[fileDescriptor].currentbyte=2047;
			}
			else{
				inodes[fileDescriptor].currentbyte=0;
			}
		}
	}
	else if (whence == FS_SEEK_BEGIN){
		inodes[fileDescriptor].currentbyte=0;
	}
	else if (whence == FS_SEEK_END){
		inodes[fileDescriptor].currentbyte=2047;
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
