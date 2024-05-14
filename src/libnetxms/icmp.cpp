/* 
** libnetxms - Common NetXMS utility library
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: icmp.cpp
**
**/

#include "libnetxms.h"

#ifdef _WIN32

#include <iphlpapi.h>
#include <icmpapi.h>

#define MIN_PING_SIZE      28
#define MAX_PING_SIZE      8192

/**
 * Do an ICMP ping to specific address
 * Return value: ICMP error code
 * Parameters: addr - IP address
 *             numRetries - number of retries
 *             timeout - Timeout waiting for response in milliseconds
 *             rtt - pointer to save round trip time
 *             packetSize - ping packet size in bytes
 *             dontFragment - if true "don't fragment" flag will be set on outgoing packet
 */
uint32_t LIBNETXMS_EXPORTABLE IcmpPing(const InetAddress &addr, int numRetries, uint32_t timeout, uint32_t *prtt, uint32_t packetSize, bool dontFragment)
{
   static char payload[MAX_PING_SIZE] = "NetXMS ICMP probe [01234567890]";

   HANDLE hIcmpFile = (addr.getFamily() == AF_INET) ? IcmpCreateFile() : Icmp6CreateFile();
   if (hIcmpFile == INVALID_HANDLE_VALUE)
      return ICMP_API_ERROR;

   // Check packet size and adjust to indicate payload size
   if (packetSize < MIN_PING_SIZE)
      packetSize = 0;
   else if (packetSize > MAX_PING_SIZE)
      packetSize = MAX_PING_SIZE - MIN_PING_SIZE;
   else
      packetSize -= MIN_PING_SIZE;

   DWORD replySize = packetSize + 16 + ((addr.getFamily() == AF_INET) ? sizeof(ICMP_ECHO_REPLY) : sizeof(ICMPV6_ECHO_REPLY));
	char *reply = (char *)alloca(replySize);
	int retries = numRetries;
	uint32_t rc = ICMP_API_ERROR;
	do
	{
		if (addr.getFamily() == AF_INET)
      {
         IP_OPTION_INFORMATION opt;
         memset(&opt, 0, sizeof(opt));
         opt.Ttl = 127;
         if (dontFragment)
            opt.Flags = IP_FLAG_DF;
         rc = IcmpSendEcho(hIcmpFile, htonl(addr.getAddressV4()), payload, (WORD)packetSize, &opt, reply, replySize, timeout);
      }
      else
      {
         sockaddr_in6 sa, da;

         memset(&sa, 0, sizeof(sa));
         sa.sin6_addr = in6addr_any;
         sa.sin6_family = AF_INET6;

         memset(&da, 0, sizeof(da));
         da.sin6_family = AF_INET6;
         memcpy(da.sin6_addr.s6_addr, addr.getAddressV6(), 16);

         IP_OPTION_INFORMATION opt;
         memset(&opt, 0, sizeof(opt));
         opt.Ttl = 127;
         if (dontFragment)
            opt.Flags = IP_FLAG_DF;
         rc = Icmp6SendEcho2(hIcmpFile, nullptr, nullptr, nullptr, &sa, &da, payload, (WORD)packetSize, &opt, reply, replySize, timeout);
      }
		if (rc != 0)
		{
         ULONG status;
         ULONG rtt;
         if (addr.getFamily() == AF_INET)
         {
#if defined(_WIN64)
			   ICMP_ECHO_REPLY32 *er = (ICMP_ECHO_REPLY32 *)reply;
#else
			   ICMP_ECHO_REPLY *er = (ICMP_ECHO_REPLY *)reply;
#endif
            status = er->Status;
            rtt = er->RoundTripTime;
         }
         else
         {
			   ICMPV6_ECHO_REPLY *er = (ICMPV6_ECHO_REPLY *)reply;
            status = er->Status;
            rtt = er->RoundTripTime;
         }

			switch(status)
			{
				case IP_SUCCESS:
					rc = ICMP_SUCCESS;
					if (prtt != nullptr)
						*prtt = rtt;
					break;
				case IP_REQ_TIMED_OUT:
					rc = ICMP_TIMEOUT;
					break;
				case IP_BUF_TOO_SMALL:
				case IP_NO_RESOURCES:
				case IP_PACKET_TOO_BIG:
				case IP_GENERAL_FAILURE:
					rc = ICMP_API_ERROR;
					break;
				default:
					rc = ICMP_UNREACHABLE;
					break;
			}
		}
		else
		{
			rc = (GetLastError() == IP_REQ_TIMED_OUT) ? ICMP_TIMEOUT : ICMP_API_ERROR;
		}
		retries--;
   }
   while((rc != ICMP_SUCCESS) && (retries > 0));

   IcmpCloseHandle(hIcmpFile);
   return rc;
}

