/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
** File: http.cpp
**
**/

#include "portcheck.h"
#include <netxms-regex.h>

/**
 * Save HTTP(s) responce to file for later investigation
 * (Should be enabled by setting "FailedDirectory" in config
 */
static void SaveResponse(char *host, const InetAddress& ip, char *buffer)
{
   if (g_szFailedDir[0] == 0)
      return;

   time_t now = time(nullptr);
   char fileName[2048];
   char tmp[64];
   snprintf(fileName, 2048, "%s%s%s-%d",
         g_szFailedDir, FS_PATH_SEPARATOR_A,
         host != NULL ? host : ip.toStringA(tmp),
         (int)now);
   FILE *f = fopen(fileName, "wb");
   if (f != NULL)
   {
      fwrite(buffer, strlen(buffer), 1, f);
      fclose(f);
   }
}

/**
 * Check HTTP/HTTPS service - parameter handler
 */
LONG H_CheckHTTP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[1024];
	TCHAR szPort[1024];
	char szURI[1024];
	char szHeader[1024];
	char szMatch[1024];

	AgentGetParameterArgA(param, 1, szHost, sizeof(szHost));
	AgentGetParameterArg(param, 2, szPort, sizeof(szPort) / sizeof(TCHAR));
	AgentGetParameterArgA(param, 3, szURI, sizeof(szURI));
	AgentGetParameterArgA(param, 4, szHeader, sizeof(szHeader));
	AgentGetParameterArgA(param, 5, szMatch, sizeof(szMatch));

	if (szHost[0] == 0 || szPort[0] == 0 || szURI[0] == 0)
		return SYSINFO_RC_ERROR;

	uint16_t nPort = static_cast<uint16_t>(_tcstoul(szPort, nullptr, 10));
	if (nPort == 0)
	{
		nPort = 80;
	}

   uint32_t timeout = GetTimeoutFromArgs(param, 6);
   int64_t start = GetCurrentTimeMs();
   int result = (arg[1] == 'S') ?
            CheckHTTPS(szHost, InetAddress::INVALID, nPort, szURI, szHeader, szMatch, timeout) :
            CheckHTTP(szHost, InetAddress::INVALID, nPort, szURI, szHeader, szMatch, timeout);
   if (*arg == 'R')
   {
      if (result == PC_ERR_NONE)
         ret_int64(value, GetCurrentTimeMs() - start);
      else if (g_serviceCheckFlags & SCF_NEGATIVE_TIME_ON_ERROR)
         ret_int(value, -result);
      else
         nRet = SYSINFO_RC_ERROR;
   }
   else
   {
	   ret_int(value, result);
   }
	return nRet;
}

/**
 * Check HTTP service
 */
int CheckHTTP(char *szAddr, const InetAddress& addr, short nPort, char *szURI, char *szHost, char *szMatch, UINT32 dwTimeout)
{
	int nRet = 0;
	SOCKET nSd;

	if (szMatch[0] == 0)
	{
		strcpy(szMatch, "^HTTP/(1\\.[01]|2) 200 .*");
	}

   const char *errptr;
   int erroffset;
	pcre *preg = pcre_compile(szMatch, PCRE_COMMON_FLAGS_A | PCRE_CASELESS, &errptr, &erroffset, NULL);
	if (preg == NULL)
	{
		return PC_ERR_BAD_PARAMS;
	}

	nSd = NetConnectTCP(szAddr, addr, nPort, dwTimeout);
	if (nSd != INVALID_SOCKET)
	{
		char szTmp[4096];
		char szHostHeader[4096];

		nRet = PC_ERR_HANDSHAKE;

		snprintf(szHostHeader, sizeof(szHostHeader), "Host: %s:%u\r\n",
				szHost[0] != 0 ? szHost : szAddr, nPort);

		snprintf(szTmp, sizeof(szTmp),
				"GET %s HTTP/1.1\r\nConnection: close\r\nAccept: */*\r\n%s\r\n",
				szURI, szHostHeader);

		if (NetWrite(nSd, szTmp, strlen(szTmp)))
		{
#define READ_TIMEOUT 5000
#define CHUNK_SIZE 10240
			char *buff = (char *)malloc(CHUNK_SIZE);
			ssize_t offset = 0;
			ssize_t buffSize = CHUNK_SIZE;

			while(SocketCanRead(nSd, READ_TIMEOUT))
			{
				ssize_t nBytes = NetRead(nSd, buff + offset, buffSize - offset);
				if (nBytes > 0) 
            {
					offset += nBytes;
					if (buffSize - offset < (CHUNK_SIZE / 2)) 
               {
						char *tmp = (char *)realloc(buff, buffSize + CHUNK_SIZE);
						if (tmp != NULL) 
                  {
							buffSize += CHUNK_SIZE;
							buff = tmp;
						}
						else 
                  {
							MemFreeAndNull(buff);
                     break;
						}
					}
				}
				else 
            {
					break;
				}
			}

			if (buff != NULL && offset > 0) 
         {
				buff[offset] = 0;

				int ovector[30];
            if (pcre_exec(preg, nullptr, buff, static_cast<int>(strlen(buff)), 0, 0, ovector, 30) >= 0)
				{
					nRet = PC_ERR_NONE;
				}
            else
            {
               SaveResponse(szAddr, addr, buff);
            }
			}

         MemFree(buff);
		}
		NetClose(nSd);
	}
	else
	{
		nRet = PC_ERR_CONNECT;
	}

	pcre_free(preg);
	return nRet;
}

