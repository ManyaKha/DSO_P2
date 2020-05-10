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
	for (int i =0; i<(sBlock.numInodesBlocks-1); i++) //numInodesBlocks= 32
	{
		bread(DEVICE_IMAGE, sBlock.rootInodeBlock+i, b);
		memmove(&(inodes[i*sBlock.inodesPerBlock]), b, sBlock.inodesPerBlock*sizeof(InodeDiskType));
		remaining_inodes = remaining_inodes - sBlock.inodesPerBlock;
	}
	bread(DEVICE_IMAGE, sBlock.rootInodeBlock+sBlock.numInodesBlocks-1, b);
	//(sBlock.numInodesBlocks-2)*sBlock.inodesPerBlock
	printf("Llamo memmove:inodes[%d],%ld bytes\n", NUM_INODES-remaining_inodes, remaining_inodes*sizeof(InodeDiskType));

	memmove(&(inodes[NUM_INODES-remaining_inodes]), b, remaining_inodes*sizeof(InodeDiskType));
	return 1;
}

int metadata_fromMemoryToDisk ( void ){
	char b[BLOCK_SIZE];

	// To write from sBlock, i_map, b_map into disk
	memset(b, 0, BLOCK_SIZE) ;
  memmove(b, &(sBlock), sizeof(SuperblockType)) ;
	memmove(b+sizeof(SuperblockType), &(i_map), sizeof(TypeInodeMap));
	memmove(b+sizeof(SuperblockType)+sizeof(TypeInodeMap), &(b_map), sizeof(TypeBlockMap)) ;
	bwrite(DEVICE_IMAGE, 0, b);

	// To write the i-node to disk
	//int inodesPerBlock = BLOCK_SIZE / sizeof(InodeDiskType) ;
	//TO-DO controlar en la ultima iteración si hay menos inodes/block. Para no desperdiciar espacio. Intentar que todos los bloques tengan el mismo numero de inodos.
	int remaining_inodes  = NUM_INODES;
	for (int i=0; i<sBlock.numInodesBlocks-1; i++){
		memset(b, 0, BLOCK_SIZE) ;
    memmove(b, &(inodes[i*sBlock.inodesPerBlock]), sBlock.inodesPerBlock*sizeof(InodeDiskType));
		bwrite(DEVICE_IMAGE, sBlock.rootInodeBlock+i, b);
		remaining_inodes = remaining_inodes - sBlock.inodesPerBlock;
	}
	memset(b, 0, BLOCK_SIZE) ;
	memmove(b, &(inodes[NUM_INODES-remaining_inodes]), remaining_inodes*sizeof(InodeDiskType));
	printf("Llamo memmove_write:inodes[%d],%ld bytes\n", NUM_INODES-remaining_inodes, remaining_inodes*sizeof(InodeDiskType));
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
	//int b[BLOCK_SIZE/4] ;
	char b[BLOCK_SIZE];
	int logic_block ;

	if(inode_id>sBlock.numInodes){
		return -1;
	}
	//logic block associated
	logic_block = offset / BLOCK_SIZE;
	//if (logic_block > (1+BLOCK_SIZE/4))
	if (logic_block > (1+BLOCK_SIZE))
		return -1;
	// return the associated direct block reference
	if (0 == logic_block)
		return inodes[inode_id].directBlock[0];
	//NOSOTRAS NO TENEMOS INDIRECTOS. HAY QUE CAMBIARLO.
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
	if(deviceSize<MIN_DISK_SIZE || deviceSize>MAX_DISK_SIZE){
		printf("%s\n", "Storage capacity is exceeded or underestimated");
		return -1;
	}
	sBlock.magicNumber = 100366919;
	sBlock.numInodes = NUM_INODES;
	//REPARTIR INODES EN PARTES IGUALES
	sBlock.numInodesBlocks = (NUM_INODES*sizeof(InodeDiskType)+BLOCK_SIZE-1)/BLOCK_SIZE; //BLOCK-1 me iguala para redondear
	sBlock.rootInodeBlock = 1;
	sBlock.inodesPerBlock = BLOCK_SIZE / sizeof(InodeDiskType) ; //2048/48= 32 max
	//sBlock.numDataBlocks = deviceSize/BLOCK_SIZE;/*?*/
	sBlock.numDataBlocks = NUM_DATA_BLOCKS;
	sBlock.firstMapsBlock = 0; //ESTAN EN SUPERBLOQUE
	sBlock.firstDataBlock  = 1 +sBlock.numInodesBlocks;
	sBlock.deviceSize = deviceSize;

	printf("SuperblockType:%ld\n", sizeof(SuperblockType));
	printf("InodeDiskType:%ld\n", sizeof(InodeDiskType));
	printf("InodesDiskType:%ld\n", sizeof(InodesDiskType));
	printf("TypeInodeMap:%ld\n", sizeof(TypeInodeMap));
	printf("TypeBlockMap:%ld\n", sizeof(TypeBlockMap));
	printf("numInodesBlocks:%d\n", sBlock.numInodesBlocks);



	for(int i=0; i<sBlock.numInodes; i++){
		i_map[i]=0; //free
	}
	for(int i=0; i<sBlock.numDataBlocks; i++){
		b_map[i]=0; //free
	}
	for(int i=0; i<sBlock.numInodes; i++){
		memset(&(inodes[i]), 0, sizeof(InodeDiskType));
	}

	char b[BLOCK_SIZE];
	metadata_fromMemoryToDisk();
	memset(b, 0, BLOCK_SIZE) ;
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
	if(1==mounted){
		printf("Error: System file already mounted.\n Execute unmountFS() to continue\n");
		return -1;
	}
	metadata_fromDiskToMemory();
	if(100366919 != sBlock.magicNumber){
		printf("%s\n", "Wrong magic number");
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
	if(0 == mounted){
		printf("Error: FS already unmounted\n");
		return -1;
	}
	for (int i=0; i<sBlock.numInodes; i++){
		//if(i ==  inodes_x[i].open){
		if(inodes_x[i].open){
			printf("Error: There are open files%d\n", i);
			return -1;
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

    // es: comprueba si existe el fichero
    // en: check file exist
    inode_id = namei(fileName) ;
    if (inode_id >= 0) {
				printf("%s\n", "File already exists");
        return -1 ;
    }

    inode_id = ialloc() ;
    if (inode_id < 0) {
        //return inode_id ;
				return -2;
    }

    strcpy(inodes[inode_id].name, fileName) ;
    inodes[inode_id].type = T_FILE ;
    inodes[inode_id].directBlock[0] = 255 ;
    inodes_x[inode_id].f_seek = 0 ;
    inodes_x[inode_id].open  = 1 ;
	inodes[inode_id].CRC[0] = 0;
    //return inode_id ;
		return 0 ;
	//return -2;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
 //TO-DO: -2 – In case of other errors.
int removeFile(char *fileName)
{
	int inode_id ;

     // es: obtener inodo a partir del nombre
     // en: get inode id from name
     inode_id = namei(fileName) ;
     if (inode_id < 0) {
         //return inode_id ;
				 return -1;
     }

     bfree(inodes[inode_id].directBlock[0]) ;
     memset(&(inodes[inode_id]), 0, sizeof(InodeDiskType)) ;
     ifree(inode_id) ;

    return 0 ;
	//return -2;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
 //TO-DO: -2 – In case of other errors.
int openFile(char *fileName)
{
	int inode_id ;
	inode_id = namei(fileName);
	if (inode_id < 0){
		return -1;
		//return inode_id;
	}
	//Si inodes[inode_id].type ==  enlace_simbolico
	//Hallar el nombre al que apunta el enlace_simbolico --> Cuidado al crear los datos del enlace
	//return openFile(nombre de lo apuntado)



	inodes_x[inode_id].f_seek = 0 ;
  inodes_x[inode_id].open  = 1 ;
  return inode_id ;
	//return -2;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	if ((fileDescriptor < 0) || (fileDescriptor >= sBlock.numInodes)){
		return -1 ;
	} if(strlen(inodes[fileDescriptor].name)==0){
		printf("NO EXISTE\n");
		return -1;
	}if(inodes_x[fileDescriptor].open==0){
		printf("%s\n", "ALREADY CLOSED FILE");
		return 0;
	}
		inodes_x[fileDescriptor].f_seek = 0 ;
		inodes_x[fileDescriptor].open  = 0 ;

		return 0 ;
	//return -1;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
		char b[BLOCK_SIZE] ;
		int b_id ;

		// es: comprobar parámetros
		// en: check params
		if ( (fileDescriptor < 0) || (fileDescriptor >= sBlock.numInodes) )
		{
				return -1 ;
		}if(strlen(inodes[fileDescriptor].name)==0){
			 printf("NO EXISTE\n");
			 return -1;
		}if(inodes_x[fileDescriptor].open == 0){
			 printf("ESTA CERRADO\n");
			 return -1;
		 }

		// es: reajusta el tamaño
		// en: ajust size
		if (inodes_x[fileDescriptor].f_seek+numBytes > inodes[fileDescriptor].size) {
				numBytes = inodes[fileDescriptor].size - inodes_x[fileDescriptor].f_seek ;
		}
		if (numBytes <= 0) {
				return 0 ;
		}

		// es: obtener bloque
		// en: get block
		int remaining_bytes = numBytes;
		while(remaining_bytes>0){
			b_id = bmap(fileDescriptor, inodes_x[fileDescriptor].f_seek) ;
			if (b_id < 0) {
					return -1 ;
			}
			bread(DEVICE_IMAGE, sBlock.firstDataBlock+b_id, b) ;
			//BLOCK_SIZE-inodes_x[fileDescriptor].f_seek --> numero de bytes que "puedo" escribir/leer todavía en el bloque
			//CUIDADO: f_seek puede ser > BLOCK_SIZE --> mod
			int remaining_bytes_in_block  = BLOCK_SIZE-(inodes_x[fileDescriptor].f_seek%BLOCK_SIZE);
			//Operador ternario: devuelve minimo
			remaining_bytes_in_block = (remaining_bytes_in_block<remaining_bytes)?remaining_bytes_in_block:remaining_bytes;
			int already_written_bytes = numBytes - remaining_bytes;
	    memmove(buffer+already_written_bytes,
							b+inodes_x[fileDescriptor].f_seek,
							remaining_bytes_in_block);
	    inodes_x[fileDescriptor].f_seek += remaining_bytes_in_block ;
			remaining_bytes = remaining_bytes - remaining_bytes_in_block;
		}

     return numBytes ;
	//return -1;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	 char b[BLOCK_SIZE] ;
     int b_id ;

     // en: check params
     if ( (fileDescriptor < 0) || (fileDescriptor >= sBlock.numInodes) )
     {
         return -1 ;
     }
	if(strlen(inodes[fileDescriptor].name)==0){
		printf("NO EXISTE\n");
		return -1;
	}
	if(inodes_x[fileDescriptor].open == 0){
		printf("ESTA CERRADO\n");
		return -1;
	}/*if (inodes_x[fileDescriptor].f_seek+numBytes > BLOCK_SIZE) {
         numBytes = BLOCK_SIZE - inodes_x[fileDescriptor].f_seek ;
     }*/
		 
	if(inodes_x[fileDescriptor].f_seek+numBytes > inodes[fileDescriptor].size){
		numBytes = inodes[fileDescriptor].size - inodes_x[fileDescriptor].f_seek;
	}
     if (numBytes <= 0) {
         return 0 ;
     }
	 
     // en: get block
	int remaining_bytes =  numBytes;
	while(remaining_bytes>0){
		b_id = bmap(fileDescriptor, inodes_x[fileDescriptor].f_seek) ;
		if (255 == b_id) 
		{
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
		memmove(buffer+already_written_bytes,
							b+inodes_x[fileDescriptor].f_seek,
							remaining_bytes_in_block);
		bwrite(DEVICE_IMAGE, sBlock.firstDataBlock+b_id, b) ;
		inodes_x[fileDescriptor].f_seek += remaining_bytes;
		remaining_bytes = remaining_bytes - remaining_bytes_in_block;
		if(inodes_x[fileDescriptor].f_seek>inodes[fileDescriptor].size)
		{
			 inodes[fileDescriptor].size = inodes_x[fileDescriptor].f_seek+1 ;
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
	//SWITCH CASE.
	if ((fileDescriptor<0)||(fileDescriptor>=sBlock.numInodes)){
		return -1 ;
	}if(inodes_x[fileDescriptor].open == 0){
		return -1;
	}

	switch (whence) {
		case FS_SEEK_BEGIN:
			inodes_x[fileDescriptor].f_seek=0;
			printf("BEGINING F_SEEK:%d\n", inodes_x[fileDescriptor].f_seek);
			break;
		case FS_SEEK_END:
			printf("END:%d\n", inodes[fileDescriptor].size);
			inodes_x[fileDescriptor].f_seek= inodes[fileDescriptor].size-1;
			printf("END F_SEEK:%d\n", inodes_x[fileDescriptor].f_seek);
			break;
		case FS_SEEK_CUR:
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

	/*if(whence==FS_SEEK_BEGIN){ //FS_SEEK_BEGIN: reference pointed to the beginning of the file
		inodes_x[fileDescriptor].f_seek=0;
		printf("BEGINING F_SEEK:%d\n", inodes_x[fileDescriptor].f_seek);
	//EL FINAL DEL ARCHIVO ES EL TAMAÑO TOTAL O EL ULTIMO BYTE ESCRITO?
	}else if(whence==FS_SEEK_END){ //FS_SEEK_END: reference pointed to the end of the file
		printf("END:%d\n", inodes[fileDescriptor].size);
		inodes_x[fileDescriptor].f_seek= inodes[fileDescriptor].size-1;
		printf("END F_SEEK:%d\n", inodes_x[fileDescriptor].f_seek);
	}*/
	//checking the value of whence
	//whence:constant value acting as reference for the seek operation
	//offset:no. of byte to displace the pointer
/*	else if(whence==FS_SEEK_CUR){ //FS_SEEK_CUR: current position
		//If offset is 0, seek pointer do not displace from FS_SEEK_CUR
		if(offset==0){
			return 0 ;
		//possitive or negative offset not allowing f_seek out of file limits
		}else if((inodes_x[fileDescriptor].f_seek+offset)>=0&&(inodes_x[fileDescriptor].f_seek+offset)<MAX_FILE_SIZE){
			inodes_x[fileDescriptor].f_seek+=offset;
		//Out of file limits. Error
		}else{
			return -1;
		}
	}*/
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

int includeIntegrity (char * fileName)
{
	int inode_id ;
	inode_id = namei(fileName);
	int b_id;
	char b[BLOCK_SIZE] ;

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
	int remaining_bytes= inodes[inode_id].size;
	int i;
	if(!(inodes[inode_id].CRC[0])){
		//loop to traverse the block
		/*int size = inodes[inode_id].size;
		char b[size];
		readFile(inode_id, b, size);*/
		//inodes[inode_id].CRC[0]=CRC32(b, sizeof(b));
		while(remaining_bytes>0)
		{
			b_id = bmap(inode_id, inodes_x[inode_id].f_seek) ;
			if (b_id < 0) {
					return -1 ;
			}
			int buffer=bread(DEVICE_IMAGE, sBlock.firstDataBlock+b_id, b) ;
			inodes[inode_id].CRC[i]=CRC32((const unsigned char*)buffer,inodes[inode_id].size);
			int remaining_bytes_in_block  = BLOCK_SIZE-(inodes_x[inode_id].f_seek%BLOCK_SIZE);
			remaining_bytes_in_block = (remaining_bytes_in_block<remaining_bytes)?remaining_bytes_in_block:remaining_bytes;
			int already_written_bytes = remaining_bytes - remaining_bytes;
	    	memmove(buffer+already_written_bytes,
							b+inodes_x[inode_id].f_seek,
							remaining_bytes_in_block);
	    	inodes_x[inode_id].f_seek += remaining_bytes_in_block ;
			remaining_bytes = remaining_bytes - remaining_bytes_in_block;
			}
			i++;
		}
		
		//printf("%s\n", "PUT CRC");

    return 0;
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
	int inode_id ;
    // en: check file exist
    inode_id = namei(fileName) ;
    if (inode_id < 0) {
				printf("%s\n", "File does not exist");
        return -1 ;
    }
    inode_id = ialloc() ;
    if (inode_id < 0) {
				return -2;
    }
    strcpy(inodes[inode_id].name, linkName) ;
    inodes[inode_id].type = T_LINK ;
    inodes[inode_id].directBlock[0] = 255 ;
    inodes_x[inode_id].f_seek = 0 ;
    inodes_x[inode_id].open  = 1 ;
		char *str = fileName;
		int numBytes =  strlen(str);
		if(writeFile(inode_id, str, numBytes)<0){
			return -1;
		}
		if(closeFile(inode_id)<0){
			return -1;
		}
		//inodes[inode_id].CRC[0] = 0;
    //return inode_id ;
		return 0 ;
	//return -2;
}

/*
 * @brief 	Deletes an existing symbolic link
 * @return 	0 if the file is correct, -1 if the symbolic link does not exist, -2 in case of error.
 */

 //FALTA -2 IN CASE OF ERROR
int removeLn(char *linkName)
{
	int inode_id ;
  // en: get inode id from name
	inode_id = namei(linkName) ;
  if (inode_id < 0) {
			return -1;
  }
  bfree(inodes[inode_id].directBlock[0]) ;
  memset(&(inodes[inode_id]), 0, sizeof(InodeDiskType)) ;
  ifree(inode_id) ;

	return 0 ;
}