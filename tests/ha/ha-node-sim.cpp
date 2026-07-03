/*
** ha-node-sim - netxmsd HA lease algorithm test driver
** Copyright (C) 2026 Raden Solutions
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
** A fake netxmsd for the HA adversarial test harness (tests/ha/harness.py).
** Runs the real HALeaseManager (compiled from src/server/core/halease.cpp)
** against a shared database, performs continuous role-stamped writes into a
** scratch table while active, keeps a dumb TCP heartbeat with its peer (to
** prove the promotion rule ignores channel state), and exposes fault
** injection commands on stdin.
**
** stdout protocol (one record per line):
**   STATUS role=<r> term=<t> holder=<guid> remaining=<sec> peer=<up|down>
**   EVENT promoted term=<t>
**   EVENT fenced reason=<text>
**   EVENT released
**   ALLOC id=<id> term=<t>
**   ALLOC-COLLISION id=<id>
**   LONGTXN committed term=<t> seq=<n>
**
** stdin commands:
**   release        - graceful lease release (after which the process exits 0)
**   alloc <n>      - allocate <n> IDs from the in-memory allocator and insert them
**   longtxn <ms>   - open transaction, insert a write, hold it <ms>, commit
**   quit           - exit 0
**
** Exit codes: 0 = normal exit, 42 = fenced, 1 = startup error
*/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxdbapi.h>
#include <netxms-version.h>
#include <halease.h>
#include <netxmsdb.h>
#include <signal.h>

NETXMS_EXECUTABLE_HEADER(ha-node-sim)

/**
 * Configuration
 */
static wchar_t s_dbDriver[64] = L"pgsql.ddr";
static wchar_t s_dbServer[256] = L"localhost";
static wchar_t s_dbName[256] = L"netxms";
static wchar_t s_dbLogin[256] = L"netxms";
static wchar_t s_dbPassword[256] = L"";
static wchar_t s_nodeName[64] = L"sim";
static uuid s_nodeGuid;
static uint32_t s_refreshInterval = 5;
static uint32_t s_validity = 20;
static uint32_t s_fenceMargin = 3;
static uint32_t s_writeInterval = 200;   // milliseconds
static uint16_t s_listenPort = 0;        // heartbeat listener; 0 = disabled
static wchar_t s_peerAddress[256] = L"";
static uint16_t s_peerPort = 0;
static bool s_rebaseOnPromotion = false;

/**
 * State
 */
static DB_DRIVER s_driver = nullptr;
static HALeaseManager *s_manager = nullptr;
static std::atomic<bool> s_shutdown(false);
static VolatileCounter64 s_writeSeq = 0;

static Mutex s_controlDbLock(MutexType::FAST);
static DB_HANDLE s_controlDb = nullptr;
static int64_t s_nextAllocId = 0;

static Mutex s_peerLock(MutexType::FAST);
static int64_t s_lastPeerContact = 0;    // monotonic ms
static char s_peerRole[16] = "";

/**
 * Print one output record (line buffered, harness-parseable)
 */
static void Output(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);
   putchar('\n');
   fflush(stdout);
}

/**
 * Establish a database connection
 */
static DB_HANDLE ConnectDB()
{
   wchar_t errorText[DBDRV_MAX_ERROR_TEXT];
   DB_HANDLE hdb = DBConnect(s_driver, s_dbServer, s_dbName, s_dbLogin, s_dbPassword, nullptr, errorText);
   if (hdb == nullptr)
      nxlog_debug_tag(L"sim", 3, L"Database connection failed (%s)", errorText);
   return hdb;
}

/**
 * Epoch-seconds-now SQL expression for scratch table writes (harness runs on
 * PostgreSQL; MySQL supported for convenience)
 */
static const wchar_t *SimDbTimeExpression(DB_HANDLE hdb)
{
   switch(DBGetSyntax(hdb))
   {
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         return L"CAST(EXTRACT(EPOCH FROM now()) AS bigint)";
      case DB_SYNTAX_MYSQL:
         return L"UNIX_TIMESTAMP()";
      default:
         return L"0";
   }
}

/**
 * Create scratch tables (ignore failures - they likely exist already)
 */
