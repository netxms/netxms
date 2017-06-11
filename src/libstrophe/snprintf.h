/*
 * Copyright Patrick Powell 1995
 * This code is based on code written by Patrick Powell (papowell@astart.com)
 * It may be used for any purpose as long as this notice remains intact
 * on all source code distributions
 */

/** @file
 *  Compatibility wrappers for OSes lacking snprintf(3) and/or vsnprintf(3).
 */

#ifndef __LIBSTROPHE_SNPRINTF_H__
#define __LIBSTROPHE_SNPRINTF_H__

#define xmpp_snprintf snprintf
#define xmpp_vsnprintf vsnprintf

#endif /* __LIBSTROPHE_SNPRINTF_H__ */
