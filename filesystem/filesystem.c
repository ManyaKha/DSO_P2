
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
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	return -1;
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