#else	/* not _WIN32 */

#include <nxnet.h>

/**
 * Ping request state
 */
enum PingRequestState
{
   PENDING = 0,
   IN_PROGRESS = 1,
   COMPLETED = 2
};

/**
 * Ping request
 */
struct PingRequest
{
   PingRequest *next;
   uint64_t timestamp;
   InetAddress address;
   uint32_t packetSize;
   uint32_t result;
   uint32_t rtt;
   uint16_t id;
   uint16_t sequence;
   bool dontFragment;
   PingRequestState state;
#ifdef _USE_GNU_PTH
   pth_cond_t wakeupCondition;
#else
   pthread_cond_t wakeupCondition;
#endif
};

/**
 * Max number of requests per processor
 */
#define MAX_REQESTS_PER_PROCESSOR   256

/**
 * Mark request as completed from processing thread
 */
static inline void CloseRequest(PingRequest *r, int32_t result)
{
   if (r->state != COMPLETED)
   {
      r->state = COMPLETED;
      r->result = result;
#ifdef _USE_GNU_PTH
      pth_cond_notify(&r->wakeupCondition, false);
#else
      pthread_cond_signal(&r->wakeupCondition);
#endif
   }
}

/**
 * Processor ID
 */
static VolatileCounter s_nextProcessorId = 0;

/**
 * Request processor
 */
class PingRequestProcessor
{
private:
   PingRequest *m_head;
#ifdef _USE_GNU_PTH
   pth_mutex_t m_mutex;
#else
   pthread_mutex_t m_mutex;
#endif
   SOCKET m_dataSocket;
   SOCKET m_controlSockets[2];
   THREAD m_processingThread;
   time_t m_lastSocketOpenAttempt;
   uint16_t m_id;
   uint16_t m_sequence;
   int m_family;
   bool m_shutdown;
   VolatileCounter m_usage;

   bool openSocket();
   void processingThread();

   void receivePacketV4();
   void receivePacketV6();
   void processEchoReply(const InetAddress& addr, uint16_t id, uint16_t sequence);
   void processHostUnreachable(const InetAddress& addr);

   void sendRequestV4(PingRequest *request);
   void sendRequestV6(PingRequest *request);
   bool sendRequest(PingRequest *request)
   {
      if (m_family == AF_INET)
         sendRequestV4(request);
      else
         sendRequestV6(request);
      return request->state == IN_PROGRESS;
   }

public:
   PingRequestProcessor(int family);
   ~PingRequestProcessor();

   uint32_t ping(const InetAddress &addr, uint32_t timeout, uint32_t *rtt, uint32_t packetSize, bool dontFragment);

   int getFamily() const { return m_family; }

   bool acquire()
   {
      if (InterlockedIncrement(&m_usage) <= MAX_REQESTS_PER_PROCESSOR)
         return true;
      InterlockedDecrement(&m_usage);
      return false;
   }

   void release()
   {
      InterlockedDecrement(&m_usage);
   }
};

/**
 * Constructor
 */
