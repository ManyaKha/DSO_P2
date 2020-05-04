
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

//#define MAX_NUM_FILES 48
#define NUM_INODES 48
//(NUM_INODES*directBlocks)
#define NUM_DATA_BLOCKS 240
//#define MAX_FILE_SIZE 10240
#define BLOCK_SIZE 2048
//MAX_DEVICE_SIZE 300 (blocks) / MIN_DEVICE_SIZE 230 (blocks)
//#define MIN_DISK_SIZE 471040
#define MIN_DISK_SIZE 230*BLOCK_SIZE
//#define MAX_DISK_SIZE 614400
#define MAX_DISK_SIZE 300*BLOCK_SIZE
#define T_FILE 1

/*#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
  if (val_)
    bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
  else
    bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}*/

typedef struct{
  uint32_t magicNumber; /*Superblock magic number 100366919*/
  //unsigned int numBlocksInodeMap; /* Number of blocks of the inode map*/
  //unsigned int numBlocksBlockMap; /* Number of blocks of the data map */
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
   uint8_t type; /*T_FILE, integridad y links también*/
   char name[32+1]; /*File name. mAX 32 + 1(end)*/
   uint16_t size; /*File size in bytes*/
   //unsigned inodeTable[200] /*type==dir. list of inodes from the directory*/
   uint8_t directBlock[5]; /*Direct block number array. Max 5 direct blocks*/
   uint32_t CRC[5]; /*Firma asociada a cada bloque de datos*/
 }InodeDiskType;

 typedef InodeDiskType InodesDiskType[NUM_INODES];
 //inode map
 typedef uint8_t TypeInodeMap [NUM_INODES];
 //data block map
 typedef uint8_t TypeBlockMap [NUM_DATA_BLOCKS];
