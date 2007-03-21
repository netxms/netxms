/* $Id: nxweb.cpp,v 1.1 2007-03-21 10:15:18 alk Exp $ */

#include "nxweb.h"

#define SESSION_TIMEOUT (10 * 60)

NxWeb::NxWeb()
{
	m_sessionsMutex = MutexCreate();
}

NxWeb::~NxWeb()
{
	MutexDestroy(m_sessionsMutex);
}

bool NxWeb::HandleRequest(HttpRequest &request, HttpResponse &response)
{
	// redirect to /netxms.app
	if (request.GetURI() == "/")
	{
		response.SetCode(HTTP_FOUND);
		response.SetHeader("Location", "/netxms.app");
		return true;
	}

	if (request.GetURI() == "/netxms.app")
	{
		string sid;
		string cmd;

		Session *session = NULL;
		if (request.GetQueryParam("sid", sid))
		{
			session = CheckSession(sid);
		}

		if (session != NULL)
		{
			if (request.GetQueryParam("cmd", cmd))
			{
				// got command
				if (cmd == "login")
					;
				else if (cmd == "objects")
					;
				else
				{
					// unknown command; show error page
					//response.RenderTemplate("templates/error.tpt");
				}
			}
			// got valid session
			response.SetCode(HTTP_OK);
			response.SetType("text/plain");
			response.SetBody("Got valid session");
		}
		else
		{
			//response.RenderTemplate("templates/login.tpt");
		}

		/*
		response.SetCode(HTTP_OK);
		response.SetType("application/xml");

		bool isOk = false;

		if (request.GetQueryParam("cmd", cmd))
		{
			if (cmd == "login")
			{
				err = "Can't connect to NetXMS server or wrong credentials";
				if (DoLogin(request, response, sid))
				{
					cmd = "overview";
				}
			}

			request.GetQueryParam("sid", sid);
			Session *s = CheckSession(sid);
			if (s != NULL)
			{
				if (cmd == "overview")
				{
					DoOverview(s, request, response);
					isOk = true;
				}
				else if (cmd == "alarms")
				{
					DoAlarms(s, request, response);
					isOk = true;
				}
				else if (cmd == "objects")
				{
					DoObjects(s, request, response);
					isOk = true;
				}
				else if (cmd == "objectsList")
				{
					DoObjectsList(s, request, response);
					isOk = true;
				}
				else if (cmd == "objectInfo")
				{
					DoObjectInfo(s, request, response);
					isOk = true;
				}
				else if (cmd == "logout")
				{
					DoLogout(s);
					err = "";
					isOk = false;
				}
			}
		}

		// ...
		if (!isOk)
		{
			response.SetBody(_Login(request, err));
		}
		*/

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	if (request.GetURI() == "/pie.png")
	{
		NxPie pie;
		string name, value;
		char var[16];
		int count;

		response.SetCode(HTTP_OK);

		count = 0;

		for (count = 0; count < MAX_PIE_ELEMENTS; count++)
		{
			snprintf(var, 16, "l%d", count);
			if (request.GetQueryParam(var, name))
			{
				snprintf(var, 16, "v%d", count);
				if (request.GetQueryParam(var, value))
				{
					int n = atoi(value.c_str());

					pie.setValue(name, n);
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}

		if (count > 0)
		{
			// show chart
			if (pie.build())
			{
				response.SetType("image/png");
				response.SetBody((const char *)pie.getRawImage(), pie.getRawImageSize());
			}
			else
			{
				count = 0;
			}
		}

		if (count == 0)
		{
			// show error
			response.SetType("text/plain");
			response.SetBody("error");
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
Session *NxWeb::CheckSession(string sid)
{
	Session *ret = NULL;

	MutexLock(m_sessionsMutex, INFINITE);

	tSessionsMap::iterator it;

	// expire
	for (it = sessions.begin(); it != sessions.end(); it++)
	{
		if (it->second->lastSeen + (SESSION_TIMEOUT) < time(NULL))
		{
			NXCDisconnect(it->second->handle);
			delete it->second;
			sessions.erase(it->first);

			if (sessions.empty())
			{
				break;
				it = sessions.begin();
			}
		}
	}

	// find
	it = sessions.find(sid);

	if (it != sessions.end())
	{
		it->second->lastSeen = time(NULL);
		ret = it->second;
	}
	MutexUnlock(m_sessionsMutex);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