static void CreateScratchTables(DB_HANDLE hdb)
{
   DBQuery(hdb,
      L"CREATE TABLE ha_sim_writes ("
      L"  node_guid varchar(36) not null,"
      L"  incarnation bigint not null,"
      L"  seq bigint not null,"
      L"  term bigint not null,"
      L"  db_time bigint not null,"
      L"  mono_time bigint not null,"
      L"  PRIMARY KEY(node_guid,incarnation,seq))");
   DBQuery(hdb,
      L"CREATE TABLE ha_sim_alloc ("
      L"  alloc_id bigint not null,"
      L"  node_guid varchar(36) not null,"
      L"  term bigint not null,"
      L"  PRIMARY KEY(alloc_id))");
}

/**
 * Seed in-memory ID allocator from database maximum (mimics id.cpp InitIdTable)
 */
static void SeedAllocator(DB_HANDLE hdb)
{
   DB_RESULT hResult = DBSelect(hdb, L"SELECT coalesce(max(alloc_id),0) FROM ha_sim_alloc");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_nextAllocId = DBGetFieldInt64(hResult, 0, 0) + 1;
      DBFreeResult(hResult);
   }
   nxlog_debug_tag(L"sim", 3, L"ID allocator seeded at " INT64_FMTW, s_nextAllocId);
}

/**
 * Insert one role-stamped write. Returns false on SQL failure.
 */
static bool InsertWrite(DB_HANDLE hdb, DB_STATEMENT hStmt, int64_t term)
{
   wchar_t guidText[64];
   s_nodeGuid.toString(guidText);
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guidText, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, static_cast<int64_t>(s_manager->getIncarnation()));
   DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, InterlockedIncrement64(&s_writeSeq));
   DBBind(hStmt, 4, DB_SQLTYPE_BIGINT, term);
   DBBind(hStmt, 5, DB_SQLTYPE_BIGINT, GetMonotonicClockTime());
   return DBExecute(hStmt);
}

/**
 * Role-sensitive writer thread: one write every s_writeInterval ms while the
 * node holds the lease. The isActive() call is the "role-sensitive dispatch
 * point" from the design - it runs the opportunistic fence deadline check, so
 * a process resuming from a pause longer than the lease fences here before
 * dispatching any new write.
 */
static void WriterThread()
{
   DB_HANDLE hdb = nullptr;
   DB_STATEMENT hStmt = nullptr;

   while(!s_shutdown.load())
   {
      ThreadSleepMs(s_writeInterval);

      if (!s_manager->isActive())
         continue;

      if (hdb == nullptr)
      {
         hdb = ConnectDB();
         if (hdb == nullptr)
            continue;
         StringBuffer query(L"INSERT INTO ha_sim_writes (node_guid,incarnation,seq,term,mono_time,db_time) VALUES (?,?,?,?,?,(");
         query.append(SimDbTimeExpression(hdb));
         query.append(L"))");
         hStmt = DBPrepare(hdb, query);
         if (hStmt == nullptr)
         {
            DBDisconnect(hdb);
            hdb = nullptr;
            continue;
         }
      }

      if (!InsertWrite(hdb, hStmt, s_manager->getTerm()))
      {
         DBFreeStatement(hStmt);
         DBDisconnect(hdb);
         hdb = nullptr;
         hStmt = nullptr;
      }
   }

   if (hStmt != nullptr)
      DBFreeStatement(hStmt);
   if (hdb != nullptr)
      DBDisconnect(hdb);
}

/**
 * Heartbeat listener thread: reads peer state lines from incoming connection
 */
