#ifndef __MAIN__H__
#define __MAIN__H__

enum
{
	PC_ERR_NONE,
	PC_ERR_BAD_PARAMS,
	PC_ERR_CONNECT,
	PC_ERR_HANDSHAKE,
};

LONG H_CheckPOP3(char *, char *, char *);
int CheckPOP3(char *, DWORD, short, char *, char *);
LONG H_CheckSSH(char *, char *, char *);
int CheckSSH(char *, DWORD, short, char *, char *);
LONG H_CheckSMTP(char *, char *, char *);
int CheckSMTP(char *, DWORD, short, char *);
int CheckHTTP(char *, DWORD, short, char *, char *, char *);
int CheckCustom(char *, DWORD, short, char *, char *);

#endif // __MAIN__H__
