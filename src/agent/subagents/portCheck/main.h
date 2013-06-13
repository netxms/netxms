#ifndef __MAIN__H__
#define __MAIN__H__

enum
{
	PC_ERR_NONE,
	PC_ERR_BAD_PARAMS,
	PC_ERR_CONNECT,
	PC_ERR_HANDSHAKE
};

LONG H_CheckPOP3(const TCHAR *, const TCHAR *, TCHAR *);
int CheckPOP3(char *, UINT32, short, char *, char *, UINT32);
LONG H_CheckSSH(const TCHAR *, const TCHAR *, TCHAR *);
int CheckSSH(char *, UINT32, short, char *, char *, UINT32);
LONG H_CheckSMTP(const TCHAR *, const TCHAR *, TCHAR *);
int CheckSMTP(char *, UINT32, short, char *, UINT32);
LONG H_CheckHTTP(const TCHAR *, const TCHAR *, TCHAR *);
int CheckHTTP(char *, UINT32, short, char *, char *, char *, UINT32);
LONG H_CheckCustom(const TCHAR *, const TCHAR *, TCHAR *);
int CheckCustom(char *, UINT32, short, UINT32);
LONG H_CheckTelnet(const TCHAR *, const TCHAR *, TCHAR *);
int CheckTelnet(char *, UINT32, short, char *, char *, UINT32);

extern char g_szDomainName[];

#endif // __MAIN__H__
