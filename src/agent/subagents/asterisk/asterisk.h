/*
** NetXMS Asterisk subagent
** Copyright (C) 2004-2018 Victor Kirhenshtein
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
** File: asterisk.h
**
**/

#ifndef _asterisk_h_
#define _asterisk_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>

#define DEBUG_TAG _T("sa.asterisk")

#define MAX_AMI_TAG_LEN       32
#define MAX_AMI_SUBTYPE_LEN   64

/**
 * AMI message type
 */
enum AmiMessageType
{
   AMI_UNKNOWN = 0,
   AMI_EVENT = 1,
   AMI_ACTION = 2,
   AMI_RESPONSE = 3
};

/**
 * AMI message tag
 */
struct AmiMessageTag
{
   AmiMessageTag *next;
   char name[MAX_AMI_TAG_LEN];
   char *value;

   AmiMessageTag(const char *_name, const char *_value, AmiMessageTag *_next)
   {
      strlcpy(name, _name, MAX_AMI_TAG_LEN);
      value = strdup(_value);
      next = _next;
   }

   ~AmiMessageTag()
   {
      MemFree(value);
   }
};

/**
 * AMI message
 */
class AmiMessage : public RefCountObject
{
private:
   AmiMessageType m_type;
   char m_subType[MAX_AMI_SUBTYPE_LEN];
   INT64 m_id;
   AmiMessageTag *m_tags;
   StringList *m_data;

   AmiMessage();

   AmiMessageTag *findTag(const char *name);

protected:
   virtual ~AmiMessage();

public:
   AmiMessage(const char *subType);

   AmiMessageType getType() const { return m_type; }
   const char *getSubType() const { return m_subType; }
   bool isSuccess() const { return !stricmp(m_subType, "Success") || !stricmp(m_subType, "Follows"); }

   INT64 getId() const { return m_id; }
   void setId(INT64 id) { m_id = id; }

   const char *getTag(const char *name);
   INT32 getTagAsInt32(const char *name, INT32 defaultValue = 0);
   UINT32 getTagAsUInt32(const char *name, UINT32 defaultValue = 0);
   double getTagAsDouble(const char *name, double defaultValue = 0);
   void setTag(const char *name, const char *value);

   const StringList *getData() const { return m_data; }
   StringList *acquireData() { StringList *d = m_data; m_data = NULL; return d; }

   ByteStream *serialize();

   static AmiMessage *createFromNetwork(RingBuffer& buffer);
};

/**
 * AMI event listener
 */
class AmiEventListener
{
public:
   virtual ~AmiEventListener() { }

   virtual void processEvent(AmiMessage *event) = 0;
};

/**
 * Cumulative event counters
 */
struct EventCounters
{
   UINT64 callBarred;
   UINT64 callRejected;
   UINT64 channelUnavailable;
   UINT64 congestion;
   UINT64 noRoute;
   UINT64 subscriberAbsent;
};

/**
 * Real time RTCP data
 */
struct RTCPData
{
   UINT32 count;
   UINT32 rtt;
   UINT32 jitter;
   UINT32 packetLoss;
};

/**
 * RTCP statistic element index
 */
enum RTCPStatisticElementIndex
{
   RTCP_AVG = 0,
   RTCP_LAST = 1,
   RTCP_MAX = 2,
   RTCP_MIN = 3
};

/**
 * RTCP based statistic
 */
struct RTCPStatistic
{
   UINT64 rtt[4];
   UINT64 jitter[4];
   UINT64 packetLoss[4];
};

/**
 * SIP registration test
 */
class SIPRegistrationTest
{
private:
   TCHAR *m_name;
   char *m_login;
   char *m_password;
   char *m_domain;
   char *m_proxy;
   UINT32 m_interval;
   time_t m_lastRunTime;
   INT32 m_elapsedTime;
   int m_status;
   MUTEX m_mutex;
   bool m_stop;

   void run();

public:
   SIPRegistrationTest(ConfigEntry *config, const char *defaultProxy);
   ~SIPRegistrationTest();

   const TCHAR *getName() const { return m_name; }
   const char *getLogin() const { return m_login; }
   const char *getDomain() const { return m_domain; }
   const char *getProxy() const { return m_proxy; }
   UINT32 getInterval() const { return m_interval; }
   time_t getLastRunTime() const { return GetAttributeWithLock(m_lastRunTime, m_mutex); }
   INT32 getElapsedTime() const { return GetAttributeWithLock(m_elapsedTime, m_mutex); }
   int getStatus() const { return GetAttributeWithLock(m_status, m_mutex); }

   void start();
   void stop();
};

/**
 * Asterisk system information
 */
