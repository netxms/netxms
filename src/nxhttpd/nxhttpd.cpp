/* $Id: nxhttpd.cpp,v 1.1 2007-03-21 10:15:18 alk Exp $ */

#include "nxhttpd.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////

NxHttpd::NxHttpd()
{
	m_socket = -1;
	m_port = 80;
	m_goDown = false;
	m_documentRoot = "";
}

NxHttpd::~NxHttpd()
{
}

//////////////////////////////////////////////////////////////////////////

bool NxHttpd::Start(void)
{
	bool ret = false;

	m_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (m_socket != -1)
	{
		struct sockaddr_in sa;

		memset(&sa, 0, sizeof(sa));
		
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = INADDR_ANY;
		sa.sin_port = htons(m_port);

		SetSocketReuseFlag(m_socket);

		if (bind(m_socket, (sockaddr *)&sa, sizeof(sa)) != (-1))
		{
			if (listen(m_socket, 5) == 0)
			{
				while(!m_goDown)
				{
					int clientSocket;
					struct sockaddr_in saClient;
					int saLen = sizeof(sockaddr_in);
               struct timeval tv;
               fd_set rdfs;

               FD_ZERO(&rdfs);
               FD_SET(m_socket, &rdfs);
               tv.tv_sec = 1;
               tv.tv_usec = 0;
               if (select(m_socket + 1, &rdfs, NULL, NULL, &tv) > 0)
               {
					   clientSocket = accept(m_socket, (struct sockaddr *)&saClient, (socklen_t *)&saLen);
					   if (clientSocket > 0)
					   {
						   NxClientThreadStorage *st = new NxClientThreadStorage();

						   st->clientSocket = clientSocket;
						   st->instance = this;

						   ThreadCreate(ClientHandler, 0, (void *)st);
					   }
               }
				}
				closesocket(m_socket);
				ret = true;
			}
		}
	}

	return ret;
}

bool NxHttpd::Stop(void)
{
	m_goDown = true;

	return false;
}

bool NxHttpd::HandleRequest(HttpRequest &request, HttpResponse &response)
{
	response.SetCode(HTTP_NOTFOUND);
	response.SetBody("File not found", 0);

	return true;
}

bool NxHttpd::DefaultHandleRequest(HttpRequest &request, HttpResponse &response)
{
	int i;

	response.SetCode(HTTP_NOTFOUND);
	response.SetBody("File not found", 0);

	string uri = request.GetURI();

	// ugly but works
	if (!m_documentRoot.empty() && uri.find("..") == string::npos)
	{
		for (i = 0; i < uri.size(); i++)
		{
			if ((uri[i] < 'A' && uri[i] > 'Z') &&
				(uri[i] < 'a' && uri[i] > 'z') &&
				(uri[i] < '0' && uri[i] > '9') &&
				uri[i] != '.' && uri[i] != '_' && uri[i] != '/')
			{
				break;
			}
		}

		if (i == uri.size())
		{
			int extStart = uri.rfind(".");
			if (extStart != string::npos)
			{
				string ext = uri.substr(extStart + 1);
				char *ct = NULL;

				if (ext == "html") { ct = "text/html"; }
				else if (ext == "css") { ct = "text/css"; }
				else if (ext == "js") { ct = "application/javascript"; }
				else if (ext == "png") { ct = "image/png"; }
				else if (ext == "jpg") { ct = "image/jpg"; }
				else if (ext == "gif") { ct = "image/gif"; }
				else if (ext == "xsl") { ct = "application/xml"; }

				if (ct != NULL)
				{
					FILE *f = fopen((m_documentRoot + "/" + uri).c_str(), "r+b");
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
								response.SetBody(buffer, size, true);
							}
						}

						response.SetType(ct);
						response.SetCode(HTTP_OK);

						fclose(f);
					}
				}
			}
		}
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////

void NxHttpd::SetPort(int port)
{
	m_port = port;
}


void NxHttpd::SetDocumentRoot(string root)
{
	m_documentRoot = root;
}

//////////////////////////////////////////////////////////////////////////

THREAD_RESULT THREAD_CALL NxHttpd::ClientHandler(void *pArg)
{
	int size;
	HttpRequest req;
	HttpResponse resp;
	char *ptr;

	NxClientThreadStorage *st = (NxClientThreadStorage *)pArg;

	if (req.Read(st->clientSocket))
	{
		if (st->instance->HandleRequest(req, resp) == false)
		{
			st->instance->DefaultHandleRequest(req, resp);
		}
	}
	else
	{
		resp.SetCode(HTTP_BADREQUEST);
		resp.SetBody("Your browser sent a request that this server could not understand", 0);
	}

	printf("%s >>> %s\n", resp.GetCodeString().substr(0, 3).c_str(), req.GetURI().c_str());

	ptr = resp.BuildStream(size);
	send(st->clientSocket, ptr, size, 0);
	resp.CleanStream(ptr);

	closesocket(st->clientSocket);

	delete st;

	return THREAD_OK;
}

//////////////////////////////////////////////////////////////////////////

string NxHttpd::FilterEnt(string in)
{
	string out = "";

	for (int i = 0; i < in.size(); i++)
	{
		if (in[i] == '&')
		{
			out += "&amp;";
		}
		else if (in[i] == '<')
		{
			out += "&lt;";
		}
		else if (in[i] == '>')
		{
			out += "&gt;";
		}
		else if (in[i] == '"')
		{
			out += "&quot;";
		}
		else
		{
			out += in[i];
		}
	}

	return out;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
