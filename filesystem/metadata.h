
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

#define MAX_NUM_FILES 48
//#define MAX_LENGTH 32
#define MAX_size 10240
#define BLOCK_SIZE 2048
#define MIN_DISK_SIZE 471040
#define MAX_DISK_SIZE 614400
#define TYPE_FILE 4




#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
  if (val_)
    bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
  else
    bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}

typedef struct{
  unsigned int magicNumber; /*Superblock magic number_ 0x000D5500*/
  unsigned int numBlocksInodeMap; /* Number of blocks of the inode map*/
  unsigned int numBlocksBlockMap; /* Number of blocks of the data map */
  unsigned int numInodes; /*Number of inodes on the device*/
  unsigned int rootInode; /*Block number of root inode on the device*/
  unsigned int numDataBlocks; /* Number of data blocks on the device */
  unsigned int firstDataBLock; /* Block number of the first block*/
  unsigned int deviceSize; /* Total device size in bytes*/
  char padding[992]; /* Padding for filling a block */
}SuperblockType;

 typedef struct{
   //unsigned int type; /*T_FILE or T_DIRECTORY*/
   char name[32]; /*File name*/
   unsigned int size; /*File size in bytes*/
   //unsigned inodeTable[200] /*type==dir. list of inodes from the directory*/
   unsigned int directBlock; /*Direct block number*/
   unsigned int indirectBlock; /*Indirect block number*/
   //char padding[PADDING_I]; /*Padding for filling a block*/
 }InodeDiskType;

  typedef struct{
    int currentbyte;
    int open;
  }numInodes_active;
