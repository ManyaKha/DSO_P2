/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	Last revision 01/04/2020
 *
 */

//#define MAX_NUM_FILES 48. MAX_INODE_SIZE:42 -> 132 bytes
#define NUM_INODES 48
#define NUM_DATA_BLOCKS 240
#define BLOCK_SIZE 2048
#define MIN_DISK_SIZE 230*BLOCK_SIZE
#define MAX_DISK_SIZE 300*BLOCK_SIZE
#define T_FILE 1
#define T_LINK 2

typedef struct{
  uint32_t magicNumber; /*Superblock magic number 100366919*/
  uint8_t numInodes; /*Number of inodes on the device*/
  uint8_t rootInodeBlock; /*Block number of root inode on the device*/
  uint16_t inodesPerBlock;
  uint8_t numInodesBlocks; /*Number of blocks dedicated to inodes in the device*/
  uint8_t numDataBlocks; /* Number of data blocks on the device */
  uint8_t firstDataBlock; /* Block number of the first block*/
  uint8_t firstMapsBlock; /*Block ID where maps are stored*/
  uint16_t deviceSize; /* Total device size in bytes*/
}SuperblockType;

 typedef struct{
   uint8_t type; /*T_FILE, T_LINK, integridad y links también*/
   char name[32+1]; /*File name. mAX 32 + 1(end)*/
   uint16_t size; /*File size in bytes*/
   uint8_t directBlock[5]; /*Direct block number array. Max 5 direct blocks*/
   uint32_t CRC[5]; /*Data block assigned firm*/ //NOS VALE UN CRC PARA TODO EL FICHERO ENTERO! NO HACE FALTA UNO POR BLOQUE
 }InodeDiskType;

 typedef InodeDiskType InodesDiskType[NUM_INODES];
 //inode map
 typedef uint8_t TypeInodeMap [NUM_INODES];
 //data block map
 typedef uint8_t TypeBlockMap [NUM_DATA_BLOCKS];
