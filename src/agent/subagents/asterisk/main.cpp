/*
** NetXMS Asterisk subagent
** Copyright (C) 2004-2021 Victor Kirhenshtein
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

#include "asterisk.h"
#include <netxms-version.h>

/******* Externals *******/
LONG H_ChannelList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ChannelStats(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ChannelTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_GlobalEventCounters(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_PeerEventCounters(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_PeerRTCPStats(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SIPPeerDetails(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SIPPeerList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_SIPPeerStats(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SIPPeerTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_SIPRegistrationTestData(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SIPRegistrationTestList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_SIPRegistrationTestTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_SIPTestRegistration(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_TaskProcessorDetails(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_TaskProcessorList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_TaskProcessorTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);

/**
 * Thread pool
 */
ThreadPool *g_asteriskThreadPool = nullptr;

/**
 * Configured systems
 */
static ObjectArray<AsteriskSystem> s_systems(16, 16, Ownership::True);
static StringObjectMap<AsteriskSystem> s_indexByName(Ownership::False);

/**
 * Get asterisk system by name
 */
AsteriskSystem *GetAsteriskSystemByName(const TCHAR *name)
{
   return s_indexByName.get(name);
}

/**
 * Handler for Asterisk.AMI.Status parameter
 */
static LONG H_AMIStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);
   ret_int(value, sys->isAmiSessionReady() ? 1 : 0);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Asterisk.AMI.Version parameter
 */
static LONG H_AMIVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);
   return sys->readSingleTag("CoreSettings", "AMIversion", value);
}

/**
 * Handler for Asterisk.Version parameter
 */
static LONG H_AsteriskVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);
   return sys->readSingleTag("CoreSettings", "AsteriskVersion", value);
}

/**
 * Handler for Asterisk.CurrentCalls parameter
 */
static LONG H_CurrentCalls(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);
   return sys->readSingleTag("CoreStatus", "CurrentCalls", value);
}

/**
 * Handler for Asterisk.Systems list
 */
static LONG H_SystemList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_systems.size(); i++)
      value->add(s_systems.get(i)->getName());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Asterisk.CommandOutput list
 */
static LONG H_CommandOutput(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(1);

   char command[256];
   GET_ARGUMENT_A(1, command, 256);

   StringList *output = sys->executeCommand(command);
   if (output == nullptr)
      return SYSINFO_RC_ERROR;

   value->addAll(output);
   delete output;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   unique_ptr<ObjectArray<ConfigEntry>> systems = config->getSubEntries(_T("/Asterisk/Systems"), nullptr);
   if (systems != nullptr)
   {
      for(int i = 0; i < systems->size(); i++)
      {
         AsteriskSystem *s = AsteriskSystem::createFromConfig(systems->get(i), false);
         if (s != nullptr)
         {
            s_systems.add(s);
            s_indexByName.set(s->getName(), s);
            nxlog_debug_tag(DEBUG_TAG, 1, _T("Added Asterisk system %s"), s->getName());
         }
         else
         {
            AgentWriteLog(NXLOG_WARNING, _T("Asterisk: cannot add system %s definition from config"), systems->get(i)->getName());
         }
      }
   }

   // Create default "LOCAL" system if explicitly enabled or other systems are not defined
   ConfigEntry *root = config->getEntry(_T("/Asterisk"));
   if ((root != nullptr) &&
       (s_systems.isEmpty() || (root->getSubEntryValue(_T("Login")) != nullptr) || (root->getSubEntryValue(_T("Port")) != nullptr)))
   {
      AsteriskSystem *s = AsteriskSystem::createFromConfig(root, true);
      if (s != nullptr)
      {
         s_systems.add(s);
         s_indexByName.set(s->getName(), s);
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Added Asterisk system %s"), s->getName());
      }
   }

   g_asteriskThreadPool = ThreadPoolCreate(_T("ASTERISK"), 1, 64, 0);

   for(int i = 0; i < s_systems.size(); i++)
   {
      s_systems.get(i)->start();
   }
	return true;
}

/**
 * Shutdown subagent
 */
static void SubagentShutdown()
{
   for(int i = 0; i < s_systems.size(); i++)
   {
      s_systems.get(i)->stop();
   }
   nxlog_debug_tag(DEBUG_TAG, 4, _T("All AMI connectors stopped"));
   ThreadPoolDestroy(g_asteriskThreadPool);
}

/**
 * Parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Asterisk.AMI.Status"), H_AMIStatus, nullptr, DCI_DT_INT, _T("Asterisk: AMI connection status") },
   { _T("Asterisk.AMI.Status(*)"), H_AMIStatus, nullptr, DCI_DT_INT, _T("Asterisk: AMI connection status") },
   { _T("Asterisk.AMI.Version"), H_AMIVersion, nullptr, DCI_DT_STRING, _T("Asterisk: AMI version") },
   { _T("Asterisk.AMI.Version(*)"), H_AMIVersion, nullptr, DCI_DT_STRING, _T("Asterisk: AMI version") },
   { _T("Asterisk.Channels.Active"), H_ChannelStats, _T("A"), DCI_DT_UINT, _T("Asterisk: active channels") },
   { _T("Asterisk.Channels.Active(*)"), H_ChannelStats, _T("A"), DCI_DT_UINT, _T("Asterisk: active channels") },
   { _T("Asterisk.Channels.Busy"), H_ChannelStats, _T("B"), DCI_DT_UINT, _T("Asterisk: busy channels") },
   { _T("Asterisk.Channels.Busy(*)"), H_ChannelStats, _T("B"), DCI_DT_UINT, _T("Asterisk: busy channels") },
   { _T("Asterisk.Channels.Dialing"), H_ChannelStats, _T("D"), DCI_DT_UINT, _T("Asterisk: dialing channels") },
   { _T("Asterisk.Channels.Dialing(*)"), H_ChannelStats, _T("D"), DCI_DT_UINT, _T("Asterisk: dialing channels") },
   { _T("Asterisk.Channels.OffHook"), H_ChannelStats, _T("O"), DCI_DT_UINT, _T("Asterisk: off-hook channels") },
   { _T("Asterisk.Channels.OffHook(*)"), H_ChannelStats, _T("O"), DCI_DT_UINT, _T("Asterisk: off-hook channels") },
   { _T("Asterisk.Channels.Reserved"), H_ChannelStats, _T("R"), DCI_DT_UINT, _T("Asterisk: reserved channels") },
   { _T("Asterisk.Channels.Reserved(*)"), H_ChannelStats, _T("R"), DCI_DT_UINT, _T("Asterisk: reserved channels") },
   { _T("Asterisk.Channels.Ringing"), H_ChannelStats, _T("r"), DCI_DT_UINT, _T("Asterisk: ringing channels") },
   { _T("Asterisk.Channels.Ringing(*)"), H_ChannelStats, _T("r"), DCI_DT_UINT, _T("Asterisk: ringing channels") },
   { _T("Asterisk.Channels.Up"), H_ChannelStats, _T("U"), DCI_DT_UINT, _T("Asterisk: up channels") },
   { _T("Asterisk.Channels.Up(*)"), H_ChannelStats, _T("U"), DCI_DT_UINT, _T("Asterisk: up channels") },
   { _T("Asterisk.CurrentCalls"), H_CurrentCalls, nullptr, DCI_DT_UINT, _T("Asterisk: current calls") },
   { _T("Asterisk.CurrentCalls(*)"), H_CurrentCalls, nullptr, DCI_DT_UINT, _T("Asterisk: current calls") },
   { _T("Asterisk.Events.CallBarred"), H_GlobalEventCounters, _T("B"), DCI_DT_COUNTER64, _T("Asterisk: call barred events") },
   { _T("Asterisk.Events.CallBarred(*)"), H_GlobalEventCounters, _T("B"), DCI_DT_COUNTER64, _T("Asterisk: call barred events") },
   { _T("Asterisk.Events.CallRejected"), H_GlobalEventCounters, _T("R"), DCI_DT_COUNTER64, _T("Asterisk: call rejected events") },
   { _T("Asterisk.Events.CallRejected(*)"), H_GlobalEventCounters, _T("R"), DCI_DT_COUNTER64, _T("Asterisk: call rejected events") },
   { _T("Asterisk.Events.ChannelUnavailable"), H_GlobalEventCounters, _T("U"), DCI_DT_COUNTER64, _T("Asterisk: channel unavailable events") },
   { _T("Asterisk.Events.ChannelUnavailable(*)"), H_GlobalEventCounters, _T("U"), DCI_DT_COUNTER64, _T("Asterisk: channel unavailable events") },
   { _T("Asterisk.Events.Congestion"), H_GlobalEventCounters, _T("C"), DCI_DT_COUNTER64, _T("Asterisk: congestion events") },
   { _T("Asterisk.Events.Congestion(*)"), H_GlobalEventCounters, _T("C"), DCI_DT_COUNTER64, _T("Asterisk: congestion events") },
   { _T("Asterisk.Events.NoRoute"), H_GlobalEventCounters, _T("N"), DCI_DT_COUNTER64, _T("Asterisk: no route events") },
   { _T("Asterisk.Events.NoRoute(*)"), H_GlobalEventCounters, _T("N"), DCI_DT_COUNTER64, _T("Asterisk: no route events") },
   { _T("Asterisk.Events.SubscriberAbsent"), H_GlobalEventCounters, _T("A"), DCI_DT_COUNTER64, _T("Asterisk: subscriber absent events") },
   { _T("Asterisk.Events.SubscriberAbsent(*)"), H_GlobalEventCounters, _T("A"), DCI_DT_COUNTER64, _T("Asterisk: subscriber absent events") },
   { _T("Asterisk.Peer.Events.CallBarred(*)"), H_PeerEventCounters, _T("B"), DCI_DT_COUNTER64, _T("Asterisk: peer {instance} call barred events") },
   { _T("Asterisk.Peer.Events.CallRejected(*)"), H_PeerEventCounters, _T("R"), DCI_DT_COUNTER64, _T("Asterisk: peer {instance} call rejected events") },
   { _T("Asterisk.Peer.Events.ChannelUnavailable(*)"), H_PeerEventCounters, _T("U"), DCI_DT_COUNTER64, _T("Asterisk: peer {instance} channel unavailable events") },
   { _T("Asterisk.Peer.Events.Congestion(*)"), H_PeerEventCounters, _T("C"), DCI_DT_COUNTER64, _T("Asterisk: peer {instance} congestion events") },
   { _T("Asterisk.Peer.Events.NoRoute(*)"), H_PeerEventCounters, _T("N"), DCI_DT_COUNTER64, _T("Asterisk: peer {instance} no route events") },
   { _T("Asterisk.Peer.Events.SubscriberAbsent(*)"), H_PeerEventCounters, _T("A"), DCI_DT_COUNTER64, _T("Asterisk: peer {instance} subscriber absent events") },
   { _T("Asterisk.Peer.RTCP.AverageJitter(*)"), H_PeerRTCPStats, _T("JA"), DCI_DT_INT, _T("Asterisk: peer {instance} average jitter") },
   { _T("Asterisk.Peer.RTCP.AveragePacketLoss(*)"), H_PeerRTCPStats, _T("PA"), DCI_DT_INT, _T("Asterisk: peer {instance} average packet loss") },
   { _T("Asterisk.Peer.RTCP.AverageRTT(*)"), H_PeerRTCPStats, _T("RA"), DCI_DT_INT, _T("Asterisk: peer {instance} average RTT") },
   { _T("Asterisk.Peer.RTCP.LastJitter(*)"), H_PeerRTCPStats, _T("JL"), DCI_DT_INT, _T("Asterisk: peer {instance} last call jitter") },
   { _T("Asterisk.Peer.RTCP.LastPacketLoss(*)"), H_PeerRTCPStats, _T("PL"), DCI_DT_INT, _T("Asterisk: peer {instance} last call packet loss") },
   { _T("Asterisk.Peer.RTCP.LastRTT(*)"), H_PeerRTCPStats, _T("RL"), DCI_DT_INT, _T("Asterisk: peer {instance} last call RTT") },
   { _T("Asterisk.Peer.RTCP.MaxJitter(*)"), H_PeerRTCPStats, _T("JM"), DCI_DT_INT, _T("Asterisk: peer {instance} maximum jitter") },
   { _T("Asterisk.Peer.RTCP.MaxPacketLoss(*)"), H_PeerRTCPStats, _T("PM"), DCI_DT_INT, _T("Asterisk: peer {instance} maximum packet loss") },
   { _T("Asterisk.Peer.RTCP.MaxRTT(*)"), H_PeerRTCPStats, _T("RM"), DCI_DT_INT, _T("Asterisk: peer {instance} maximum RTT") },
   { _T("Asterisk.Peer.RTCP.MinJitter(*)"), H_PeerRTCPStats, _T("Jm"), DCI_DT_INT, _T("Asterisk: peer {instance} minimum jitter") },
   { _T("Asterisk.Peer.RTCP.MinPacketLoss(*)"), H_PeerRTCPStats, _T("Pm"), DCI_DT_INT, _T("Asterisk: peer {instance} minimum packet loss") },
   { _T("Asterisk.Peer.RTCP.MinRTT(*)"), H_PeerRTCPStats, _T("Rm"), DCI_DT_INT, _T("Asterisk: peer {instance} minimum RTT") },
   { _T("Asterisk.SIP.Peer.Details(*)"), H_SIPPeerDetails, nullptr, DCI_DT_STRING, _T("Asterisk: SIP peer {instance} detailed information") },
   { _T("Asterisk.SIP.Peer.IPAddress(*)"), H_SIPPeerDetails, _T("I"), DCI_DT_STRING, _T("Asterisk: SIP peer {instance} IP address") },
   { _T("Asterisk.SIP.Peer.Status(*)"), H_SIPPeerDetails, _T("S"), DCI_DT_STRING, _T("Asterisk: SIP peer {instance} status") },
   { _T("Asterisk.SIP.Peer.Type(*)"), H_SIPPeerDetails, _T("T"), DCI_DT_STRING, _T("Asterisk: SIP peer {instance} type") },
   { _T("Asterisk.SIP.Peer.UserAgent(*)"), H_SIPPeerDetails, _T("U"), DCI_DT_STRING, _T("Asterisk: SIP peer {instance} user agent") },
   { _T("Asterisk.SIP.Peer.VoiceMailbox(*)"), H_SIPPeerDetails, _T("V"), DCI_DT_STRING, _T("Asterisk: SIP peer {instance} voice mailbox") },
   { _T("Asterisk.SIP.Peers.Connected(*)"), H_SIPPeerStats, _T("C"), DCI_DT_UINT, _T("Asterisk: connected SIP peers") },
   { _T("Asterisk.SIP.Peers.Total(*)"), H_SIPPeerStats, _T("T"), DCI_DT_UINT, _T("Asterisk: total SIP peers") },
   { _T("Asterisk.SIP.Peers.Unknown(*)"), H_SIPPeerStats, _T("U"), DCI_DT_UINT, _T("Asterisk: unknown state SIP peers") },
   { _T("Asterisk.SIP.Peers.Unmonitored(*)"), H_SIPPeerStats, _T("M"), DCI_DT_UINT, _T("Asterisk: unmonitored SIP peers") },
   { _T("Asterisk.SIP.Peers.Unreachable(*)"), H_SIPPeerStats, _T("R"), DCI_DT_UINT, _T("Asterisk: unreachable SIP peers") },
   { _T("Asterisk.SIP.RegistrationTest.ElapsedTime(*)"), H_SIPRegistrationTestData, _T("E"), DCI_DT_INT, _T("Asterisk: SIP client registration test {instance} elapsed time") },
   { _T("Asterisk.SIP.RegistrationTest.Status(*)"), H_SIPRegistrationTestData, _T("S"), DCI_DT_INT, _T("Asterisk: SIP client registration test {instance} status") },
   { _T("Asterisk.SIP.RegistrationTest.StatusText(*)"), H_SIPRegistrationTestData, _T("s"), DCI_DT_STRING, _T("Asterisk: SIP client registration test {instance} status text") },
   { _T("Asterisk.SIP.RegistrationTest.Timestamp(*)"), H_SIPRegistrationTestData, _T("T"), DCI_DT_INT64, _T("Asterisk: SIP client registration test {instance} timestamp") },
   { _T("Asterisk.SIP.TestRegistration(*)"), H_SIPTestRegistration, nullptr, DCI_DT_INT, _T("Asterisk: ad-hoc SIP client registration test") },
   { _T("Asterisk.TaskProcessor.HighWatermark(*)"), H_TaskProcessorDetails, _T("H"), DCI_DT_UINT, _T("Asterisk: task processor {instance} high watermark") },
   { _T("Asterisk.TaskProcessor.LowWatermark(*)"), H_TaskProcessorDetails, _T("L"), DCI_DT_UINT, _T("Asterisk: task processor {instance} low watermark") },
   { _T("Asterisk.TaskProcessor.MaxDepth(*)"), H_TaskProcessorDetails, _T("M"), DCI_DT_UINT, _T("Asterisk: task processor {instance} max queue depth") },
   { _T("Asterisk.TaskProcessor.Processed(*)"), H_TaskProcessorDetails, _T("P"), DCI_DT_COUNTER64, _T("Asterisk: task processor {instance} processed tasks") },
   { _T("Asterisk.TaskProcessor.Queued(*)"), H_TaskProcessorDetails, _T("Q"), DCI_DT_UINT, _T("Asterisk: task processor {instance} queued tasks") },
   { _T("Asterisk.Version"), H_AsteriskVersion, nullptr, DCI_DT_STRING, _T("Asterisk: version") },
   { _T("Asterisk.Version(*)"), H_AsteriskVersion, nullptr, DCI_DT_STRING, _T("Asterisk: version") }
};

/**
 * Lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
	{ _T("Asterisk.Channels"), H_ChannelList, nullptr },
   { _T("Asterisk.Channels(*)"), H_ChannelList, nullptr },
   { _T("Asterisk.CommandOutput(*)"), H_CommandOutput, nullptr },
   { _T("Asterisk.SIP.Peers"), H_SIPPeerList, nullptr },
   { _T("Asterisk.SIP.Peers(*)"), H_SIPPeerList, nullptr },
   { _T("Asterisk.SIP.RegistrationTests"), H_SIPRegistrationTestList, nullptr },
   { _T("Asterisk.SIP.RegistrationTests(*)"), H_SIPRegistrationTestList, nullptr },
   { _T("Asterisk.Systems"), H_SystemList, nullptr },
   { _T("Asterisk.TaskProcessors"), H_TaskProcessorList, nullptr },
   { _T("Asterisk.TaskProcessors(*)"), H_TaskProcessorList, nullptr }
};

/**
 * Tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("Asterisk.Channels"), H_ChannelTable, nullptr, _T("CHANNEL"), _T("Asterisk: channels") },
   { _T("Asterisk.Channels(*)"), H_ChannelTable, nullptr, _T("CHANNEL"), _T("Asterisk: channels") },
   { _T("Asterisk.SIP.Peers"), H_SIPPeerTable, nullptr, _T("NAME"), _T("Asterisk: SIP peers") },
   { _T("Asterisk.SIP.Peers(*)"), H_SIPPeerTable, nullptr, _T("NAME"), _T("Asterisk: SIP peers") },
   { _T("Asterisk.SIP.RegistrationTests"), H_SIPRegistrationTestTable, nullptr, _T("NAME"), _T("Asterisk: configured SIP registration tests") },
   { _T("Asterisk.SIP.RegistrationTests(*)"), H_SIPRegistrationTestTable, nullptr, _T("NAME"), _T("Asterisk: configured SIP registration tests") },
   { _T("Asterisk.TaskProcessors"), H_TaskProcessorTable, nullptr, _T("NAME"), _T("Asterisk: task processors") },
   { _T("Asterisk.TaskProcessors(*)"), H_TaskProcessorTable, nullptr, _T("NAME"), _T("Asterisk: task processors") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("ASTERISK"), NETXMS_VERSION_STRING,
	SubagentInit, SubagentShutdown, nullptr, nullptr, nullptr,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	s_lists,
   sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
   s_tables,
   0, nullptr,	// actions
	0, nullptr	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(ASTERISK)
{
	*ppInfo = &s_info;
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
