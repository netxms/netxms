#include "NxWeb.h"

static string m_szStatusText[] = { "NORMAL", "WARNING", "MINOR", "MAJOR",
                            "CRITICAL", "UNKNOWN", "UNMANAGED",
                            "DISABLED", "TESTING" };

bool NxWeb::DoLogin(HttpRequest req, HttpResponse &resp, string &sid)
{
	bool ret = false;
	string user, password;

	if (req.GetQueryParam("user", user))
	{
		if (req.GetQueryParam("pwd", password))
		{
			Session *s = new Session();

			printf("LOGIN: (%s) (%s)\n", user.c_str(), password.c_str());

			if (s != NULL)
			{
				int err;

				err = NXCConnect(0,
						g_szMasterServer,
						(char *)user.c_str(),
						(char *)password.c_str(),
						0, NULL, NULL,
						&s->handle,
						"NetXMS WSM/" NETXMS_VERSION_STRING,
						NULL);

				if (err == RCC_SUCCESS)
				{
					NXCSetCommandTimeout(s->handle,
							30000); // 30 sec. timeout

					err = NXCSyncObjects(s->handle);
				}
				else
				{
					s->handle = NULL;
				}

				// load users
				if (err == RCC_SUCCESS)
				{
					err = NXCLoadUserDB(s->handle);
				}

				// load events
				if (err == RCC_SUCCESS)
				{
					err = NXCLoadEventDB(s->handle);
				}

				if (err == RCC_SUCCESS)
				{
					// FIXME
					char szTmp[128];
					unsigned char cHash[64];

					int nSize = snprintf(szTmp, sizeof(szTmp), "SID-%s-%lu-%d-%s",
							user.c_str(), (unsigned int)(((unsigned long)-1) *
								(rand() / (RAND_MAX + 1.0))),
							time(NULL), password.c_str());
					CalculateSHA1Hash((unsigned char *)szTmp, nSize, cHash);
					for (int i = 0; i < 20; i++)
					{
						sprintf(szTmp + ( i * 2 ), "%02X", cHash[i]);
					}

					s->sid = szTmp;
					s->lastSeen = time(NULL);

					MutexLock(m_sessionsMutex, INFINITE);
					sessions[s->sid] = s;
					MutexUnlock(m_sessionsMutex);

					sid = s->sid;

					ret = true;
				}

				// cleanup on errors
				if (err != RCC_SUCCESS)
				{
					if (s->handle != NULL)
					{
						NXCDisconnect(s->handle);
					}
					delete s;
				}
			}
		}
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////

bool NxWeb::DoLogout(Session *s)
{
	MutexLock(m_sessionsMutex, INFINITE);
	NXCDisconnect(s->handle);
	sessions.erase(s->sid);
	delete s;
	MutexUnlock(m_sessionsMutex);

	return true;
}

//////////////////////////////////////////////////////////////////////////

bool NxWeb::DoOverview(Session *s, HttpRequest req, HttpResponse &resp)
{
	bool ret = false;

	NXC_SERVER_STATS serverStatus;
	NXCGetServerStats(s->handle, &serverStatus);

	resp.SetBody(_Overview(req, s->sid, &serverStatus));

	ret = true;

	return ret;
}

//////////////////////////////////////////////////////////////////////////

bool NxWeb::DoAlarms(Session *s, HttpRequest req, HttpResponse &resp)
{
	bool ret = false;

	string sID;

	if (req.GetQueryParam("ack", sID))
	{
		int id = atoi(sID.c_str());
		NXCAcknowledgeAlarm(s->handle, id);
	}

	resp.SetBody(_Alarms(req, s));
	ret = true;

	return ret;
}

//////////////////////////////////////////////////////////////////////////

bool NxWeb::DoObjects(Session *s, HttpRequest req, HttpResponse &resp)
{
	bool ret = false;

	resp.SetBody(_Objects(req, s->sid));
	
	ret = true;

	return ret;
}


//////////////////////////////////////////////////////////////////////////

bool NxWeb::DoObjectsList(Session *s, HttpRequest req, HttpResponse &resp)
{
	bool ret = false;

	string sParent;
	int parentId = 0;

	if (req.GetQueryParam("parent", sParent))
	{
		parentId = atoi(sParent.c_str());
	}

	char szTmp[16];
	snprintf(szTmp, sizeof(szTmp), "%d", parentId);
	sParent = szTmp;

	NXC_OBJECT **ppRootObjects = NULL;
	NXC_OBJECT_INDEX *pIndex = NULL;
	DWORD i, j, dwNumObjects, dwNumRootObj;

	if (parentId == 0)
	{
		pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(s->handle, &dwNumObjects);
		ppRootObjects = (NXC_OBJECT **)malloc(sizeof(NXC_OBJECT *) * dwNumObjects);
		for(i = 0, dwNumRootObj = 0; i < dwNumObjects; i++)
		{
			if (!pIndex[i].pObject->bIsDeleted)
			{
				for(j = 0; j < pIndex[i].pObject->dwNumParents; j++)
				{
					if (NXCFindObjectByIdNoLock(s->handle, pIndex[i].pObject->pdwParentList[j]) != NULL)
					{
						break;
					}
				}
				if (j == pIndex[i].pObject->dwNumParents)
				{
					ppRootObjects[dwNumRootObj++] = pIndex[i].pObject;
				}
			}
		}
		NXCUnlockObjectIndex(s->handle);
	}
	else
	{
		dwNumRootObj = 0;
		NXC_OBJECT *object = NXCFindObjectById(s->handle, parentId);
		if (object != NULL)
		{
			ppRootObjects = (NXC_OBJECT **)malloc(sizeof(NXC_OBJECT *) * object->dwNumChilds);
			for (i = 0; i < object->dwNumChilds; i++)
			{
				NXC_OBJECT *childObject = NXCFindObjectById(
					s->handle, object->pdwChildList[i]);
				if (childObject != NULL)
				{
					ppRootObjects[dwNumRootObj++] = childObject;
				}
			}
		}
	}

	string tmp = "<tree>";

	for(i = 0; i < dwNumRootObj; i++)
	{
		char szTmp[16];
		snprintf(szTmp, 16, "%d", ppRootObjects[i]->dwId);

		string ico = "";
		switch (ppRootObjects[i]->iClass)
		{
		case 1:
			ico = "/images/_subnet.png";
			break;
		case 2:
			ico = "/images/_node.png";
			break;
		case 3:
			ico = "/images/_interface.png";
			break;
		case 4:
			ico = "/images/_network.png";
			break;
		}

		tmp += "<tree text=\"" + FilterEnt(ppRootObjects[i]->szName) + "\" ";
		if (ico.size() > 0)
		{
			tmp += "openIcon=\"" + ico + "\" icon=\"" + ico + "\" ";
		}

		if (ppRootObjects[i]->dwNumChilds > 0)
		{
			tmp += "src=\"/netxms.app?cmd=objectsList&amp;sid=" + s->sid +
				"&amp;parent=" + string(szTmp) + "\" ";
		}

		tmp += "action=\"javascript:showObjectInfo(" + string(szTmp) + ");\" />";
	}

	tmp += "</tree>";

	if (ppRootObjects != NULL)
	{
		free(ppRootObjects);
	}


	resp.SetBody(tmp);
	
	ret = true;

	return ret;
}


//////////////////////////////////////////////////////////////////////////

bool NxWeb::DoObjectInfo(Session *s, HttpRequest req, HttpResponse &resp)
{
	bool ret = false;

	string id;
	int objectId;
	if (req.GetQueryParam("id", id))
	{
		objectId = atoi(id.c_str());
		if (objectId > 0)
		{
			string clText = "";
			NXC_OBJECT *object = NXCFindObjectById(s->handle, objectId);

			switch(object->iClass)
			{
			case OBJECT_GENERIC:
				clText = " (OBJECT_GENERIC)";
				break;
			case OBJECT_SUBNET:
				clText = " (OBJECT_SUBNET)";
				break;
			case OBJECT_NETWORK:
				clText = " (OBJECT_NETWORK)";
				break;
			case OBJECT_CONTAINER:
				clText = " (OBJECT_CONTAINER)";
				break;
			case OBJECT_ZONE:
				clText = " (OBJECT_ZONE)";
				break;
			case OBJECT_SERVICEROOT:
				clText = " (OBJECT_SERVICEROOT)";
				break;
			case OBJECT_TEMPLATE:
				clText = " (OBJECT_TEMPLATE)";
				break;
			case OBJECT_TEMPLATEGROUP:
				clText = " (OBJECT_TEMPLATEGROUP)";
				break;
			case OBJECT_TEMPLATEROOT:
				clText = " (OBJECT_TEMPLATEROOT)";
				break;
			case OBJECT_NETWORKSERVICE:
				clText = " (OBJECT_NETWORKSERVICE)";
				break;
			case OBJECT_VPNCONNECTOR:
				clText = " (OBJECT_VPNCONNECTOR)";
				break;

			case OBJECT_NODE:
				{
					DWORD dwNumItems;
					NXC_DCI_VALUE *pItemList;

					if (NXCGetLastValues(s->handle, objectId, &dwNumItems, &pItemList) == RCC_SUCCESS)
					{
						string out = "<p><h2>Last DCIs</h2></p><p><table border=\"1\" cellpadding=\"3\"><tr><th>Name</th><th>Value</th><th>Date</th></tr>";
						for(DWORD i = 0; i < dwNumItems; i++)
						{
							string t = ctime((const time_t *)&pItemList[i].dwTimestamp);
							out += \
								"<tr><td>" + string(pItemList[i].szDescription) + "</td>" \
								"<td>" + string(pItemList[i].szValue) + "</td>" \
								"<td>" + t + "</td></tr>";
						}
						out += "</table></p>";

						resp.SetBody(out);
					}
					else
					{
						resp.SetBody("<i>Error while fetching last DCIs</i>");
					}

					ret = true;
				}
				break;
			case OBJECT_INTERFACE:
				{
					char szIpBuffer[32];

					IpToStr(object->dwIpAddr, szIpBuffer);
					
					resp.SetBody(
						"<p><h2>Interface " + string(object->szName) + "</h2></p><p>IP: " +
						szIpBuffer + "</p><p>Status: " +
						m_szStatusText[object->iStatus] + "</p>");

					ret = true;
				}
				break;
			}

			if (ret != true)
			{
					resp.SetBody("<i>nothing to show for this" + clText + " object class</i>");
			}
		}
	}
	else
	{
		resp.SetBody("<i>wrong request</i>");
	}
	
	ret = true;

	return ret;
}
