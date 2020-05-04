
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
  * @brief 	Escribe en el disco los datos almacenados en las estructuras de datos
  * @return	0 si todo es correcto,-1 en caso de error
  */
 //int syncDisk(void);

 /*
  * @brief 	Busca el inodo con el nombre [path] y devuelve su posición
  * @return	numInodo si todo es correcto, -1 en caso de error
  */
 int namei(char *path);

 /*
  * @brief 	Busca inodos libres
  * @return	numInodo si todo es correcto, -1 en caso de que estén todos ocupados
  */
 int ialloc(void);

 /*
  * @brief 	Busca bloques de datos libres
  * @return	numInodo si todo es correcto, -1 en caso de que estén todos ocupados
  */
 int alloc(void);

 /*
  * @brief 	Libera bloque
  * @return	0 si todo es correcto, -1 en caso de que estén todos ocupados
*/
int bfree(int block_id);
