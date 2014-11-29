#ifndef __MAIN__H__
#define __MAIN__H__

#ifdef WITH_OPENSSL
#include <openssl/ssl.h>
#endif

enum
{
	PC_ERR_NONE,
	PC_ERR_BAD_PARAMS,
	PC_ERR_CONNECT,
	PC_ERR_HANDSHAKE,
	PC_ERR_INTERNAL
};

LONG H_CheckPOP3(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckPOP3(char *, UINT32, short, char *, char *, UINT32);
LONG H_CheckSSH(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckSSH(char *, UINT32, short, char *, char *, UINT32);
LONG H_CheckSMTP(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckSMTP(char *, UINT32, short, char *, UINT32);
LONG H_CheckHTTP(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckHTTP(char *, UINT32, short, char *, char *, char *, UINT32);
int CheckHTTPS(char *, UINT32, short, char *, char *, char *, UINT32);
LONG H_CheckCustom(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckCustom(char *, UINT32, short, UINT32);
LONG H_CheckTelnet(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
int CheckTelnet(char *, UINT32, short, char *, char *, UINT32);

extern char g_szDomainName[];
extern char g_szFailedDir[];

#endif // __MAIN__H__