/**
 * Check HTTPS service
 */
int CheckHTTPS(char *szAddr, const InetAddress& addr, short nPort, char *szURI, char *szHost, char *szMatch, UINT32 dwTimeout)
{
#ifdef _WITH_ENCRYPTION
   if (szMatch[0] == 0)
   {
      strcpy(szMatch, "^HTTP/(1\\.[01]|2) 200 .*");
   }

   const char *errptr;
   int erroffset;
   pcre *preg = pcre_compile(szMatch, PCRE_COMMON_FLAGS_A | PCRE_CASELESS, &errptr, &erroffset, NULL);
   if (preg == NULL)
   {
      return PC_ERR_BAD_PARAMS;
   }

   int ret = PC_ERR_INTERNAL;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   const SSL_METHOD *method = TLS_method();
#else
   const SSL_METHOD *method = SSLv23_method();
#endif
   SSL_CTX *ctx = SSL_CTX_new(method);
   if (ctx != nullptr)
   {
      SSL *ssl = SSL_new(ctx);
      if (ssl != nullptr)
      {
         SSL_set_connect_state(ssl);
         BIO *ssl_bio = BIO_new(BIO_f_ssl());
         if (ssl_bio != nullptr)
         {
            BIO_set_ssl(ssl_bio, ssl, BIO_CLOSE);

            ret = PC_ERR_CONNECT;

            BIO *out = BIO_new(BIO_s_connect());
            if (out != NULL)
            {
               if (szAddr != NULL)
               {
                  BIO_set_conn_hostname(out, szAddr);
               }
               else
               {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
                  char addrText[128];
                  BIO_set_conn_hostname(out, addr.toStringA(addrText));
#else		  
                  UINT32 addrV4 = htonl(addr.getAddressV4());
                  BIO_set_conn_ip(out, &addrV4);
#endif
               }
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
               char portText[32];
               snprintf(portText, 32, "%d", (int)nPort);
               BIO_set_conn_port(out, portText);
#else
               int intPort = nPort;
               BIO_set_conn_int_port(out, &intPort);
#endif
               out = BIO_push(ssl_bio, out);

               if (BIO_do_connect(out) > 0)
               {
                  bool sendFailed = false;
                  // send request
                  char szHostHeader[256];
                  char szTmp[2048];
                  snprintf(szHostHeader, sizeof(szHostHeader), "Host: %s:%u\r\n", szHost[0] != 0 ? szHost : szAddr, nPort); 
                  snprintf(szTmp, sizeof(szTmp), "GET %s HTTP/1.1\r\nConnection: close\r\nAccept: */*\r\n%s\r\n", szURI, szHostHeader);
                  int len = (int)strlen(szTmp);
                  int offset = 0;
                  while(true)
                  {
                     int sent = BIO_write(out, &(szTmp[offset]), len);
                     if (sent <= 0)
                     {
                        if (BIO_should_retry(out))
                        {
                           continue;
                        }
                        else
                        {
                           sendFailed = true;
                           AgentWriteDebugLog(7, _T("PortCheck: BIO_write failed"));
                           break;
                        }
                     }
                     offset += sent;
                     len -= sent;
                     if (len <= 0)
                     {
                        break;
                     }
                  }

                  ret = PC_ERR_HANDSHAKE;

                  // read reply
                  if (!sendFailed)
                  {
#define BUFSIZE (10 * 1024 * 1024)
                     char *buffer = (char *)malloc(BUFSIZE); // 10Mb
                     memset(buffer, 0, BUFSIZE);

                     int i;
                     int offset = 0;
                     while(offset < BUFSIZE - 1)
                     {
                        i = BIO_read(out, buffer + offset, BUFSIZE - offset - 1);
                        if (i == 0)
                        {
                           break;
                        }
                        if (i < 0)
                        {
                           if (BIO_should_retry(out))
                           {
                              continue;
                           }
                           AgentWriteDebugLog(7, _T("PortCheck: BIO_read failed (offset=%d)"), offset);
                           buffer[0] = 0;  // do not check incomplete buffer
                           break;
                        }
                        offset += i;
                     }
                     if (buffer[0] != 0) 
                     {
                        int ovector[30];
                        if (pcre_exec(preg, NULL, buffer, static_cast<int>(strlen(buffer)), 0, 0, ovector, 30) >= 0)
                        {
                           ret = PC_ERR_NONE;
                        }
                        else
                        {
                           SaveResponse(szAddr, addr, buffer);
                           AgentWriteDebugLog(7, _T("PortCheck: content do not match"));
                        }
                     }

                     MemFree(buffer);
                  }
               }
               BIO_free_all(out);
            }
         }
         else
         {
            AgentWriteDebugLog(7, _T("PortCheck: BIO_new failed"));
         }
      }
      else
      {
         AgentWriteDebugLog(7, _T("PortCheck: SSL_new failed"));
      }
      SSL_CTX_free(ctx);
   }
   else
   {
      AgentWriteDebugLog(7, _T("PortCheck: SSL_CTX_new failed"));
   }

   pcre_free(preg);
   return ret;

#else
   return PC_ERR_INTERNAL;
#endif
}
