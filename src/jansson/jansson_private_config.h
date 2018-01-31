#ifndef _WIN32
#include <config.h>
#endif

/* Define to 1 if you have the `close' function. */
#define HAVE_CLOSE 1

/* Define to 1 if you have the `open' function. */
#define HAVE_OPEN 1

/* Define to 1 if you have the `read' function. */
#define HAVE_READ 1

/* Define to 1 if you have the `strtoll' function. */
#undef HAVE_STRTOLL
#define HAVE_STRTOLL 1

/* Number of buckets new object hashtables contain is 2 raised to this power.
   E.g. 3 -> 2^3 = 8. */
#define INITIAL_HASHTABLE_ORDER 3

/* Define to 1 if /dev/urandom should be used for seeding the hash function */
#define USE_URANDOM 1

/* Define to 1 if CryptGenRandom should be used for seeding the hash function
   */
#define USE_WINDOWS_CRYPTOAPI 1

#ifdef _WIN32
#define HAVE_STDINT_H   1
#endif