static void HeartbeatListenerThread()
{
   SOCKET listenSocket = CreateSocket(AF_INET, SOCK_STREAM, 0);
   if (listenSocket == INVALID_SOCKET)
      return;
   SetSocketReuseFlag(listenSocket);

   struct sockaddr_in servAddr;
   memset(&servAddr, 0, sizeof(servAddr));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servAddr.sin_port = htons(s_listenPort);
   if ((bind(listenSocket, (struct sockaddr *)&servAddr, sizeof(servAddr)) != 0) || (listen(listenSocket, 2) != 0))
   {
      closesocket(listenSocket);
      nxlog_debug_tag(L"sim", 1, L"Cannot bind heartbeat listener to port %u", s_listenPort);
      return;
   }

   while(!s_shutdown.load())
   {
      SocketPoller poller;
      poller.add(listenSocket);
      if (poller.poll(1000) <= 0)
         continue;

      SOCKET client = accept(listenSocket, nullptr, nullptr);
      if (client == INVALID_SOCKET)
         continue;

      char buffer[256];
      while(!s_shutdown.load())
      {
         SocketPoller clientPoller;
         clientPoller.add(client);
         if (clientPoller.poll(1000) <= 0)
         {
            if (s_shutdown.load())
               break;
            continue;
         }
         ssize_t bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
         if (bytes <= 0)
            break;
         buffer[bytes] = 0;

         // Line format: "HB <role> <term>\n" (possibly several per read)
         char *role = strstr(buffer, "HB ");
         if (role != nullptr)
         {
            role += 3;
            char *end = strchr(role, ' ');
            if (end != nullptr)
            {
               *end = 0;
               LockGuard lockGuard(s_peerLock);
               s_lastPeerContact = GetMonotonicClockTime();
               strlcpy(s_peerRole, role, sizeof(s_peerRole));
            }
         }
      }
      closesocket(client);
   }
   closesocket(listenSocket);
}

/**
 * Heartbeat sender thread: pushes this node's state to the peer
 */
static void HeartbeatSenderThread()
{
   InetAddress peerAddr = InetAddress::resolveHostName(s_peerAddress);
   SOCKET s = INVALID_SOCKET;

   while(!s_shutdown.load())
   {
      ThreadSleepMs(500);

      if (s == INVALID_SOCKET)
      {
         s = ConnectToHost(peerAddr, s_peerPort, 500);
         if (s == INVALID_SOCKET)
            continue;
      }

      const char *role;
      switch(s_manager->getState())
      {
         case HALeaseState::ACTIVE:
            role = "ACTIVE";
            break;
         case HALeaseState::STANDBY:
            role = "STANDBY";
            break;
         case HALeaseState::FENCED:
            role = "FENCED";
            break;
         default:
            role = "OTHER";
            break;
      }
      char message[64];
      snprintf(message, sizeof(message), "HB %s " INT64_FMTA "\n", role, s_manager->getTerm());
      if (send(s, message, strlen(message), 0) <= 0)
      {
         closesocket(s);
         s = INVALID_SOCKET;
      }
   }
   if (s != INVALID_SOCKET)
      closesocket(s);
}

/**
 * Status reporter thread: one STATUS line per second
 */
static void StatusThread()
{
   const wchar_t *stateNames[] = { L"INIT", L"STANDBY", L"ACTIVE", L"FENCED", L"STOPPED" };
   while(!s_shutdown.load())
   {
      ThreadSleepMs(1000);

      HALeaseStatus status = s_manager->getStatus();
      bool peerUp;
      char peerRole[16];
      {
         LockGuard lockGuard(s_peerLock);
         peerUp = (s_lastPeerContact != 0) && (GetMonotonicClockTime() - s_lastPeerContact < 2000);
         strlcpy(peerRole, s_peerRole, sizeof(peerRole));
      }
      wchar_t holderText[64];
      status.holderGuid.toString(holderText);
      Output("STATUS role=%ls term=" INT64_FMTA " holder=%ls remaining=" INT64_FMTA " peer=%s peer_role=%s",
            stateNames[static_cast<int>(status.state)], status.term, holderText, status.remainingValidity,
            peerUp ? "up" : "down", peerUp ? peerRole : "-");
   }
}

/**
 * Long transaction thread: BEGIN, one write, hold, COMMIT (scenario: long
 * transaction crossing a promotion)
 */
static void LongTransactionThread(uint32_t holdTime)
{
   DB_HANDLE hdb = ConnectDB();
   if (hdb == nullptr)
   {
      Output("LONGTXN error=connect");
      return;
   }

   int64_t term = s_manager->getTerm();
   StringBuffer query(L"INSERT INTO ha_sim_writes (node_guid,incarnation,seq,term,mono_time,db_time) VALUES (?,?,?,?,?,(");
   query.append(SimDbTimeExpression(hdb));
   query.append(L"))");

   DBBegin(hdb);
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   bool success = false;
   int64_t seq = 0;
   if (hStmt != nullptr)
   {
      seq = InterlockedIncrement64(&s_writeSeq);
      wchar_t guidText[64];
      s_nodeGuid.toString(guidText);
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guidText, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, static_cast<int64_t>(s_manager->getIncarnation()));
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, seq);
      DBBind(hStmt, 4, DB_SQLTYPE_BIGINT, term);
      DBBind(hStmt, 5, DB_SQLTYPE_BIGINT, GetMonotonicClockTime());
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   ThreadSleepMs(holdTime);

   if (success)
   {
      DBCommit(hdb);
      Output("LONGTXN committed term=" INT64_FMTA " seq=" INT64_FMTA, term, seq);
   }
   else
   {
      DBRollback(hdb);
      Output("LONGTXN error=insert");
   }
   DBDisconnect(hdb);
}