PingRequestProcessor::PingRequestProcessor(int family)
{
   m_head = MemAllocStruct<PingRequest>();
   m_dataSocket = INVALID_SOCKET;
   m_controlSockets[0] = INVALID_SOCKET;
   m_controlSockets[1] = INVALID_SOCKET;
   m_processingThread = INVALID_THREAD_HANDLE;
   m_lastSocketOpenAttempt = 0;
   m_id = InterlockedIncrement(&s_nextProcessorId);
   m_sequence = 0;
   m_family = family;
   m_shutdown = false;
#ifdef _USE_GNU_PTH
   pth_mutex_init(&m_mutex);
#else
#if HAVE_DECL_PTHREAD_MUTEX_ADAPTIVE_NP
   pthread_mutexattr_t a;
   pthread_mutexattr_init(&a);
   MUTEXATTR_SETTYPE(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
   pthread_mutex_init(&m_mutex, &a);
   pthread_mutexattr_destroy(&a);
#else
   pthread_mutex_init(&m_mutex, nullptr);
#endif
#endif
}

/**
 * Destructor
 */
PingRequestProcessor::~PingRequestProcessor()
{
#ifdef _USE_GNU_PTH
   pth_mutex_acquire(&m_mutex, FALSE, nullptr);
#else
   pthread_mutex_lock(&m_mutex);
#endif
   m_shutdown = true;
#ifdef _USE_GNU_PTH
   pth_mutex_release(&m_mutex);
#else
   pthread_mutex_unlock(&m_mutex);
#endif

   if (m_controlSockets[1] != INVALID_SOCKET)
      write(m_controlSockets[1], "S", 1);

   ThreadJoin(m_processingThread);
   MemFree(m_head);
#ifndef _USE_GNU_PTH
   pthread_mutex_destroy(&m_mutex);
#endif

   close(m_dataSocket);
   close(m_controlSockets[0]);
   close(m_controlSockets[1]);
}

/**
 * Open sockets
 */
bool PingRequestProcessor::openSocket()
{
   time_t now = time(nullptr);
   if (now - m_lastSocketOpenAttempt < 60)
      return false;

   if (m_dataSocket == INVALID_SOCKET)
   {
#ifdef WITH_IPV6
      if (m_family == AF_INET6)
         m_dataSocket = CreateSocket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
      else
         m_dataSocket = CreateSocket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
#else
      m_dataSocket = CreateSocket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
#endif
   }

   m_lastSocketOpenAttempt = now;
   return m_dataSocket != INVALID_SOCKET;
}

/**
 * Receiver thread
 */
void PingRequestProcessor::processingThread()
{
   SocketPoller sp;
   while(!m_shutdown)
   {
      sp.reset();
      sp.add(m_dataSocket);
      sp.add(m_controlSockets[0]);
      if (sp.poll(30000) <= 0)
         continue;

      if (sp.isSet(m_controlSockets[0]))
      {
         char command = 0;
         read(m_controlSockets[0], &command, 1);
         if (command == 'S')
            break;
      }

      if (sp.isSet(m_dataSocket))
      {
#ifdef _USE_GNU_PTH
         pth_mutex_acquire(&m_mutex, FALSE, nullptr);
#else
         pthread_mutex_lock(&m_mutex);
#endif
         if (m_family == AF_INET)
            receivePacketV4();
         else
            receivePacketV6();
#ifdef _USE_GNU_PTH
         pth_mutex_release(&m_mutex);
#else
         pthread_mutex_unlock(&m_mutex);
#endif
      }
   }

   // Cancel all pending requests
#ifdef _USE_GNU_PTH
   pth_mutex_acquire(&m_mutex, FALSE, nullptr);
#else
   pthread_mutex_lock(&m_mutex);
#endif
   for(PingRequest *r = m_head->next; r != nullptr; r = r->next)
      CloseRequest(r, ICMP_API_ERROR);
   m_head->next = nullptr;
#ifdef _USE_GNU_PTH
   pth_mutex_release(&m_mutex);
#else
   pthread_mutex_unlock(&m_mutex);
#endif
}

/**
 * Process echo reply
 */
void PingRequestProcessor::processEchoReply(const InetAddress& addr, uint16_t id, uint16_t sequence)
{
   for(PingRequest *r = m_head->next; r != nullptr; r = r->next)
   {
      if (r->address.equals(addr) && (r->id == id) && (r->sequence == sequence))
      {
         r->rtt = GetCurrentTimeMs() - r->timestamp;
         CloseRequest(r, ICMP_SUCCESS);
         break;
      }
   }
}

/**
 * Process "host unreachable" notification
 */
void PingRequestProcessor::processHostUnreachable(const InetAddress& addr)
{
   for(PingRequest *r = m_head->next; r != nullptr; r = r->next)
      if (r->address.equals(addr))
         CloseRequest(r, ICMP_UNREACHABLE);
}

/**
 * Receive IPv4 packet
 */
void PingRequestProcessor::receivePacketV4()
{
   socklen_t addrLen = sizeof(struct sockaddr_in);
   struct sockaddr_in saSrc;
   ICMP_ECHO_REPLY reply;
   if (recvfrom(m_dataSocket, (char *)&reply, sizeof(ICMP_ECHO_REPLY), 0, (struct sockaddr *)&saSrc, &addrLen) <= 0)
      return;

   if (reply.m_icmpHdr.m_cType == 0)
   {
      processEchoReply(InetAddress(ntohl(reply.m_ipHdr.m_iaSrc.s_addr)), reply.m_icmpHdr.m_wId, reply.m_icmpHdr.m_wSeq);
   }
   else if ((reply.m_icmpHdr.m_cType == 3) && (reply.m_icmpHdr.m_cCode == 1))    // code 1 is "host unreachable"
   {
      processHostUnreachable(InetAddress(ntohl(((IPHDR *)reply.m_data)->m_iaDst.s_addr)));
   }
}

/**
 * Receive IPv6 packet
 */
void PingRequestProcessor::receivePacketV6()
{
#ifdef WITH_IPV6
   socklen_t addrLen = sizeof(struct sockaddr_in6);
   struct sockaddr_in6 saSrc;
   char buffer[MAX_PING_SIZE];
   if (recvfrom(m_dataSocket, buffer, MAX_PING_SIZE, 0, (struct sockaddr *)&saSrc, &addrLen) <= 0)
      return;

   ICMP6_REPLY *reply = reinterpret_cast<ICMP6_REPLY*>(buffer);
   if (reply->type == 129) // ICMPv6 Echo Reply
   {
      processEchoReply(InetAddress(saSrc.sin6_addr.s6_addr), static_cast<uint16_t>(reply->id), static_cast<uint16_t>(reply->sequence));
   }
   else if ((reply->type == 1) || (reply->type == 3))    // 1 = Destination Unreachable, 3 = Time Exceeded
   {
      processHostUnreachable(InetAddress(reinterpret_cast<ICMP6_ERROR_REPORT*>(reply)->destAddr));
   }
#endif
}

/**
 * Set or clear "don't fragment" flag on socket
 */
static inline bool SetDontFragmentFlag(SOCKET s, bool enable)
{
#if HAVE_DECL_IP_MTU_DISCOVER
   int v = enable ? IP_PMTUDISC_DO : IP_PMTUDISC_DONT;
   return setsockopt(s, IPPROTO_IP, IP_MTU_DISCOVER, &v, sizeof(v)) == 0;
#elif HAVE_DECL_IP_DONTFRAG
   int v = enable ? 1 : 0;
   return setsockopt(s, IPPROTO_IP, IP_DONTFRAG, &v, sizeof(v)) == 0;
#else
   return false;
#endif
}

/**
 * Send ICMPv4 packet
 */
void PingRequestProcessor::sendRequestV4(PingRequest *request)
{
   static char payload[64] = "NetXMS ICMP probe [01234567890]";

   if (request->dontFragment && !SetDontFragmentFlag(m_dataSocket, true))
   {
      request->result = ICMP_SEND_FAILED;
      request->state = COMPLETED;
      return;
   }

   // Setup destination address structure
   SockAddrBuffer saDest;
   request->address.fillSockAddr(&saDest);

   // Fill in request structure
   ICMP_ECHO_REQUEST packet;
   packet.m_icmpHdr.m_cType = 8;   // ICMP ECHO REQUEST
   packet.m_icmpHdr.m_cCode = 0;
   packet.m_icmpHdr.m_wId = request->id;
   packet.m_icmpHdr.m_wSeq = request->sequence;
   memcpy(packet.m_data, payload, MIN(request->packetSize - sizeof(ICMPHDR) - sizeof(IPHDR), 64));

   // Do ping
   bool canRetry = true;
   int bytes = request->packetSize - sizeof(IPHDR);
   packet.m_icmpHdr.m_wChecksum = 0;
   packet.m_icmpHdr.m_wChecksum = CalculateIPChecksum(&packet, bytes);
retry:
   if (sendto(m_dataSocket, (char *)&packet, bytes, 0, (struct sockaddr *)&saDest.sa4, sizeof(struct sockaddr_in)) == bytes)
   {
      request->state = IN_PROGRESS;
   }
   else
   {
      if ((errno == ENOBUFS) && canRetry)
      {
         canRetry = false;
         SocketPoller sp(true);
         sp.add(m_dataSocket);
         if (sp.poll(150))
         {
            request->timestamp = GetCurrentTimeMs();
            goto retry;
         }
      }
      request->result = ICMP_SEND_FAILED;
      request->state = COMPLETED;
      if ((errno == EBADF) || (errno == ENOTSOCK))
      {
         close(m_dataSocket);
         m_dataSocket = INVALID_SOCKET;
      }
   }

   if (request->dontFragment && (m_dataSocket != INVALID_SOCKET))
      SetDontFragmentFlag(m_dataSocket, false);
}

#ifdef WITH_IPV6

/**
 * Find source address for given destination
 */
static bool FindSourceAddress(struct sockaddr_in6 *dest, struct sockaddr_in6 *src)
{
   int sd = socket(AF_INET6, SOCK_DGRAM, 0);
   if (sd < 0)
      return false;

   bool success = false;
   dest->sin6_port = htons(1025);
   if (connect(sd, (struct sockaddr *)dest, sizeof(struct sockaddr_in6)) != -1)
   {
      socklen_t len = sizeof(struct sockaddr_in6);
      if (getsockname(sd, (struct sockaddr *)src, &len) != -1)
      {
         src->sin6_port = 0;
         success = true;
      }
   }
   dest->sin6_port = 0;
   close(sd);
   return success;
}

/**
 * ICMPv6 checksum calculation
 */
static uint16_t CalculateICMPv6Checksum(const uint16_t *addr, size_t len)
{
   size_t count = len;
   uint32_t sum = 0;

   // Sum up 2-byte values until none or only one byte left
   while(count > 1)
   {
      sum += *(addr++);
      count -= 2;
   }

   // Add left-over byte, if any
   if (count > 0)
   {
      sum += *(BYTE *)addr;
   }

   // Fold 32-bit sum into 16 bits; we lose information by doing this,
   // increasing the chances of a collision.
   // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
   while(sum >> 16)
   {
      sum = (sum & 0xffff) + (sum >> 16);
   }

   // Checksum is one's compliment of sum.
   return (uint16_t)(~sum);
}

#endif   /* WITH_IPV6 */

/**
 * Send ICMPv6 packet
 */
void PingRequestProcessor::sendRequestV6(PingRequest *request)
{
#ifdef WITH_IPV6
   struct sockaddr_in6 src, dest;
   request->address.fillSockAddr((SockAddrBuffer *)&dest);
   if (!FindSourceAddress(&dest, &src))
   {
      request->state = COMPLETED;
      request->result = ICMP_UNREACHABLE;  // no route to host
      return;
   }

   if (request->dontFragment && !SetDontFragmentFlag(m_dataSocket, true))
   {
      request->state = COMPLETED;
      request->result = ICMP_SEND_FAILED;
      return;
   }

   // Prepare packet and calculate checksum
   static char payload[64] = "NetXMS ICMPv6 probe [01234567890]";
   size_t size = MAX(sizeof(ICMP6_PACKET_HEADER), MIN(request->packetSize, MAX_PING_SIZE));
   ICMP6_PACKET_HEADER *p = static_cast<ICMP6_PACKET_HEADER*>(MemAllocLocal(size));
   memset(p, 0, size);
   memcpy(p->srcAddr, src.sin6_addr.s6_addr, 16);
   memcpy(p->destAddr, dest.sin6_addr.s6_addr, 16);
   p->nextHeader = 58;
   p->type = 128;  // ICMPv6 Echo Request
   p->id = request->id;
   p->sequence = request->sequence;
   memcpy(p->data, payload, MIN(33, size - sizeof(ICMP6_PACKET_HEADER) + 8));
   p->checksum = 0;
   p->checksum = CalculateICMPv6Checksum(reinterpret_cast<uint16_t*>(p), size);

   // Send packet
   bool canRetry = true;
   int bytes = size - 40;  // excluding IPv6 header
retry:
   if (sendto(m_dataSocket, (char *)p + 40, bytes, 0, (struct sockaddr *)&dest, sizeof(struct sockaddr_in6)) == bytes)
   {
      request->state = IN_PROGRESS;
   }
   else
   {
      if ((errno == ENOBUFS) && canRetry)
      {
         canRetry = false;
         SocketPoller sp(true);
         sp.add(m_dataSocket);
         if (sp.poll(150))
         {
            request->timestamp = GetCurrentTimeMs();
            goto retry;
         }
      }
      request->result = ICMP_SEND_FAILED;
      request->state = COMPLETED;
      if ((errno == EBADF) || (errno == ENOTSOCK))
      {
         close(m_dataSocket);
         m_dataSocket = INVALID_SOCKET;
      }
   }

   MemFreeLocal(p);
   if (request->dontFragment && (m_dataSocket != INVALID_SOCKET))
      SetDontFragmentFlag(m_dataSocket, false);
#else
   request->state = COMPLETED;
   request->result = ICMP_API_ERROR;
#endif
}

/**
 * Do ping
 */
uint32_t PingRequestProcessor::ping(const InetAddress &addr, uint32_t timeout, uint32_t *rtt, uint32_t packetSize, bool dontFragment)
{
   PingRequest request;
   memset(&request, 0, sizeof(request));
   request.address = addr;
   request.packetSize = packetSize;
   request.dontFragment = dontFragment;
   request.timestamp = GetCurrentTimeMs();
#ifdef _USE_GNU_PTH
   pth_cond_init(&request.wakeupCondition);
#else
   pthread_cond_init(&request.wakeupCondition, nullptr);
#endif

#ifdef _USE_GNU_PTH
   pth_mutex_acquire(&m_mutex, FALSE, nullptr);
#else
   pthread_mutex_lock(&m_mutex);
#endif
   if (!m_shutdown)
   {
      if (m_dataSocket == INVALID_SOCKET)
      {
         if (!openSocket())
         {
            request.result = ICMP_RAW_SOCK_FAILED;
         }
      }
      if (m_processingThread == INVALID_THREAD_HANDLE)
      {
         if (pipe(m_controlSockets) == 0)
         {
            m_processingThread = ThreadCreateEx(this, &PingRequestProcessor::processingThread);
         }
         else
         {
            request.result = ICMP_API_ERROR;
         }
      }

      if (request.result == ICMP_SUCCESS) // Continue only if request processor is ready
      {
         request.id = m_id;
         request.sequence = m_sequence++;
         if (sendRequest(&request))
         {
            // Only add request to list if request packet was sent successfully
            request.next = m_head->next;
            m_head->next = &request;

#ifdef _USE_GNU_PTH
            pth_event_t ev = pth_event(PTH_EVENT_TIME, pth_timeout(timeout / 1000, (timeout % 1000) * 1000));
            int waitResult = pth_cond_await(&request.wakeupCondition, &m_mutex, ev);
            if (waitResult > 0)
            {
               if (pth_event_status(ev) != PTH_STATUS_OCCURRED)
                  waitResult = 0; // Success, condition signalled
            }
            else
            {
               waitResult = -1; // Failure
            }
            pth_event_free(ev, PTH_FREE_ALL);
#else /* not _USE_GNU_PTH */
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
            struct timespec ts;
            ts.tv_sec = timeout / 1000;
            ts.tv_nsec = (timeout % 1000) * 1000000;
            int waitResult = pthread_cond_reltimedwait_np(&request.wakeupCondition, &m_mutex, &ts);
#else
            struct timeval now;
            gettimeofday(&now, nullptr);
            now.tv_usec += (timeout % 1000) * 1000;

            struct timespec ts;
            ts.tv_sec = now.tv_sec + (timeout / 1000) + now.tv_usec / 1000000;
            ts.tv_nsec = (now.tv_usec % 1000000) * 1000;
            int waitResult = pthread_cond_timedwait(&request.wakeupCondition, &m_mutex, &ts);
#endif
#endif

            // Check request state in addition to timeout for case when response was received and processed
            // after pthread_cond_timedwait timeouts but before mutex was acquired by waiting thread
            if ((waitResult != 0) && (request.state == IN_PROGRESS))
            {
               request.result = ICMP_TIMEOUT;
               m_id = InterlockedIncrement(&s_nextProcessorId); // Change ID in case of timeout - we've seen cases when firewall start blocking ICMP requests with same ID
            }

            // Remove request from list
            for(PingRequest *p = m_head; p->next != nullptr; p = p->next)
            {
               if (p->next == &request)
               {
                  p->next = request.next;
                  break;
               }
            }
         }
      }
   }
   else
   {
      request.result = ICMP_API_ERROR;
   }
#ifdef _USE_GNU_PTH
   pth_mutex_release(&m_mutex);
#else
   pthread_mutex_unlock(&m_mutex);
#endif

#ifndef _USE_GNU_PTH
   pthread_cond_destroy(&request.wakeupCondition);
#endif

   if (rtt != nullptr)
      *rtt = request.rtt;
   return request.result;
}

/**
 * Request processor instances
 */
static ObjectArray<PingRequestProcessor> s_processors(8, 8, Ownership::True);
static Mutex s_processorListLock(MutexType::FAST);

/**
 * Get available request processor for given address family
 */
static PingRequestProcessor *GetRequestProcessor(int af)
{
   LockGuard lockGuard(s_processorListLock);

   for(int i = 0; i < s_processors.size(); i++)
   {
      PingRequestProcessor *p = s_processors.get(i);
      if ((p->getFamily() == af) && p->acquire())
         return p;
   }

   auto p = new PingRequestProcessor(af);
   s_processors.add(p);
   return p;
}

/**
 * Do an ICMP ping to specific IP address
 * Return value: ICMP error code
 * Parameters: addr - IP address
 *             numRetries - number of retries
 *             timeout - Timeout waiting for response in milliseconds
 *             rtt - pointer to save round trip time
 *             packetSize - ping packet size in bytes
 *             dontFragment - if true "don't fragment" flag will be set on outgoing packet
 */
uint32_t LIBNETXMS_EXPORTABLE IcmpPing(const InetAddress &addr, int numRetries, uint32_t timeout, uint32_t *rtt, uint32_t packetSize, bool dontFragment)
{
   if (addr.getFamily() == AF_UNSPEC)
      return ICMP_API_ERROR;

   if (packetSize < MIN_PING_SIZE)
      packetSize = MIN_PING_SIZE;
   else if (packetSize > MAX_PING_SIZE)
      packetSize = MAX_PING_SIZE;

   PingRequestProcessor *p = GetRequestProcessor(addr.getFamily());

   uint32_t result;
   while(numRetries-- > 0)
   {
      result = p->ping(addr, timeout, rtt, packetSize, dontFragment);
      if (result != ICMP_TIMEOUT)
         break;
   }
   p->release();
   return result;
}

#endif   /* _WIN32 */
