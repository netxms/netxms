/*================================================================
** Copyright 2000, Clark Cooper
** Copyright 2009, Victor Kirhenshtein
** All rights reserved.
**
** This is free software. You are permitted to copy, distribute, or modify
** it under the terms of the MIT/X license (contained in the COPYING file
** with this distribution.)
*/

#ifndef NWCONFIG_H
#define NWCONFIG_H

#include <string.h>

#define XML_NS 1
#define XML_DTD 1
#define XML_CONTEXT_BYTES 1024

/* all NetWare platforms are little endian */
#define BYTEORDER 1234

/* NetWare has memmove() available. */
#define HAVE_MEMMOVE

#endif /* ndef NWCONFIG_H */
