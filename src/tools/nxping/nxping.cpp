/* 
** nxping - command line tool for ICMP ping
** Copyright (C) 2024 Victor Kirhenshtein
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
** File: nxping.cpp
**
**/

#include <nms_util.h>
#include <netxms_getopt.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxping)

/**
 * Target
 */
struct Target
{
   InetAddress address;
   THREAD thread;
   uint32_t averageRTT;
   uint32_t packetLoss;
   uint32_t sendErrors;

   Target(const InetAddress& a) : address(a)
   {
      thread = INVALID_THREAD_HANDLE;
      averageRTT = 0;
      packetLoss = 0;
      sendErrors = 0;
   }

   Target(uint32_t a) : address(a)
   {
      thread = INVALID_THREAD_HANDLE;
      averageRTT = 0;
      packetLoss = 0;
      sendErrors = 0;
   }
};

/**
 * Ping interval
 */
static uint32_t s_interval = 1000;

/**
 * Iterations
 */
static int s_iterations = 10;

/**
 * Timeout
 */
static uint32_t s_timeout = 500;

/**
 * Packet size
 */
static uint32_t s_packetSize = 48;

/**
 * Do not fragment flag
 */
static bool s_dontFragment = false;

/**
 * Ping worker thread
 */
static void WorkerThread(Target *target)
{
   TCHAR addrText[64];
   target->address.toString(addrText);

   uint32_t lostPackets = 0;
   uint32_t count = 0;
   for(int i = 1; i <= s_iterations; i++)
   {
      uint32_t rtt;
      uint32_t rc = IcmpPing(target->address, 1, s_timeout, &rtt, s_packetSize, s_dontFragment);
      if (rc == ICMP_SUCCESS)
      {
         _tprintf(_T("%s: sequence=%d time=%u ms\n"), addrText, i, rtt);
         target->averageRTT = (static_cast<uint64_t>(target->averageRTT) * static_cast<uint64_t>(count) + rtt) / (count + 1);
         count++;
      }
      else
      {
         switch(rc)
         {
            case ICMP_TIMEOUT:
               _tprintf(_T("%s: sequence=%d timeout\n"), addrText, i);
               count++;
               lostPackets++;
               break;
            case ICMP_UNREACHABLE:
               _tprintf(_T("%s: sequence=%d unreachable\n"), addrText, i);
               count++;
               lostPackets++;
               break;
            case ICMP_SEND_FAILED:
               target->sendErrors++;
               break;
            default:
               _tprintf(_T("%s: sequence=%d error %u\n"), addrText, i, rc);
               break;
         }
      }
      ThreadSleepMs(s_interval);
   }

   if (count > 0)
      target->packetLoss = lostPackets * 100 / count;
}

/**
 * main
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   // Parse command line
   bool showOnlyPartialLoss = false;
   opterr = 1;
   int ch;
   while((ch = getopt(argc, argv, "c:hi:nps:t:v")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxping [<options>] <address> ...\n"
                   "Valid options are:\n"
                   "   -h           : Display help and exit.\n"
                   "   -c count     : Set number of packets to be sent (default is 10).\n"
                   "   -i interval  : Send packets with given interval in milliseconds (default is one second).\n"
                   "   -n           : Set \"don't fragment\" flag.\n"
                   "   -p           : Show only targets with partial packet loss in summary.\n"
                   "   -s size      : Set packet size (default is 48).\n"
                   "   -t timeout   : Set timeout in milliseconds (default is 500).\n"
                   "   -v           : Display version and exit.\n"
                   "\n");
            return 0;
         case 'c':
            s_iterations = strtol(optarg, nullptr, 0);
            break;
         case 'i':
            s_interval = strtoul(optarg, nullptr, 0);
            break;
         case 'n':
            s_dontFragment = true;
            break;
         case 'p':
            showOnlyPartialLoss = true;
            break;
         case 's':
            s_packetSize = strtoul(optarg, nullptr, 0);
            break;
         case 't':
            s_timeout = strtoul(optarg, nullptr, 0);
            break;
         case 'v':   // Print version and exit
            printf("NetXMS ICMP Ping Tool Version " NETXMS_VERSION_STRING_A "\n");
            return 0;
         case '?':
            return 1;
         default:
            break;
      }
   }

   if (argc - optind == 0)
   {
      printf("Missing target address\n");
      return 1;
   }

   std::vector<Target> targets;
   for(int i = optind; i < argc; i++)
   {
      char *s = strchr(argv[i], '-');
      if (s != nullptr)
      {
         *s = 0;
         s++;
         InetAddress addr1 = InetAddress::parse(argv[i]);
         InetAddress addr2 = InetAddress::parse(s);
         if (addr1.isValid() && addr2.isValid() && (addr1.getFamily() == addr2.getFamily()) && (addr1.getFamily() == AF_INET) && (addr1.getAddressV4() < addr2.getAddressV4()))
         {
            for(uint32_t a = addr1.getAddressV4(); a <= addr2.getAddressV4(); a++)
               targets.emplace_back(a);
         }
         else
         {
            _tprintf(_T("Invalid address range %hs\n"), argv[i]);
         }
      }
      else
      {
         InetAddress addr = InetAddress::parse(argv[i]);
         if (addr.isValid())
         {
            targets.emplace_back(addr);
         }
         else
         {
            _tprintf(_T("Invalid address %hs\n"), argv[i]);
         }
      }
   }

   for(int i = 0; i < targets.size(); i++)
   {
      targets[i].thread = ThreadCreateEx(WorkerThread, &targets[i]);
      ThreadSleepMs(10);
   }

   for(int i = 0; i < targets.size(); i++)
   {
      ThreadJoin(targets[i].thread);
   }

   _tprintf(_T("\n==== %d iterations completed ====\n\n"), s_iterations);
   for(int i = 0; i < targets.size(); i++)
   {
      const Target& t = targets[i];
      if (!showOnlyPartialLoss || ((t.packetLoss > 0) && (t.packetLoss < 100)) || (t.sendErrors > 0))
         _tprintf(_T("%s: %d ms average response time, %d%% packet loss, %d send errors\n"), t.address.toString().cstr(), t.averageRTT, t.packetLoss, t.sendErrors);
   }

   return 0;
}
