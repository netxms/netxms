/*
** NetXMS SMS sending subagent
** Copyright (C) 2006-2014 Raden Solutions
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
** File: main.cpp
**
**/

#include "sms.h"

/**
 * Model of attacjed modem
 */
TCHAR g_szDeviceModel[256] = _T("<unknown>");

/**
 * Port name
 */
static TCHAR m_szDevice[MAX_PATH];

/**
 * Handler for SMS.SerialConfig and SMS.DeviceModel
 */
static LONG H_StringConst(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	ret_string(pValue, pArg);
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for SMS.Send action
 */
static LONG H_SendSMS(const TCHAR *pszAction, const StringList *pArgs, const TCHAR *pData, AbstractCommSession *session)
{
	if (pArgs->size() < 2)
		return ERR_BAD_ARGUMENTS;

#ifdef UNICODE
	char *rcpt = MBStringFromWideString(pArgs->get(0));
	char *text = MBStringFromWideString(pArgs->get(1));
	LONG rc = SendSMS(rcpt, text) ? ERR_SUCCESS : ERR_INTERNAL_ERROR;
	MemFree(rcpt);
	MemFree(text);
#else
	LONG rc = SendSMS(pArgs->get(0), pArgs->get(1)) ? ERR_SUCCESS : ERR_INTERNAL_ERROR;
#endif
	return rc;
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
	// Parse configuration
	const TCHAR *value = config->getValue(_T("/SMS/Device"));
	if (value != NULL)
	{
		nx_strncpy(m_szDevice, value, MAX_PATH);
		if (!InitSender(m_szDevice))
			return false;
	}
	else
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("SMS: device not specified"));
	}

	return value != NULL;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
}

/**
 * Subagent parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("SMS.DeviceModel"), H_StringConst, g_szDeviceModel, DCI_DT_STRING, _T("Current serial port configuration") },
	{ _T("SMS.SerialConfig"), H_StringConst, m_szDevice, DCI_DT_STRING, _T("Current serial port configuration") }
};

/**
 * Subagent actions
 */
static NETXMS_SUBAGENT_ACTION m_actions[] =
{
	{ _T("SMS.Send"), H_SendSMS, NULL, _T("Send SMS") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("SMS"), NETXMS_BUILD_TAG,
	SubAgentInit, SubAgentShutdown, NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, NULL,	// lists
	0, NULL,	// tables
	sizeof(m_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
	m_actions,
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(SMS)
{
	*ppInfo = &m_info;
	return true;
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
