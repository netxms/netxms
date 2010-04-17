#ifndef __MAIN__H__
#define __MAIN__H__

enum
{
	PC_ERR_NONE,
	PC_ERR_BAD_PARAMS,
	PC_ERR_CONNECT,
	PC_ERR_HANDSHAKE
};

LONG H_CheckPOP3(const char *, const char *, char *);
int CheckPOP3(char *, DWORD, short, char *, char *, DWORD);
LONG H_CheckSSH(const char *, const char *, char *);
int CheckSSH(char *, DWORD, short, char *, char *, DWORD);
LONG H_CheckSMTP(const char *, const char *, char *);
int CheckSMTP(char *, DWORD, short, char *, DWORD);
LONG H_CheckHTTP(const char *, const char *, char *);
int CheckHTTP(char *, DWORD, short, char *, char *, char *, DWORD);
LONG H_CheckCustom(const char *, const char *, char *);
int CheckCustom(char *, DWORD, short, char *, char *, DWORD);
LONG H_CheckTelnet(const char *, const char *, char *);
int CheckTelnet(char *, DWORD, short, char *, char *, DWORD);

extern TCHAR g_szDomainName[];

#endif // __MAIN__H__
