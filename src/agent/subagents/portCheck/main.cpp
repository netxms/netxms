/* $Id: main.cpp,v 1.2 2005-01-19 13:42:47 alk Exp $ */

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"
#include "pop3.h"
#include "ssh.h"

//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { "ServiceCheck.POP3(*)",         H_CheckPOP3,       NULL },
   { "ServiceCheck.SSH(*)",          H_CheckSSH,        NULL },
};

static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   //{ "Net.ArpCache",                 H_NetArpCache,     NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	"portCheck",
	NETXMS_VERSION_STRING,
	NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};

//
// Entry point for NetXMS agent
//

extern "C" BOOL NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo)
{
   *ppInfo = &m_info;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.1.1.1  2005/01/18 18:38:54  alk
Initial import

implemented:
	ServiceCheck.POP3(host, user, password) - connect to host:110 and try to login


*/