/**
 * Allocate IDs from the in-memory allocator (mimics stale-allocator hazard)
 */
static void AllocateIds(int count)
{
   LockGuard lockGuard(s_controlDbLock);
   if (s_controlDb == nullptr)
      return;

   DB_STATEMENT hStmt = DBPrepare(s_controlDb, L"INSERT INTO ha_sim_alloc (alloc_id,node_guid,term) VALUES (?,?,?)");
   if (hStmt == nullptr)
      return;

   wchar_t guidText[64];
   s_nodeGuid.toString(guidText);
   for(int i = 0; i < count; i++)
   {
      int64_t id = s_nextAllocId++;
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, id);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, guidText, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, s_manager->getTerm());
      if (DBExecute(hStmt))
         Output("ALLOC id=" INT64_FMTA " term=" INT64_FMTA, id, s_manager->getTerm());
      else
         Output("ALLOC-COLLISION id=" INT64_FMTA, id);
   }
   DBFreeStatement(hStmt);
}

/**
 * Signal handler
 */
static void SignalHandler(int sig)
{
   s_shutdown = true;
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   int debugLevel = 4;
   int ch;
   while((ch = getopt(argc, argv, "d:D:g:hl:m:n:N:p:P:r:Rs:U:v:w:x:")) != -1)
   {
      switch(ch)
      {
         case 'd':
            mb_to_wchar(optarg, -1, s_dbDriver, 64);
            break;
         case 'D':
            mb_to_wchar(optarg, -1, s_dbName, 256);
            break;
         case 'g':
         {
            wchar_t guidText[64];
            mb_to_wchar(optarg, -1, guidText, 64);
            s_nodeGuid = uuid::parse(guidText);
            break;
         }
         case 'l':
            s_listenPort = static_cast<uint16_t>(strtoul(optarg, nullptr, 10));
            break;
         case 'm':
            s_fenceMargin = strtoul(optarg, nullptr, 10);
            break;
         case 'N':
            mb_to_wchar(optarg, -1, s_nodeName, 64);
            break;
         case 'p':
         {
            char *sep = strchr(optarg, ':');
            if (sep != nullptr)
            {
               *sep = 0;
               s_peerPort = static_cast<uint16_t>(strtoul(sep + 1, nullptr, 10));
            }
            mb_to_wchar(optarg, -1, s_peerAddress, 256);
            break;
         }
         case 'P':
            mb_to_wchar(optarg, -1, s_dbPassword, 256);
            break;
         case 'r':
            s_refreshInterval = strtoul(optarg, nullptr, 10);
            break;
         case 'R':
            s_rebaseOnPromotion = true;
            break;
         case 's':
            mb_to_wchar(optarg, -1, s_dbServer, 256);
            break;
         case 'U':
            mb_to_wchar(optarg, -1, s_dbLogin, 256);
            break;
         case 'v':
            s_validity = strtoul(optarg, nullptr, 10);
            break;
         case 'w':
            s_writeInterval = strtoul(optarg, nullptr, 10);
            break;
         case 'x':
            debugLevel = atoi(optarg);
            break;
         case 'h':
         default:
            printf("Usage: ha-node-sim [options]\n"
                   "  -d <driver>    database driver (default pgsql.ddr)\n"
                   "  -s <server>    database server (default localhost)\n"
                   "  -D <dbname>    database name (default netxms)\n"
                   "  -U <login>     database login (default netxms)\n"
                   "  -P <password>  database password\n"
                   "  -N <name>      node name (default sim)\n"
                   "  -g <guid>      node GUID (default: generate)\n"
                   "  -r <seconds>   lease refresh interval (default 5)\n"
                   "  -v <seconds>   lease validity (default 20)\n"
                   "  -m <seconds>   fence margin (default 3)\n"
                   "  -w <ms>        role-stamped write interval (default 200)\n"
                   "  -l <port>      heartbeat listen port (0 = disabled)\n"
                   "  -p <host:port> peer heartbeat address\n"
                   "  -R             rebase ID allocator on promotion\n"
                   "  -x <level>     debug level (default 4)\n");
            return (ch == 'h') ? 0 : 1;
      }
   }

   if (s_nodeGuid.isNull())
      s_nodeGuid = uuid::generate();

   nxlog_open(L"ha-node-sim", NXLOG_USE_STDOUT);
   nxlog_set_debug_level(debugLevel);

   if (!DBInit())
   {
      Output("ERROR db-init");
      return 1;
   }
   s_driver = DBLoadDriver(s_dbDriver, L"", nullptr, nullptr);
   if (s_driver == nullptr)
   {
      Output("ERROR driver-load");
      return 1;
   }

   s_controlDb = ConnectDB();
   if (s_controlDb == nullptr)
   {
      Output("ERROR db-connect");
      return 1;
   }
   CreateScratchTables(s_controlDb);
   SeedAllocator(s_controlDb);

   signal(SIGINT, SignalHandler);
   signal(SIGTERM, SignalHandler);
#ifndef _WIN32
   signal(SIGPIPE, SIG_IGN);
#endif

   s_manager = new HALeaseManager(s_nodeGuid, s_nodeName, ConnectDB, s_refreshInterval, s_validity, s_fenceMargin);
   s_manager->setPromotionHandler(
      [] (int64_t term) -> void
      {
         if (s_rebaseOnPromotion)
         {
            LockGuard lockGuard(s_controlDbLock);
            SeedAllocator(s_controlDb);
         }
         Output("EVENT promoted term=" INT64_FMTA, term);
      });
   s_manager->setFenceHandler(
      [] (const wchar_t *reason) -> void
      {
         Output("EVENT fenced reason=%ls", reason);
         fflush(stdout);
         _exit(42);
      });
   s_manager->start();

   THREAD writerThread = ThreadCreateEx(WriterThread);
   THREAD statusThread = ThreadCreateEx(StatusThread);
   THREAD hbListenerThread = (s_listenPort != 0) ? ThreadCreateEx(HeartbeatListenerThread) : INVALID_THREAD_HANDLE;
   THREAD hbSenderThread = (s_peerPort != 0) ? ThreadCreateEx(HeartbeatSenderThread) : INVALID_THREAD_HANDLE;

   wchar_t guidText[64];
   Output("READY node=%ls guid=%ls incarnation=" UINT64_FMTA, s_nodeName, s_nodeGuid.toString(guidText), s_manager->getIncarnation());

   // Command loop
   char command[256];
   while(!s_shutdown.load() && (fgets(command, sizeof(command), stdin) != nullptr))
   {
      char *nl = strchr(command, '\n');
      if (nl != nullptr)
         *nl = 0;

      if (!strcmp(command, "quit"))
      {
         s_shutdown = true;
      }
      else if (!strcmp(command, "release"))
      {
         s_manager->requestRelease();
         while((s_manager->getState() == HALeaseState::ACTIVE) && !s_shutdown.load())
            ThreadSleepMs(100);
         Output("EVENT released");
         s_shutdown = true;
      }
      else if (!strncmp(command, "alloc ", 6))
      {
         AllocateIds(atoi(command + 6));
      }
      else if (!strncmp(command, "longtxn ", 8))
      {
         uint32_t holdTime = strtoul(command + 8, nullptr, 10);
         ThreadCreateEx(LongTransactionThread, holdTime);
      }
      else if (command[0] != 0)
      {
         Output("ERROR unknown-command %s", command);
      }
   }
   s_shutdown = true;

   ThreadJoin(writerThread);
   ThreadJoin(statusThread);
   ThreadJoin(hbListenerThread);
   ThreadJoin(hbSenderThread);
   s_manager->stop();
   delete s_manager;
   DBDisconnect(s_controlDb);
   DBUnloadDriver(s_driver);
   return 0;
}
