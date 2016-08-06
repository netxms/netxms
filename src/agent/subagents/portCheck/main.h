#ifndef __MAIN__H__
#define __MAIN__H__

#ifdef WITH_OPENSSL
#include <openssl/ssl.h>
#endif

/**
 * Service check return codes
 */
enum
{
	PC_ERR_NONE,
	PC_ERR_BAD_PARAMS,
	PC_ERR_CONNECT,
	PC_ERR_HANDSHAKE,
	PC_ERR_INTERNAL
};

/**
 * Flags
 */
#define SCF_NEGATIVE_TIME_ON_ERROR  0x0001

LONG H_CheckPOP3(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckPOP3(char *, const InetAddress&, short, char *, char *, UINT32);
LONG H_CheckSSH(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckSSH(char *, const InetAddress&, short, char *, char *, UINT32);
LONG H_CheckSMTP(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckSMTP(char *, const InetAddress&, short, char *, UINT32);
LONG H_CheckHTTP(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckHTTP(char *, const InetAddress&, short, char *, char *, char *, UINT32);
int CheckHTTPS(char *, const InetAddress&, short, char *, char *, char *, UINT32);
LONG H_CheckCustom(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckCustom(char *, const InetAddress&, short, UINT32);
LONG H_CheckTelnet(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckTelnet(char *, const InetAddress&, short, char *, char *, UINT32);

extern char g_szDomainName[];
extern char g_szFailedDir[];
extern UINT32 g_serviceCheckFlags;

#endif // __MAIN__H__
