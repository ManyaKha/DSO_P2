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
#include <stdio.h>
#include <string.h>


/*ADDITIONAL STRUCTS*////////////////////////////////////////////////////////////////////////////////////
struct{
	int16_t f_seek; //read/write seek position
	int8_t open; //0:false, 1:true
}inodes_x[NUM_INODES];


/*SYSTEM STRUCTURE*//////////////////////////////////////////////////////////////////////////////////////
SuperblockType sBlock; //Superblock
TypeInodeMap i_map; //Inodes map
TypeBlockMap b_map; //Blocks map
InodesDiskType inodes;

//State of FS: mounted or unmounted
int8_t mounted = 0; //0:false, 1:true

/*AUXILIARY FUNCTIONS*///////////////////////////////////////////////////////////////////////////////////
int metadata_fromDiskToMemory (void){
	char b[BLOCK_SIZE]; //buffer
	// To read from disk into sBlock, i_map, b_map
	bread(DEVICE_IMAGE, 0, b);
	memmove(&(sBlock), b, sizeof(SuperblockType));
	memmove(&(i_map), b+sizeof(SuperblockType), sizeof(TypeInodeMap));
	memmove(&(b_map), b+sizeof(SuperblockType)+sizeof(TypeInodeMap), sizeof(TypeBlockMap));
	// To read the i-nodes to main memory
	int remaining_inodes  = NUM_INODES;
	for (int i =0; i<(sBlock.numInodesBlocks-1); i++)
	{
		bread(DEVICE_IMAGE, sBlock.rootInodeBlock+i, b);
		memmove(&(inodes[i*sBlock.inodesPerBlock]), b, sBlock.inodesPerBlock*sizeof(InodeDiskType));
		remaining_inodes = remaining_inodes - sBlock.inodesPerBlock;
	}
	bread(DEVICE_IMAGE, sBlock.rootInodeBlock+sBlock.numInodesBlocks-1, b);
	memmove(&(inodes[NUM_INODES-remaining_inodes]), b, remaining_inodes*sizeof(InodeDiskType));
	return 1;
}

