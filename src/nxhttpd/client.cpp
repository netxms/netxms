/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006, 2007 Alex Kirhenshtein and Victor Kirhenshtein
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
** File: client.cpp
**
**/

#include "nxhttpd.h"


//
// Default request handler
//

static void DefaultRequestHandler(HttpRequest &request, HttpResponse &response)
{
	int i, size;
	TCHAR *uri, *ext;

	uri = request.GetURI();

	if (!_tcscmp(uri, _T("/")))
	{
		response.SetCode(HTTP_FOUND);
		response.SetHeader(_T("Location"), _T("/login.app"));
	}
	else
	{
		response.SetCode(HTTP_NOTFOUND);
		response.SetBody("ERROR 404: File not found");

		// ugly but works
		if ((g_szDocumentRoot[0] != 0) && (_tcsstr(uri, _T("..")) == NULL))
		{
			size = _tcslen(uri);
			for (i = 0; i < size; i++)
			{
				if ((uri[i] < 'A' && uri[i] > 'Z') &&
					(uri[i] < 'a' && uri[i] > 'z') &&
					(uri[i] < '0' && uri[i] > '9') &&
					uri[i] != '.' && uri[i] != '_' && uri[i] != '/')
				{
					break;
				}
			}

			if (i == size)
			{
				ext = _tcsrchr(uri, _T('.'));
				if (ext != NULL)
				{
					TCHAR *ct = NULL;

					ext++;
					if (!_tcscmp(ext, _T("html"))) { ct = _T("text/html"); }
					else if (!_tcscmp(ext, _T("css"))) { ct = _T("text/css"); }
					else if (!_tcscmp(ext, _T("js"))) { ct = _T("application/javascript"); }
					else if (!_tcscmp(ext, _T("png"))) { ct = _T("image/png"); }
					else if (!_tcscmp(ext, _T("jpg"))) { ct = _T("image/jpg"); }
					else if (!_tcscmp(ext, _T("gif"))) { ct = _T("image/gif"); }
					else if (!_tcscmp(ext, _T("xsl"))) { ct = _T("application/xml"); }

					if (ct != NULL)
					{
						String fname;
						FILE *f;

						fname = g_szDocumentRoot;
						fname += _T("/");
						fname += uri;
						fname.Translate(_T("//"), _T("/"));
#ifdef _WIN32
						fname.Translate(_T("/"), _T("\\"));
#endif
						f = _tfopen((TCHAR *)fname, _T("rb"));
						if (f != NULL)
						{
							char buffer[10240];
							int size;

							response.SetBody("", 0);

							while (1)
							{
								size = fread(buffer, 1, sizeof(buffer), f);

								if (size <= 0)
								{
									break;
								}

								if (size > 0)
								{
									response.SetBody(buffer, size, TRUE);
								}
							}

							fclose(f);

							response.SetType(ct);
							response.SetCode(HTTP_OK);
						}
					}
				}
			}
		}
	}
}


//
// Client connection handler
//

static THREAD_RESULT THREAD_CALL ClientHandler(void *pArg)
{
	int size;
	HttpRequest req;
	HttpResponse resp;
	TCHAR *pq;
	char *ptr;

	if (req.Read(CAST_FROM_POINTER(pArg, SOCKET)))
	{
		if (SessionRequestHandler(req, resp))
		{
			// Mark content as dynamic
			// FIXME: hack
			resp.SetHeader(_T("Pragma"), _T("no-cache"));
			resp.SetHeader(_T("Cache-Control"), _T("max-age=1, must-revalidate"));
			resp.SetHeader(_T("Expires"), _T("Fri, 30 Oct 1998 14:19:41 GMT"));
		}
		else
		{
			DefaultRequestHandler(req, resp);
		}
	}
	else
	{
		resp.SetCode(HTTP_BADREQUEST);
		resp.SetBody(_T("Your browser sent a request that this server could not understand"));
	}

	pq = req.GetRawQuery();	
	DebugPrintf(INVALID_INDEX, _T("[%03d] %s %s%s%s"), resp.GetCode(), req.GetMethodName(), req.GetURI(), 
	            (*pq != 0) ? _T("?") :  _T(""), pq);

	ptr = resp.BuildStream(size);
	SendEx(CAST_FROM_POINTER(pArg, SOCKET), ptr, size, 0);
	free(ptr);

	shutdown(CAST_FROM_POINTER(pArg, SOCKET), SHUT_RDWR);
	closesocket(CAST_FROM_POINTER(pArg, SOCKET));
	return THREAD_OK;
}


//
// Client listener thread
//

THREAD_RESULT THREAD_CALL ListenerThread(void *pArg)
{
   SOCKET hSocket, hClientSocket;
   struct sockaddr_in servAddr;
   int iNumErrors = 0, nRet;
   socklen_t iSize;
   struct timeval tv;
   fd_set rdfs;

   // Create socket
   if ((hSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      WriteLog(MSG_SOCKET_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      exit(1);
   }

	SetSocketReuseFlag(hSocket);

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servAddr.sin_port = htons(g_wListenPort);

   // Bind socket
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      WriteLog(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      exit(1);
   }

   // Set up queue
   listen(hSocket, SOMAXCONN);

   // Wait for connection requests
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      FD_ZERO(&rdfs);
      FD_SET(hSocket, &rdfs);
      nRet = select(SELECT_NFDS(hSocket + 1), &rdfs, NULL, NULL, &tv);
      if ((nRet > 0) && (!(g_dwFlags & AF_SHUTDOWN)))
      {
         iSize = sizeof(struct sockaddr_in);
         if ((hClientSocket = accept(hSocket, (struct sockaddr *)&servAddr, &iSize)) == -1)
         {
            int error = WSAGetLastError();

            if (error != WSAEINTR)
               WriteLog(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
            iNumErrors++;
            if (iNumErrors > 1000)
            {
               WriteLog(MSG_TOO_MANY_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
               iNumErrors = 0;
            }
            ThreadSleepMs(500);
            continue;
         }

         iNumErrors = 0;     // Reset consecutive errors counter
			ThreadCreate(ClientHandler, 0, (void *)hClientSocket);
      }
      else if (nRet == -1)
      {
         int error = WSAGetLastError();

         // On AIX, select() returns ENOENT after SIGINT for unknown reason
#ifdef _WIN32
         if (error != WSAEINTR)
#else
         if ((error != EINTR) && (error != ENOENT))
#endif
         {
            WriteLog(MSG_SELECT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
            ThreadSleepMs(100);
         }
      }
   }

   closesocket(hSocket);
   DebugPrintf(INVALID_INDEX, "Listener thread terminated");
   return THREAD_OK;
}
