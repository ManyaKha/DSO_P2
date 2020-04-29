
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
#define MAX_FILE_SIZE 10240
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
  unsigned int magic_num;
  unsigned int inodes;
  unsigned int root_node;
  unsigned int num_blocks;
  unsigned int first_block;
  unsigned int device_size;
  char i_bitmap[5];
  char b_bitmap[5];
  char full[BLOCK_SIZE-36];
}Superblock;

 typedef struct{
   unsigned int type;
   char name[32];
   unsigned int file_size;
   unsigned int direct_block;
 }Struct_inode;

  typedef struct{
    int currentbyte;
    int open;
  }inodes_active;
