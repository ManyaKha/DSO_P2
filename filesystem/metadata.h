
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
#define MAX_FILE_SIZE 10240
#define BLOCK_SIZE 2048
//MAX_DEVICE_SIZE 300 (blocks) / MIN_DEVICE_SIZE 230 (blocks)
#define MIN_DISK_SIZE 471040
#define MAX_DISK_SIZE 614400
#define T_FILE 1




#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
  if (val_)
    bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
  else
    bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}

typedef struct{
  unsigned int magicNumber; /*Superblock magic number 100366919*/
  unsigned int numBlocksInodeMap; /* Number of blocks of the inode map*/
  unsigned int numBlocksBlockMap; /* Number of blocks of the data map */
  unsigned int numInodes; /*Number of inodes on the device*/
  unsigned int rootInode; /*Block number of root inode on the device*/
  unsigned int numDataBlocks; /* Number of data blocks on the device */
  unsigned int firstDataBlock; /* Block number of the first block*/
  uint16_t int deviceSize; /* Total device size in bytes*/
}SuperblockType;

 typedef struct{
   uint8_t type; /*T_FILE or T_DIRECTORY*/
   char name[32+1]; /*File name. mAX 32 + 1(end)*/
   unsigned int size; /*File size in bytes*/
   //unsigned inodeTable[200] /*type==dir. list of inodes from the directory*/
   uint8_t directBlock[5]; /*Direct block number array. Max 5 direct blocks*/
 }InodeDiskType;

 typedef InodeDiskType InodesDiskType[NUM_INODES];
 typedef char TypeInodeMap [NUM_INODES];
 typedef char TypeBlockMap [NUM_DATA_BLOCKS];
