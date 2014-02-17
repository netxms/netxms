/*
 ** Enhanced CheckSumm subagent
 ** Copyright (C) 2006 Alex Kirhenshtein
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **
 **/

#include <nms_common.h>
#include <nms_agent.h>

#ifdef _WIN32
#define ECS_EXPORTABLE __declspec(dllexport) __cdecl
#else
#define ECS_EXPORTABLE
#endif

// max size of remote file (10mb)
#define MAX_REMOTE_SIZE (10 * 1024 * 1024)

static unsigned char *GetHttpUrl(char *url, int *size)
{
	char *ret = NULL;

	char *host = strdup(url);

	char *uri = strchr(host, '/');
	if (uri == NULL)
	{
		uri = (char *)"";
	}
	else
	{
		*uri = 0;
		uri++;
	}

	char *p = strchr(host, ':');
	unsigned int port = 80;
	if (p != NULL)
	{
		*p = 0;
		p++;
		port = atoi(p);
		if (port == 0)
		{
			port = 80;
		}
	}

	*size = 0;

	DWORD hostAddr = ResolveHostNameA(host);
	if (hostAddr != INADDR_NONE)
	{
		SOCKET sd = socket(AF_INET, SOCK_STREAM, 0);
		if (sd != INVALID_SOCKET)
		{
			struct sockaddr_in sa;

			sa.sin_family = AF_INET;
			sa.sin_port = htons(port);
			sa.sin_addr.s_addr = hostAddr;

			if (connect(sd, (struct sockaddr*)&sa, sizeof(sa)) == 0)
			{
				char req[1024];

				int len = sprintf(req, "GET /%s HTTP/1.0\r\nHost: %s:%u\r\nConnection: close\r\n\r\n", uri, host, port);
				if (SendEx(sd, req, len, 0, NULL) == len)
				{
					char buff[10240];
					int err;
					// ok, request sent, read content;
					while ((err = RecvEx(sd, buff, 10240, 0, 30000)) > 0)
					{
						if (*size + err > MAX_REMOTE_SIZE)
						{
							free(ret);
							ret = NULL;
							break;
						}
						p = (char *)realloc(ret, *size + err + 1);
						if (p != NULL)
						{
							ret = p;
							memcpy(ret + *size, buff, err);
							*size += err;
						}
						else
						{
							free(ret);
							ret = NULL;
							break;
						}
					}
				}
			}
			closesocket(sd);
		}
	}

	if (ret != NULL)
	{
		if (*size > 7 && !strnicmp(ret, "HTTP/1.", 7))
		{
			ret[*size] = 0;
			char *v1 = strstr(ret, "\r\n\r\n"); // should be this
			char *v2 = strstr(ret, "\n\n"); // fallback

			p = NULL;

			if (v1 != NULL && v2 == NULL)
			{
				p = v1 + 4;
			}
			else if (v2 != NULL && v1 == NULL)
			{
				p = v2 + 2;
			}
			else if (v1 != NULL && v2 != NULL)
			{
				p = min(v1 + 4, v2 + 2);
			}

			if (p != NULL)
			{
				*size -= (int)(p - ret);
				memmove(ret, p, *size);
			}
			else
			{
				free(ret);
				ret = NULL;
			}
		}
		else
		{
			free(ret);
			ret = NULL;
		}
	}

   free(host);
	return (unsigned char *)ret;
}

//
// Hanlder functions
//

static LONG H_DoHttp(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	LONG ret = SYSINFO_RC_ERROR;
	char szArg[256];

	int digetSize;
	void (*hashFunction)(unsigned char *, size_t, unsigned char *) = NULL;
	if (CAST_FROM_POINTER(pArg, int) == 1)
	{
		digetSize = SHA1_DIGEST_SIZE;
		hashFunction = &CalculateSHA1Hash;
	}
	else
	{
		digetSize = MD5_DIGEST_SIZE;
		hashFunction = (void (*)(unsigned char *, size_t, unsigned char *))&CalculateMD5Hash;
	}

	AgentGetParameterArgA(pszParam, 1, szArg, 255);

	if (!strnicmp(szArg, "http://", 7))
	{
		int contentSize;
		unsigned char *content = GetHttpUrl(szArg + 7, &contentSize);
		if (content != NULL)
		{
			unsigned char hash[SHA1_DIGEST_SIZE]; // all hashes should fit (SHA1 == 20, MD5 == 16)
			hashFunction(content, contentSize, hash);

			char hashText[SHA1_DIGEST_SIZE * 2];
			for (int i = 0; i < digetSize; i++)
			{
				sprintf(hashText + (i * 2), "%02x", hash[i]);
			}

  			ret_mbstring(pValue, hashText);
			ret = SYSINFO_RC_SUCCESS;

			free(content);
		}
	}

	return ret;
}


static LONG H_LoadTime(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
	LONG ret = SYSINFO_RC_ERROR;
	char url[256];

	AgentGetParameterArgA(param, 1, url, 255);
	if (!strnicmp(url, "http://", 7))
	{
		int contentSize;
		INT64 startTime = GetCurrentTimeMs();
		unsigned char *content = GetHttpUrl(url + 7, &contentSize);
		if (content != NULL)
		{
			DWORD loadTime = (DWORD)(GetCurrentTimeMs() - startTime);
			ret_uint(value, loadTime);
			ret = SYSINFO_RC_SUCCESS;
			free(content);
		}
	}

	return ret;
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("ECS.HttpSHA1(*)"),				H_DoHttp,					(TCHAR *)1,
		DCI_DT_STRING, _T("Calculates SHA1 hash of * URL") },
	{ _T("ECS.HttpMD5(*)"),				  H_DoHttp,					(TCHAR *)5,
		DCI_DT_STRING, _T("Calculates MD5 hash of * URL") },
	{ _T("ECS.HttpLoadTime(*)"),		H_LoadTime,				NULL,
		DCI_DT_STRING, _T("Measure load time for URL *") }
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("ECS"), NETXMS_VERSION_STRING,
	NULL, NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, NULL,		// lists
	0, NULL,		// tables
	0, NULL,		// actions
	0, NULL		// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(ECS)
{
	*ppInfo = &m_info;
	return TRUE;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
