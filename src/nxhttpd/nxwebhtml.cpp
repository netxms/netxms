#include "NxWeb.h"

#define XML_HEADER string("<?xml version='1.0'?>\r\n<?xml-stylesheet type=\"text/xsl\" href=\"/nxweb.xsl\"?>\r\n")

string NxWeb::_Login(HttpRequest req, string err)
{
	string out = XML_HEADER + "<nxweb><login>";

	if (err.size() > 0)
	{
		out += "<error>" + FilterEnt(err) + "</error>";
	}

	out += "</login></nxweb>";

	return out;
}


string NxWeb::_Overview(HttpRequest req, string sid, NXC_SERVER_STATS *status)
{
	string out = XML_HEADER + "<nxweb><sid>" + sid + "</sid><overview><server>";
	char szTmp[1024];

	snprintf(szTmp, sizeof(szTmp), "<item name=\"Server uptime\">%.0fh</item>", (float)status->dwServerUptime / 3600);
	out += szTmp; 
	snprintf(szTmp, sizeof(szTmp), "<item name=\"Object count\">%d</item>", status->dwNumObjects);
	out += szTmp; 
	snprintf(szTmp, sizeof(szTmp), "<item name=\"Node count\">%d</item>", status->dwNumNodes);
	out += szTmp; 
	snprintf(szTmp, sizeof(szTmp), "<item name=\"DCI count\">%d</item>", status->dwNumDCI);
	out += szTmp; 
	snprintf(szTmp, sizeof(szTmp), "<item name=\"Connected clients\">%d</item>", status->dwNumClientSessions);
	out += szTmp; 
	snprintf(szTmp, sizeof(szTmp), "<item name=\"Server version\">%s</item>", status->szServerVersion);
	out += szTmp; 

	out += "</server>";

	if (status->dwNumAlarms > 0)
	{
		out += "<alarms>";
		snprintf(szTmp, sizeof(szTmp), "<normal>%d</normal>", status->dwAlarmsBySeverity[0]);
		out += szTmp; 
		snprintf(szTmp, sizeof(szTmp), "<warning>%d</warning>", status->dwAlarmsBySeverity[1]);
		out += szTmp; 
		snprintf(szTmp, sizeof(szTmp), "<minor>%d</minor>", status->dwAlarmsBySeverity[2]);
		out += szTmp; 
		snprintf(szTmp, sizeof(szTmp), "<major>%d</major>", status->dwAlarmsBySeverity[3]);
		out += szTmp; 
		snprintf(szTmp, sizeof(szTmp), "<critical>%d</critical>", status->dwAlarmsBySeverity[4]);
		out += szTmp; 
		out += "</alarms>";
	}

	out += "</overview></nxweb>";

	return out;
}

string NxWeb::_Alarms(HttpRequest req, Session *s)
{
	string out = XML_HEADER + "<nxweb><sid>" + s->sid + "</sid><alarms>";

	string sev[] = {
		"Normal", "Warning", "Minor", "Major", "Critical",
			"Unknown", "Unmanaged", "Disabled", "Testing"};

	DWORD size;
	NXC_ALARM *alarms;

	if (NXCLoadAllAlarms(s->handle, FALSE, &size, &alarms) == RCC_SUCCESS)
	{
		for (int i = 0; i < size; i++)
		{
			char szTmp[1024];
			char szTime[32];
			NXC_OBJECT *pObject;

			pObject = NXCFindObjectById(s->handle,
				alarms[i].dwSourceObject);

			strftime(szTime, sizeof(szTime), "%d/%m/%Y %H:%M:%S",
				localtime((const time_t *)&alarms[i].dwLastChangeTime));
			snprintf(szTmp, sizeof(szTmp),
				"<alarm id=\"%d\" severity=\"%s\" source=\"%s\" timestamp=\"%s\">",
				alarms[i].dwAlarmId,
				sev[alarms[i].nCurrentSeverity].c_str(),
				pObject == NULL ? "[UNKNOWN]" : pObject->szName,
				szTime);

			out += szTmp;
			out += alarms[i].szMessage;
			out += "</alarm>";
		}
	}

	out += "</alarms></nxweb>";

	return out;
}

string NxWeb::_Objects(HttpRequest req, string sid)
{
	return XML_HEADER + "<nxweb><sid>" + sid + "</sid><objects /></nxweb>";

/*	out += \
	"<script>" \
	"	function showObjectInfo(id)" \
	"	{" \
	"		var xmlHttp = XmlHttp.create();" \
	"		xmlHttp.open(\"GET\", \"/netxms.app?cmd=objectInfo&sid=" + sid + "&id=\" + id, false);" \
	"		xmlHttp.send(null);" \
	"		document.getElementById(\"objectData\").innerHTML = xmlHttp.responseText;" \
	"	}" \
	"</script>" \
	;*/
}
