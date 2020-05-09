
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	Last revision 01/04/2020
 *
 */

 /*
  * @brief 	Writes from disk all data stored into data structures
  * @return	1 if everthing is correct, -1 in case of error
  */
  int metadata_fromDiskToMemory (void);

/*
 * @brief 	Writes in disk all data stored into data structures
 * @return	1 if everthing is correct, -1 in case of error
 */
 int metadata_fromMemoryToDisk (void);


 /*
  * @brief 	Search the inode with name [path] and returns its position
  * @return	position if everything is correct, -1 if error
  */
 int namei(char *path);

 /*
  * @brief 	Search for free inodes
  * @return	return inode id if everything is correct, -1 if all are busy
  */
 int ialloc(void);

 /*
  * @brief 	Search for free blocks
  * @return	return block id if everything is correct, -1 if all are busy ocupados
  */
 int alloc(void);

 /*
  * @brief 	Frees inode with id inode_id
  * @return	0 if everything is correct, -1 in case of error
*/
 int ifree(int inode_id);

 /*
  * @brief 	Frees block with id block_id
  * @return	0 if everything is correct, -1 in case of error
*/
int bfree(int block_id);

/*
 * @brief 	Searchs the block associated to inode with inode_id id
 * @return	associated inode block if everything is correct, -1 in case of error
*/
int bmap ( int inode_id, int offset );
