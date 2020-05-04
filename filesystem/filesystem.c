
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
	int16_t f_seek; //read/write seek position
	int8_t open; //0:false, 1:true
}inodes_x[NUM_INODES];


/*SYSTEM STRUCTURE*//////////////////////////////////////////////////////////////////////////////////////
SuperblockType sBlock;
TypeInodeMap i_map;
TypeBlockMap b_map;
InodesDiskType inodes;


int8_t mounted = 0;

/*AUXILIARY FUNCTIONS*///////////////////////////////////////////////////////////////////////////////////
int metadata_fromDiskToMemory (void){
	char b[BLOCK_SIZE]; //buffer

	// To read from disk into sBlock, i_map, b_map
	bread(DEVICE_IMAGE, 0, b);
	memmove(&(sBlock), b, sizeof(SuperblockType));
	memmove(&(i_map), b+sizeof(SuperblockType), sizeof(TypeInodeMap));
	memmove(&(b_map), b+sizeof(SuperblockType)+sizeof(TypeInodeMap), sizeof(TypeBlockMap));

	// To read the i-nodes to main memory
	//int inodesPerBlock = BLOCK_SIZE / sizeof(InodeDiskType) ; //2048/48= 42
	for (int i =0; i<sBlock.numInodesBlocks; i++) //numInodesBlocks= 42
	{
		bread(DEVICE_IMAGE, sBlock.rootInodeBlock+i, b);
		memmove(&(inodes[i*sBlock.inodesPerBlock]), b, sBlock.inodesPerBlock*sizeof(InodeDiskType));
	}
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
	for (int i=0; i<sBlock.numInodesBlocks; i++){
		memset(b, 0, BLOCK_SIZE) ;
    memmove(b, &(inodes[i*sBlock.inodesPerBlock]), sBlock.inodesPerBlock*sizeof(InodeDiskType));
		bwrite(DEVICE_IMAGE, sBlock.rootInodeBlock+i, b);
	}
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
	sBlock.inodesPerBlock = BLOCK_SIZE / sizeof(InodeDiskType) ; //2048/48= 42
	//sBlock.numDataBlocks = deviceSize/BLOCK_SIZE;/*?*/
	sBlock.numDataBlocks = NUM_DATA_BLOCKS;
	sBlock.firstMapsBlock = 0; //ESTAN EN SUPERBLOQUE
	sBlock.firstDataBlock  = 1 +sBlock.numInodesBlocks;
	sBlock.deviceSize = deviceSize;

	printf("SuperblockType:%ld\n", sizeof(SuperblockType));
	printf("InodeDiskType:%ld\n", sizeof(InodeDiskType));
	printf("TypeInodeMap:%ld\n", sizeof(TypeInodeMap));
	printf("TypeBlockMap:%ld\n", sizeof(TypeBlockMap));

	printf("%d\n", sBlock.numInodesBlocks);
	printf("%d\n", sBlock.rootInodeBlock);
	printf("%d\n", sBlock.inodesPerBlock);
	printf("%d\n", sBlock.numDataBlocks);
	printf("%d\n", sBlock.firstMapsBlock);
	printf("%d\n", sBlock.firstDataBlock);
	printf("%d\n", sBlock.deviceSize);


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
		if(i ==  inodes_x[i].open){
			printf("Error: There are open files\n");
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
        return -1 ;
    }

    inode_id = ialloc() ;
    if (inode_id < 0) {
        return inode_id ;
    }

    strcpy(inodes[inode_id].name, fileName) ;
    inodes[inode_id].type           = T_FILE ;
    inodes[inode_id].directBlock[0] = 255 ;
    inodes_x[inode_id].f_seek = 0 ;
    inodes_x[inode_id].open  = 1 ;

    return inode_id ;
	//return -2;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	int inode_id ;

     // es: obtener inodo a partir del nombre
     // en: get inode id from name
     inode_id = namei(fileName) ;
     if (inode_id < 0) {
         return inode_id ;
     }

     bfree(inodes[inode_id].directBlock[0]) ;
     memset(&(inodes[inode_id]), 0, sizeof(InodeDiskType)) ;
     ifree(inode_id) ;

    return 1 ;
	//return -2;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	int inode_id ;
	inode_id = namei(fileName);
	if (inode_id < 0){
		return inode_id;
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
	if ( (fileDescriptor < 0) || (fileDescriptor >= sBlock.numInodes) )
		{
				return -1 ;
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
		b_id = bmap(fileDescriptor, inodes_x[fileDescriptor].f_seek) ;
		if (b_id < 0) {
				return -1 ;
		}
		bread(DEVICE_IMAGE, sBlock.firstDataBlock+b_id, b) ;
    memmove(buffer, b+inodes_x[fileDescriptor].f_seek, numBytes) ;

     inodes_x[fileDescriptor].f_seek += numBytes ;

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

     // es: comprobar parámetros
     // en: check params
     if ( (fileDescriptor < 0) || (fileDescriptor >= sBlock.numInodes) )
     {
         return -1 ;
     }

     if (inodes_x[fileDescriptor].f_seek+numBytes > BLOCK_SIZE) {
         numBytes = BLOCK_SIZE - inodes_x[fileDescriptor].f_seek ;
     }
     if (numBytes <= 0) {
         return 0 ;
     }

     // es: obtener bloque
     // en: get block
     b_id = bmap(fileDescriptor, inodes_x[fileDescriptor].f_seek) ;
     if (255 == b_id) {
         b_id = alloc() ;
         if (b_id < 0) {
             return -1 ;
         }
         inodes[fileDescriptor].directBlock[0] = b_id ;
     }

     // es: lee bloque + actualiza algunos bytes + escribe bloque
     // en: read block + modify some bytes + write block
     bread(DEVICE_IMAGE, sBlock.firstDataBlock+b_id, b) ;
     memmove(b+inodes_x[fileDescriptor].f_seek, buffer, numBytes) ;
     bwrite(DEVICE_IMAGE, sBlock.firstDataBlock+b_id, b) ;
		 //Si el fichero es con integridad
		 //{calcular CRC de b, almacenar en inodes[fileDescriptor].CRC[b_id]}

     inodes_x[fileDescriptor].f_seek += numBytes ;
     inodes[fileDescriptor].size += numBytes ;

     return numBytes ;
	//return -1;
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


/*int main()
{
   int    ret = 1 ;
   int    fd = 1 ;
   char *str1 = "hola mundo..." ;
   char  str2[20] ;

   printf("\n") ;
   printf("Size of data structures:\n") ;
   printf(" * Size of Superblock: %ld bytes.\n", sizeof(SuperblockType)) ;
   printf(" * Size of InodeDisk:  %ld bytes.\n", sizeof(InodeDiskType)) ;
   printf(" * Size of InodeMap:   %ld bytes.\n", sizeof(TypeInodeMap)) ;
   printf(" * Size of BlockMap:   %ld bytes.\n", sizeof(TypeBlockMap)) ;

   printf("\n") ;
   printf("Tests:\n") ;

   //
   // mkfs-mount
   //
   if (ret != - 1 ){
       printf(" * nanofs_mkfs(32) -> ") ;
       ret = mkFS ( 32 );
       printf("%d\n", ret) ;
   }

   if (ret != - 1 ){
       printf(" * nanofs_mount() -> ") ;
       ret = mountFS ();
       printf("%d\n", ret) ;
   }

   //
   // creat-write-close
   //
   if (ret != - 1 ) {
       printf(" * nanofs_creat('test1.txt') -> ") ;
       ret = fd = createFile("test1.txt") ;
       printf("%d\n", ret) ;
   }

   if (ret != - 1 ) {
       printf(" * nanofs_write(%d,'%s',%ld) -> ", ret, str1, strlen(str1)) ;
       ret = writeFile(fd, str1, strlen(str1)) ;
       printf("%d\n", ret) ;
   }

   if (ret != - 1 ) {
       printf(" * nanofs_close(%d) -> ", ret) ;
       ret = closeFile (fd);
       printf("%d\n", ret) ;
   }

   //
   // open-read-close
   //
   if (ret != - 1 )   {
       printf(" * nanofs_open('test1.txt') -> ") ;
       ret = fd = openFile("test1.txt") ;
       printf("%d\n", ret) ;
   }

   if (ret != - 1 )   {
       memset(str2, 0, 20) ;
       printf(" * nanofs_read(%d,'%s',%d) -> ", ret, str2, 13) ;
       ret = readFile (fd, str2, 13 );
       printf("%d (%s)\n", ret, str2) ;
   }

   if (ret != - 1 ){
       printf(" * nanofs_close(%d) -> ", ret) ;
       ret = closeFile (fd);
       printf("%d\n", ret) ;
   }

   //
   // unlink-umount
   //
   if (ret != - 1 ){
       printf(" * nanofs_unlink('test1.txt') -> ") ;
       ret = removeFile("test1.txt") ;
       printf("%d\n", ret) ;
   }

   if (ret != - 1 ){
       printf(" * nanofs_umount() -> ") ;
       ret = unmountFS ();
       printf("%d\n", ret) ;
   }

   return 0 ;
}*/
