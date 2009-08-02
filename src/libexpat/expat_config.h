#ifndef _expat_config_h_
#define _expat_config_h_

#include <config.h>

#undef PREFIX

#if WORD_BIGENDIAN
#define BYTEORDER 4321
#else
#define BYTEORDER 1234
#endif

#endif

