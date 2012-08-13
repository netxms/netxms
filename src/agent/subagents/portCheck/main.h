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
int CheckPOP3(TCHAR *, DWORD, short, TCHAR *, TCHAR *, DWORD);
LONG H_CheckSSH(const TCHAR *, const TCHAR *, TCHAR *);
int CheckSSH(TCHAR *, DWORD, short, TCHAR *, TCHAR *, DWORD);
LONG H_CheckSMTP(const TCHAR *, const TCHAR *, TCHAR *);
int CheckSMTP(TCHAR *, DWORD, short, TCHAR *, DWORD);
LONG H_CheckHTTP(const TCHAR *, const TCHAR *, TCHAR *);
int CheckHTTP(TCHAR *, DWORD, short, TCHAR *, TCHAR *, TCHAR *, DWORD);
LONG H_CheckCustom(const TCHAR *, const TCHAR *, TCHAR *);
int CheckCustom(TCHAR *, DWORD, short, TCHAR *, TCHAR *, DWORD);
LONG H_CheckTelnet(const TCHAR *, const TCHAR *, TCHAR *);
int CheckTelnet(TCHAR *, DWORD, short, TCHAR *, TCHAR *, DWORD);

extern TCHAR g_szDomainName[];

#endif // __MAIN__H__
