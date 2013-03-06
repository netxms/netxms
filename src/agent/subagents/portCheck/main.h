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
int CheckPOP3(char *, DWORD, short, char *, char *, DWORD);
LONG H_CheckSSH(const TCHAR *, const TCHAR *, TCHAR *);
int CheckSSH(char *, DWORD, short, char *, char *, DWORD);
LONG H_CheckSMTP(const TCHAR *, const TCHAR *, TCHAR *);
int CheckSMTP(char *, DWORD, short, char *, DWORD);
LONG H_CheckHTTP(const TCHAR *, const TCHAR *, TCHAR *);
int CheckHTTP(char *, DWORD, short, char *, char *, char *, DWORD);
LONG H_CheckCustom(const TCHAR *, const TCHAR *, TCHAR *);
int CheckCustom(char *, DWORD, short, DWORD);
LONG H_CheckTelnet(const TCHAR *, const TCHAR *, TCHAR *);
int CheckTelnet(char *, DWORD, short, char *, char *, DWORD);

extern char g_szDomainName[];

#endif // __MAIN__H__
