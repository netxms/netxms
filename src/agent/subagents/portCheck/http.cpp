#include <nms_common.h>
#include <nms_agent.h>
#include <netxms-regex.h>

#include "main.h"
#include "net.h"

/**
 * Save HTTP(s) responce to file for later investigation
 * (Should be enabled by setting "FailedDirectory" in config
 */
static void SaveResponse(char *host, UINT32 ip, char *buffer)
{
   if (g_szFailedDir[0] == 0)
   {
      return;
   }

   time_t now = time(NULL);
   char fileName[2048];
   char tmp[32];
   snprintf(fileName, 2048, "%s%s%s-%d",
         g_szFailedDir, FS_PATH_SEPARATOR_A,
         host != NULL ? host : IpToStrA(ip, tmp),
         now);
   FILE *f = fopen(fileName, "wb");
   if (f != NULL)
   {
      fwrite(buffer, strlen(buffer), 1, f);
      fclose(f);
   }
}


LONG H_CheckHTTP(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[1024];
	TCHAR szPort[1024];
	char szURI[1024];
	char szHeader[1024];
	char szMatch[1024];
	TCHAR szTimeout[64];
	unsigned short nPort;

	AgentGetParameterArgA(pszParam, 1, szHost, sizeof(szHost));
	AgentGetParameterArg(pszParam, 2, szPort, sizeof(szPort));
	AgentGetParameterArgA(pszParam, 3, szURI, sizeof(szURI));
	AgentGetParameterArgA(pszParam, 4, szHeader, sizeof(szHeader));
	AgentGetParameterArgA(pszParam, 5, szMatch, sizeof(szMatch));
   AgentGetParameterArg(pszParam, 6, szTimeout, sizeof(szTimeout));

	if (szHost[0] == 0 || szPort[0] == 0 || szURI[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)_tcstoul(szPort, NULL, 10);
	if (nPort == 0)
	{
		nPort = 80;
	}

	UINT32 dwTimeout = _tcstoul(szTimeout, NULL, 0);
   if (pArg == NULL)
   {
      // HTTP
      ret_int(pValue, CheckHTTP(szHost, 0, nPort, szURI, szHeader, szMatch, dwTimeout));
   }
   else
   {
      // HTTPS
      ret_int(pValue, CheckHTTPS(szHost, 0, nPort, szURI, szHeader, szMatch, dwTimeout));
   }
	return nRet;
}

int CheckHTTP(char *szAddr, UINT32 dwAddr, short nPort, char *szURI,
		char *szHost, char *szMatch, UINT32 dwTimeout)
{
	int nBytes, nRet = 0;
	SOCKET nSd;
	regex_t preg;

	if (szMatch[0] == 0)
	{
		strcpy(szMatch, "^HTTP/1.[01] 200 .*");
	}

	if (tre_regcomp(&preg, szMatch, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
	{
		return PC_ERR_BAD_PARAMS;
	}

	nSd = NetConnectTCP(szAddr, dwAddr, nPort, dwTimeout);
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

		if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
		{
#define READ_TIMEOUT 5000
#define CHUNK_SIZE 10240
			char *buff = (char *)malloc(CHUNK_SIZE);
			int offset = 0;
			int buffSize = CHUNK_SIZE;

			while(NetCanRead(nSd, READ_TIMEOUT))
			{
				nBytes = NetRead(nSd, buff + offset, buffSize - offset);
				if (nBytes > 0) {
					offset += nBytes;
					if (buffSize - offset < (CHUNK_SIZE / 2)) {
						char *tmp = (char *)realloc(buff, buffSize + CHUNK_SIZE);
						if (tmp != NULL) {
							buffSize += CHUNK_SIZE;
							buff = tmp;
						}
						else {
							safe_free(buff);
							buffSize = 0;
						}
					}
				}
				else {
					break;
				}
			}

			if (buff != NULL && offset > 0) {
				buff[offset] = 0;

				if (tre_regexec(&preg, buff, 0, NULL, 0) == 0)
				{
					nRet = PC_ERR_NONE;
				}
            else
            {
               SaveResponse(szAddr, dwAddr, buff);
            }

				safe_free(buff);
			}
		}
		NetClose(nSd);
	}
	else
	{
		nRet = PC_ERR_CONNECT;
	}

	regfree(&preg);

	return nRet;
}

int CheckHTTPS(char *szAddr, UINT32 dwAddr, short nPort, char *szURI, char *szHost, char *szMatch, UINT32 dwTimeout)
{
#ifdef WITH_OPENSSL
   if (szMatch[0] == 0)
   {
      strcpy(szMatch, "^HTTP/1.[01] 200 .*");
   }

   regex_t preg;
   if (tre_regcomp(&preg, szMatch, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
   {
      return PC_ERR_BAD_PARAMS;
   }

   int ret = PC_ERR_INTERNAL;

   SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
   if (ctx != NULL)
   {
      SSL *ssl = SSL_new(ctx);
      if (ssl != NULL)
      {
         SSL_set_connect_state(ssl);

         BIO *ssl_bio = BIO_new(BIO_f_ssl());
         if (ssl_bio != NULL)
         {
            BIO_set_ssl(ssl_bio, ssl, BIO_CLOSE);

            BIO *out = BIO_new(BIO_s_connect());
            if (out != NULL)
            {
               if (szAddr != NULL)
               {
                  BIO_set_conn_hostname(out, szAddr);
               }
               else
               {
                  BIO_set_conn_ip(out, htonl(dwAddr));
               }
               int intPort = nPort;
               BIO_set_conn_int_port(out, &intPort);
               BIO_set_nbio(out, 1);
               out = BIO_push(ssl_bio, out);

               ret = PC_ERR_CONNECT;

               bool sendFailed = false;
               // send request
               char szHostHeader[256];
               char szTmp[2048];
               snprintf(szHostHeader, sizeof(szHostHeader), "Host: %s:%u\r\n", szHost[0] != 0 ? szHost : szAddr, nPort); 
               snprintf(szTmp, sizeof(szTmp), "GET %s HTTP/1.1\r\nConnection: close\r\nAccept: */*\r\n%s\r\n", szURI, szHostHeader);
               int len = strlen(szTmp);
               int offset = 0;
               while (true)
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
                  while (true)
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
                        break;
                     }
                     offset += i;
                  }
                  if (buffer[0] != 0) {
                     if (tre_regexec(&preg, buffer, 0, NULL, 0) == 0)
                     {
                        ret = PC_ERR_NONE;
                     }
                     else
                     {
                        SaveResponse(szAddr, dwAddr, buffer);
                     }
                  }

                  safe_free(buffer);
               }

               BIO_free_all(out);
            }
         }
         //SSL_free(ssl);
      }
      SSL_CTX_free(ctx);
   }

   regfree(&preg);

   return ret;

#else
   return PC_ERR_INTERNAL;
#endif
}
