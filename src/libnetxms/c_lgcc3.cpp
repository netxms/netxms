/* c_lgcc3.cpp -- make g++-3.2 programs linkable with gcc-2.95
 * by pts@fazekas.hu at Wed Dec 11 16:33:03 CET 2002
 *
 * This file is not sam2p-specific. See also AC_PTS_GCC_LINKS_CXX in file
 * aclocal.m4.
 */

#include <stdio.h>
#include <stdlib.h>

/* vvv make these a macro to make them fast */
#define XMALLOC_CODE() (unsigned len) { \
  void *p=malloc(len); \
  if (p==0) { fputs("out of memory\n",stderr); abort(); } \
  return p; \
}
#define XFREE_CODE() (void* p) { if (p!=0) free(p); }

/* at Wed Dec 11 16:31:42 CET 2002
 * This helper is here for gcc-2.95: c_lgcc3.cpp should be compiled by
 * gcc-2.95, remaining files are compiled by g++-3.2
 */
void* __cxa_pure_virtual=0;
extern "C" void  _ZdlPv (void*);
extern "C" void* _Znwj (unsigned);
extern "C" void  _ZdaPv (void*);
extern "C" void* _Znaj (unsigned);
void  _ZdlPv XFREE_CODE()
void* _Znwj  XMALLOC_CODE()
void  _ZdaPv XFREE_CODE()
void* _Znaj  XMALLOC_CODE()