int metadata_fromMemoryToDisk ( void ){
	char b[BLOCK_SIZE]; //buffer
	// To write from sBlock, i_map, b_map into disk
	memset(b, 0, BLOCK_SIZE) ;
  memmove(b, &(sBlock), sizeof(SuperblockType)) ;
	memmove(b+sizeof(SuperblockType), &(i_map), sizeof(TypeInodeMap));
	memmove(b+sizeof(SuperblockType)+sizeof(TypeInodeMap), &(b_map), sizeof(TypeBlockMap)) ;
	bwrite(DEVICE_IMAGE, 0, b);
	// To write the i-nodes to disk
	int remaining_inodes  = NUM_INODES;
	for (int i=0; i<sBlock.numInodesBlocks-1; i++){
		memset(b, 0, BLOCK_SIZE) ;
    memmove(b, &(inodes[i*sBlock.inodesPerBlock]), sBlock.inodesPerBlock*sizeof(InodeDiskType));
		bwrite(DEVICE_IMAGE, sBlock.rootInodeBlock+i, b);
		remaining_inodes = remaining_inodes - sBlock.inodesPerBlock;
	}
	memset(b, 0, BLOCK_SIZE) ;
	memmove(b, &(inodes[NUM_INODES-remaining_inodes]), remaining_inodes*sizeof(InodeDiskType));
	bwrite(DEVICE_IMAGE, sBlock.rootInodeBlock+sBlock.numInodesBlocks-1, b);
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
			memset(&(inodes[i]),0,sizeof(InodeDiskType));
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

int bfree(int block_id){
	//check inode_id vality
	if(block_id>=sBlock.numDataBlocks)
		return -1;
	//free block
	b_map[block_id] = 0;
	return 0;
}

int bmap ( int inode_id, int offset )
{
	char b[BLOCK_SIZE];
	int logic_block ;

	if(inode_id>sBlock.numInodes){
		return -1;
	}
	//logic block associated
	logic_block = offset / BLOCK_SIZE;
	if (logic_block > (1+BLOCK_SIZE))
		return -1;
	// return the associated direct block reference
	if (0 == logic_block)
		return inodes[inode_id].directBlock[0];
	// return the associated inode block
	bread(DEVICE_IMAGE, sBlock.rootInodeBlock, b);
	return b[logic_block - 1] ; // 1 direct block -> x-1
}


/*REQUIRED FUNCTIONS*///////////////////////////////////////////////////////////////////////////////////////////

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{
	//Checking correct device size
	if(deviceSize<MIN_DISK_SIZE || deviceSize>MAX_DISK_SIZE){
		return -1;
	}
	//Giving values to superblock attributes
	sBlock.magicNumber = 100366919;
	sBlock.numInodes = NUM_INODES;
	sBlock.numInodesBlocks = (NUM_INODES*sizeof(InodeDiskType)+BLOCK_SIZE-1)/BLOCK_SIZE; //Number of inodes blocks
	sBlock.rootInodeBlock = 1;
	sBlock.inodesPerBlock = BLOCK_SIZE / sizeof(InodeDiskType) ; //Number of max inodes in a block
	sBlock.numDataBlocks = NUM_DATA_BLOCKS;
	sBlock.firstMapsBlock = 0; //Block maps stored in superblock
	sBlock.firstDataBlock  = 1 +sBlock.numInodesBlocks;
	sBlock.deviceSize = deviceSize;

	//BORRAR BORRAR BORRAR BORRAR BORRAR BORRAR BORRAR BORRAR BORRAR
	printf("SuperblockType:%ld\n", sizeof(SuperblockType));
	printf("InodeDiskType:%ld\n", sizeof(InodeDiskType));
	printf("InodesDiskType:%ld\n", sizeof(InodesDiskType));
	printf("TypeInodeMap:%ld\n", sizeof(TypeInodeMap));
	printf("TypeBlockMap:%ld\n", sizeof(TypeBlockMap));
	printf("numInodesBlocks:%d\n", sBlock.numInodesBlocks);
	//BORRAR BORRAR BORRAR BORRAR BORRAR BORRAR BORRAR BORRAR BORRAR

	for(int i=0; i<sBlock.numInodes; i++){
		i_map[i]=0; //free
	}
	for(int i=0; i<sBlock.numDataBlocks; i++){
		b_map[i]=0; //free
	}
	for(int i=0; i<sBlock.numInodes; i++){
		memset(&(inodes[i]), 0, sizeof(InodeDiskType)); //Set to 0
	}
	char b[BLOCK_SIZE];
	metadata_fromMemoryToDisk();
	memset(b, 0, BLOCK_SIZE) ; //Block set to 0
	//Initializing data blocks to 0
  for (int i=0; i < sBlock.numDataBlocks; i++) {
		bwrite(DEVICE_IMAGE, sBlock.firstDataBlock + i, b) ;
  }
	return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{
	//Checking if system is already mounted
	if(1==mounted){
		return -1;
	}
	metadata_fromDiskToMemory();
	//Checking if magic number is correct
	if(100366919 != sBlock.magicNumber){
		return -1;
	}
	mounted = 1;
	return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	//Checking if system is already unmounted
	if(0 == mounted){
		printf("Error: FS already unmounted\n");
		return -1;
	}
	//Checking if there are open files. If so, close them
	for (int i=0; i<sBlock.numInodes; i++){
		if(inodes_x[i].open){
			closeFile(i);
		}
	}
	metadata_fromMemoryToDisk();
	mounted = 0;
	return 0;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{
	int inode_id ;
    // check file exist
    inode_id = namei(fileName) ;
    if (inode_id >= 0) {
        return -1 ;
    }
		//If not, get a free inode checking it is valid
    inode_id = ialloc() ;
    if (inode_id < 0) {
				return -2;
    }
		//Give value to file attributes
    strcpy(inodes[inode_id].name, fileName) ;
    inodes[inode_id].type = T_FILE ;
		//Initialize direct blocks (max 5) to 255, as we can have 240 blocks as maximum, so we now it has not been used yet
	  for (int i=0; i<5; i++){
			inodes[inode_id].directBlock[i] = 255;
		}
    inodes_x[inode_id].f_seek = 0 ;
    inodes_x[inode_id].open  = 1 ;
		return 0 ;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */

int removeFile(char *fileName)
{
	int inode_id ;
	 //get inode id from name
	 inode_id = namei(fileName) ;
	 if (inode_id < 0) {
			 return -1;
	 }else{
		 //Delete from disk
		 char b[BLOCK_SIZE];
		 memset(b, 0, BLOCK_SIZE);
		 bwrite(DEVICE_IMAGE, inodes[inode_id].directBlock[0], b);
		 //Delete metadata
		 bfree(inodes[inode_id].directBlock[0]) ;
		 memset(&(inodes[inode_id]), 0, sizeof(InodeDiskType)) ;
		 ifree(inode_id) ;
		 return 0 ;
	 }
	 //In case of different error
	 return -2;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */

int openFile(char *fileName)
{
	//get inode id from name
	int inode_id ;
	inode_id = namei(fileName);
	if (inode_id < 0){
		return -1;
	}
	else{
		//If file is a symbolic link, get the linked file and open it.
		if(inodes[inode_id].type==T_LINK){
			char b[BLOCK_SIZE];
			//CUANDO CREEMOS ENLACE SIMBOLICO RESERVAR BLOQUE DIRECTO Y GUARDAR A LO QUE APUNTA(NOMBRE FICHERO)
			bread(DEVICE_IMAGE, inodes[inode_id].directBlock[0], b);
			return openFile((char *)b);

		}
		//Open file and set seek pointer to the beggining
		inodes_x[inode_id].f_seek = 0 ;
		inodes_x[inode_id].open  = 1 ;
		return inode_id ;
	}
	//In case of different error
	return -2;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{ //Checking valid fd
	if ((fileDescriptor < 0) || (fileDescriptor >= sBlock.numInodes)){
		return -1 ;
	}
	//Checking if file exists
	if(strlen(inodes[fileDescriptor].name)==0){
		return -1;
	}
	//Checking if file is close
	if(inodes_x[fileDescriptor].open==0){
		return 0;
	}
	//Set seek pointer to the beggining of the file and close it.
	inodes_x[fileDescriptor].f_seek = 0 ;
	inodes_x[fileDescriptor].open  = 0 ;
	//Update data in disk
	metadata_fromMemoryToDisk();
	return 0 ;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
		char b[BLOCK_SIZE]; //Buffer
		int b_id; //Block id
		//Check params
		if ( (fileDescriptor < 0) || (fileDescriptor >= sBlock.numInodes) ){
				return -1 ;
		}
		//Checking if file exists
		if(strlen(inodes[fileDescriptor].name)==0){
			 return -1;
		}
		//Checking if file is open
		if(inodes_x[fileDescriptor].open == 0){
			 return -1;
		 }
		// ajust size in case number of bytes to read from seek pointer is greater than size
		if (inodes_x[fileDescriptor].f_seek+numBytes > inodes[fileDescriptor].size) {
				numBytes = inodes[fileDescriptor].size - inodes_x[fileDescriptor].f_seek ;
		}
		//numBytes = 0;
		//If there are no bytes to read, it has finished
		if (numBytes <= 0) {
				return 0 ;
		}
		//get block
		int remaining_bytes = numBytes;
		//Read block while there are remaining bytes to read
		while(remaining_bytes>0){
			//Bring block checking it is correct
			b_id = bmap(fileDescriptor, inodes_x[fileDescriptor].f_seek) ;
			if (b_id < 0) {
					return -1 ;
			}
			//read block
			bread(DEVICE_IMAGE, sBlock.firstDataBlock+b_id, b);
			//Number of bytes that can be read in block
			int remaining_bytes_in_block  = BLOCK_SIZE-(inodes_x[fileDescriptor].f_seek%BLOCK_SIZE);
			//Take the minimum number between remaining bytes to read and remaiming bytes to read in block
			//Ternary operator: returns minimum one
			remaining_bytes_in_block = (remaining_bytes_in_block<remaining_bytes)?remaining_bytes_in_block:remaining_bytes;
			//Number of bytes that has been already read
			int already_read_bytes = numBytes - remaining_bytes;
			//Copy bytes to buffer
	    memmove(buffer+already_read_bytes,
							b+(inodes_x[fileDescriptor].f_seek%BLOCK_SIZE),
							remaining_bytes_in_block);
			//Update seek pointer and remaining bytes
	    inodes_x[fileDescriptor].f_seek += remaining_bytes_in_block ;
			remaining_bytes = remaining_bytes - remaining_bytes_in_block;
		}
     return numBytes ;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes){
	char b[BLOCK_SIZE]; //Buffer
  int b_id; //Block id
	//Check params
  if ((fileDescriptor < 0) || (fileDescriptor >= sBlock.numInodes) ){
		return -1 ;
	}
	//Checking if file exists
	if(strlen(inodes[fileDescriptor].name)==0){
		return -1;
	}
	//Checking if file is open
	if(inodes_x[fileDescriptor].open == 0){
		return -1;
	}
	//If bytes to write are greater than block size, update them to the difference between block size and the seek pointer position
	if (inodes_x[fileDescriptor].f_seek+numBytes > BLOCK_SIZE) {
         numBytes = BLOCK_SIZE - inodes_x[fileDescriptor].f_seek ;
  }
	//If there are no bytes to read, it has finished
  if (numBytes <= 0) {
		return 0 ;
  }
  //en: get block
	int remaining_bytes =  numBytes;
	//Write block while there are remaining bytes to write
	while(remaining_bytes>0){
		//Bring block checking it is correct and that it has not been used before
		b_id = bmap(fileDescriptor, inodes_x[fileDescriptor].f_seek) ;
		if (255 == b_id){
			b_id = alloc() ;
	    if (b_id < 0) {
				return -1 ;
	     }
	     inodes[fileDescriptor].directBlock[0] = b_id ;
	  }
		//read block + modify some bytes + write block
    bread(DEVICE_IMAGE, sBlock.firstDataBlock+b_id, b) ;
		int remaining_bytes_in_block  = BLOCK_SIZE-(inodes_x[fileDescriptor].f_seek%BLOCK_SIZE);
		remaining_bytes_in_block = (remaining_bytes_in_block<remaining_bytes)?remaining_bytes_in_block:remaining_bytes;
		int already_written_bytes = numBytes - remaining_bytes;
		memmove(b+(inodes_x[fileDescriptor].f_seek%BLOCK_SIZE), //update position within block
						buffer+already_written_bytes,
						remaining_bytes_in_block);
   	bwrite(DEVICE_IMAGE, sBlock.firstDataBlock+b_id, b) ;
		//Update seek pointer and remaining bytes
		inodes_x[fileDescriptor].f_seek += numBytes;
		remaining_bytes = remaining_bytes - remaining_bytes_in_block;
		//If seek pointer is greater than size, update it
		if(inodes_x[fileDescriptor].f_seek>inodes[fileDescriptor].size){
			 inodes[fileDescriptor].size = inodes_x[fileDescriptor].f_seek;
		}
	}
	return numBytes;
}


/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence){
	//checking the paramenter of fileDescriptor. MAX:48
	if ((fileDescriptor<0)||(fileDescriptor>=sBlock.numInodes)){
		return -1 ;
	}
	//Checking if file is open
	if(inodes_x[fileDescriptor].open == 0){
		return -1;
	}
	switch (whence) {
		//Put seek pointer at the begginig of file
		case FS_SEEK_BEGIN:
			inodes_x[fileDescriptor].f_seek=0;
			break;
		//Put seek pointer at the end of file
		case FS_SEEK_END:
			inodes_x[fileDescriptor].f_seek= inodes[fileDescriptor].size-1;
			break;
		case FS_SEEK_CUR:
			//In case the offset is 0 it as finished
			if(offset==0){
				return 0 ;
			//possitive or negative offset not allowing f_seek out of file limits
			}else if((inodes_x[fileDescriptor].f_seek+offset)>=0&&(inodes_x[fileDescriptor].f_seek+offset)<MAX_FILE_SIZE){
				inodes_x[fileDescriptor].f_seek+=offset;
			//Out of file limits. Error
			}else{
				return -1;
			}
			break;
	}
	return 0;
}

/*
 * @brief	Checks the integrity of the file.
 * @return	0 if success, -1 if the file is corrupted, -2 in case of error.
 */

int checkFile (char * fileName){
	int inode_id ;
	inode_id = namei(fileName);
	//File does not exist
	if (inode_id < 0 || inode_id >= sBlock.numInodes){
		return -2;
	}
	//File is opened
	if(inodes_x[inode_id].open==1){
			printf("%s\n", "OPEN FILE");
			return -2;
	}
	//File do not have integrity
	if(!(inodes[inode_id].CRC[0])){
			printf("%s\n", "FILE DO NOT HAVE INTEGRITY");
			return -2;
	}
	/*uint32_t original_crc = inodes[inode_id].CRC[0];
	char b[BLOCK_SIZE];
	bread(DEVICE_IMAGE, sBlock.firstDataBlock, b);
	uint32_t check_crc =  CRC32(b, sizeof(b));
	if(original_crc==check_crc){
		printf("%s\n", "SON IGUALES");
	}else{
		printf("ORIGINAL:%x\n", original_crc);
		printf("A COMPROBAR%x\n", check_crc);
	}*/
  return -2;
}

/*
 * @brief	Include integrity on a file.
 * @return	0 if success, -1 if the file does not exists, -2 in case of error.
 */
//HABILITAR INTEGRIDAD - No hace nada más
int includeIntegrity (char * fileName)
{
	int inode_id ;
	inode_id = namei(fileName);

	//Check if file exists
	if (inode_id < 0 || inode_id >= sBlock.numInodes){
		return -1;
	}

	//Check if file is opened; crc cannot be calculated
	if(inodes_x[inode_id].open==1){
			printf("%s\n", "OPEN FILE");
			return -2;
	}

	//calcular crc de cada bloque o cargar todo el fichero a memoria (bloque de tamaño máximo de lo que ya esta escrito) --> casting
	//as we include integrity to all blocks, if the first one do not have, any will do
	if(!(inodes[inode_id].CRC[0])){
		//loop to traverse the block
		/*int size = inodes[inode_id].size;
		char b[size];
		readFile(inode_id, b, size);*/
		//inodes[inode_id].CRC[0]=CRC32(b, sizeof(b));
		for(int i=0; i<sBlock.numDataBlocks; i++)
		{

			//memmove(&(inodes[i*sBlock.inodesPerBlock]), b, sBlock.inodesPerBlock*sizeof(InodeDiskType));

		}

		printf("%s\n", "PUT CRC");
	}

    return 0;
}

/*
 * @brief	Opens an existing file and checks its integrity
 * @return	The file descriptor if possible, -1 if file does not exist, -2 if the file is corrupted, -3 in case of error
 */
int openFileIntegrity(char *fileName)
{
	//Usar open dentro!! cuando hayamoms comprobado que tiene integridad
    return -2;
}

/*
 * @brief	Closes a file and updates its integrity.
 * @return	0 if success, -1 otherwise.
 */
int closeFileIntegrity(int fileDescriptor)
{
	//Usar close dentro!! cuando hayamos comprobado que tiene integridad
    return -1;
}

/*
 * @brief	Creates a symbolic link to an existing file in the file system.
 * @return	0 if success, -1 if file does not exist, -2 in case of error.
 */
int createLn(char *fileName, char *linkName)
{
	int inode_id ;
    //check file to link exists
    inode_id = namei(fileName) ;
    if (inode_id < 0) {
        return -1 ;
    }
		//Search a free inode for link
    inode_id = ialloc() ;
    if (inode_id < 0) {
				return -2;
    }
		//Give values to its attributes
    strcpy(inodes[inode_id].name, linkName) ;
    inodes[inode_id].type = T_LINK ;
		//Search free inode for direct Block of link checking it is correct
    inodes[inode_id].directBlock[0] = alloc();
		if(inodes[inode_id].directBlock[0]<0){
			return -2;
		}
		//Initialize a block to 0 to copy on it the linked file and writes it into the device
		char b[BLOCK_SIZE];
		memset(b, 0, BLOCK_SIZE);
		strcpy(b, fileName);
		bwrite(DEVICE_IMAGE, inodes[inode_id].directBlock[0], b);
		return 0 ;
}

/*
 * @brief 	Deletes an existing symbolic link
 * @return 	0 if the file is correct, -1 if the symbolic link does not exist, -2 in case of error.
 */

 //FALTA -2 IN CASE OF ERROR
int removeLn(char *linkName)
{
	int inode_id ;
  //get inode id from name checking it is valid
	inode_id = namei(linkName) ;
  if (inode_id < 0) {
			return -1;
  }
	else{
		//Delete on disk setting block to 0
		char b[BLOCK_SIZE];
		memset(b, 0, BLOCK_SIZE);
		bwrite(DEVICE_IMAGE, inodes[inode_id].directBlock[0], b);
		//Delete metadata
	  bfree(inodes[inode_id].directBlock[0]) ;
	  memset(&(inodes[inode_id]), 0, sizeof(InodeDiskType)) ;
	  ifree(inode_id) ;

		return 0 ;
	}
	return -2;
}
