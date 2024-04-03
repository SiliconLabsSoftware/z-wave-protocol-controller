/* © 2017 Silicon Laboratories Inc.
 */
/*************************************************************************** 
*
* Description: Assorted helper defines for S2.
*              The include folder is NOT the right home for this, as it
*              is only an internal header file for libs2.
* 
* Author:   Jakob Buron 
* 
* Last Changed By:  $Author: jdo $ 
* Revision:         $Revision: 1.38 $ 
* Last Changed:     $Date: 2005/07/27 15:12:54 $ 
* 
****************************************************************************/
#ifndef MISC_H_
#define MISC_H_

#include <assert.h>

/* A memcpy with builtin buffer overrun protection
 * If the copy length is longer than maxcount, only maxcount bytes
 * will be copied. */
#define SAFE_MEMCPY(dst, src, count, maxcount) do { \
  assert(count <= maxcount); \
  memcpy(dst, src, (count > maxcount) ? maxcount : count); } while(0)
#endif /* MISC_H_ */