class AsteriskSystem
{
private:
   TCHAR *m_name;
   InetAddress m_ipAddress;
   UINT16 m_port;
   char *m_login;
   char *m_password;
   SOCKET m_socket;
   THREAD m_connectorThread;
   RingBuffer m_networkBuffer;
   INT64 m_requestId;
   INT64 m_activeRequestId;
   MUTEX m_requestLock;
   CONDITION m_requestCompletion;
   AmiMessage *m_response;
   bool m_amiSessionReady;
   bool m_resetSession;
   ObjectArray<AmiEventListener> m_eventListeners;
   MUTEX m_eventListenersLock;
   UINT32 m_amiTimeout;
   EventCounters m_globalEventCounters;
   StringObjectMap<EventCounters> m_peerEventCounters;
   MUTEX m_eventCounterLock;
   StringObjectMap<RTCPData> m_rtcpData;
   StringObjectMap<RTCPStatistic> m_peerRTCPStatistic;
   MUTEX m_rtcpLock;
   StringObjectMap<SIPRegistrationTest> m_registrationTests;

   static THREAD_RESULT THREAD_CALL connectorThreadStarter(void *arg);

   AmiMessage *readMessage() { return AmiMessage::createFromNetwork(m_networkBuffer); }
   bool processMessage(AmiMessage *msg);
   void connectorThread();

   bool sendLoginRequest();
   void setupEventFilters();

   void processHangup(AmiMessage *msg);
   void processRTCP(AmiMessage *msg);
   void updateRTCPStatistic(const char *channel, const TCHAR *peer);

   AsteriskSystem(const TCHAR *name);

public:
   static AsteriskSystem *createFromConfig(ConfigEntry *config, bool defaultSystem);

   ~AsteriskSystem();

   const TCHAR *getName() const { return m_name; }
   const InetAddress& getIpAddress() const { return m_ipAddress; }
   bool isAmiSessionReady() const { return m_amiSessionReady; }

   void start();
   void stop();
   void reset();

   void addEventListener(AmiEventListener *listener);
   void removeEventListener(AmiEventListener *listener);

   AmiMessage *sendRequest(AmiMessage *request, ObjectRefArray<AmiMessage> *list = NULL, UINT32 timeout = 0);

   bool sendSimpleRequest(AmiMessage *request);
   LONG readSingleTag(const char *rqname, const char *tag, TCHAR *value);
   ObjectRefArray<AmiMessage> *readTable(const char *rqname);
   StringList *executeCommand(const char *command);

   const EventCounters *getGlobalEventCounters() const { return &m_globalEventCounters; }
   const EventCounters *getPeerEventCounters(const TCHAR *peer) const;
   RTCPStatistic *getPeerRTCPStatistic(const TCHAR *peer, RTCPStatistic *buffer) const;

   ObjectArray<SIPRegistrationTest> *getRegistrationTests() const { return m_registrationTests.values(); }
   SIPRegistrationTest *getRegistartionTest(const TCHAR *name) const { return m_registrationTests.get(name); }
};

/**
 * Get configured asterisk system by name
 */
AsteriskSystem *GetAsteriskSystemByName(const TCHAR *name);

/**
 * Thread pool
 */
extern ThreadPool *g_asteriskThreadPool;

/**
 * Get peer name from channel name
 */
char *PeerFromChannelA(const char *channel, char *peer, size_t size);
#ifdef UNICODE
WCHAR *PeerFromChannelW(const char *channel, WCHAR *peer, size_t size);
#define PeerFromChannel PeerFromChannelW
#else
#define PeerFromChannel PeerFromChannelA
#endif

/**
 * Standard prologue for parameter handler - retrieve system from first argument
 */
#define GET_ASTERISK_SYSTEM(n) \
TCHAR sysName[256]; \
if (n > 0) { \
   TCHAR temp[256]; \
   if (!AgentGetParameterArg(param, n + 1, temp, 256)) \
      return SYSINFO_RC_UNSUPPORTED; \
   if (temp[0] != 0) { \
      if (!AgentGetParameterArg(param, 1, sysName, 256)) \
         return SYSINFO_RC_UNSUPPORTED; \
   } else { \
      sysName[0] = 0; \
   } \
} else { \
if (!AgentGetParameterArg(param, 1, sysName, 256)) \
   return SYSINFO_RC_UNSUPPORTED; \
} \
AsteriskSystem *sys = GetAsteriskSystemByName((sysName[0] != 0) ? sysName : _T("LOCAL")); \
if (sys == NULL) \
   return SYSINFO_RC_NO_SUCH_INSTANCE;

/**
 * Get first argument after system ID (must be used after GET_ASTERISK_SYSTEM)
 */
#define GET_ARGUMENT(n, b, s) \
do { if (!AgentGetParameterArg(param, (sysName[0] == 0) ? n : n + 1, b, s)) return SYSINFO_RC_UNSUPPORTED; } while(0)

/**
 * Get first argument after system ID as multibyte string (must be used after GET_ASTERISK_SYSTEM)
 */
#define GET_ARGUMENT_A(n, b, s) \
do { if (!AgentGetParameterArgA(param, (sysName[0] == 0) ? n : n + 1, b, s)) return SYSINFO_RC_UNSUPPORTED; } while(0)

#endif
