/*
** NetXMS subagent for ISC Kea DHCP server
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
** File: kea.cpp
**
** Provides generic DHCP.* scope and lease information from ISC Kea DHCPv4/DHCPv6
** servers via their UNIX domain control sockets. Lease information requires the
** lease_cmds hook library to be loaded by the Kea server.
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxcldefs.h>
#include <netxms-version.h>
#include <sys/stat.h>
#include <sys/un.h>

#define DEBUG_TAG _T("kea")

/**
 * Kea command result codes
 */
#define KEA_RESULT_SUCCESS       0
#define KEA_RESULT_ERROR         1
#define KEA_RESULT_UNSUPPORTED   2
#define KEA_RESULT_EMPTY         3

/**
 * Kea lease states
 */
#define KEA_LEASE_STATE_DECLINED          1
#define KEA_LEASE_STATE_EXPIRED_RECLAIMED 2

/**
 * Default control socket locations (first existing one is used)
 */
static const char *s_defaultSocketsV4[] = { "/run/kea/kea4-ctrl-socket", "/var/run/kea/kea4-ctrl-socket", "/tmp/kea4-ctrl-socket", nullptr };
static const char *s_defaultSocketsV6[] = { "/run/kea/kea6-ctrl-socket", "/var/run/kea/kea6-ctrl-socket", "/tmp/kea6-ctrl-socket", nullptr };

/**
 * Configuration
 */
static char s_socketV4[MAX_PATH] = "";
static char s_socketV6[MAX_PATH] = "";
static bool s_autoSocketV4 = true;
static bool s_autoSocketV6 = true;
static uint32_t s_timeout = 5000;
static uint32_t s_cacheTime = 5;

/**
 * Host reservation
 */
struct KeaReservation
{
   String ipAddress;
   String macAddress;
   String clientId;
   String hostname;
   int prefixLength;

   KeaReservation(const String& ip, const String& mac, const String& cid, const String& host, int prefixLen) :
         ipAddress(ip), macAddress(mac), clientId(cid), hostname(host)
   {
      prefixLength = prefixLen;
   }
};

/**
 * Subnet (scope) information
 */
struct KeaScope
{
   int ipVersion;
   uint32_t subnetId;
   String network;
   String name;
   String ranges;
   uint64_t total;
   uint64_t used;
   uint64_t declined;
   ObjectArray<KeaReservation> reservations;

   KeaScope(int version, uint32_t id, const String& net, const String& n, const String& r) :
         network(net), name(n), ranges(r), reservations(0, 16, Ownership::True)
   {
      ipVersion = version;
      subnetId = id;
      total = 0;
      used = 0;
      declined = 0;
   }

   uint64_t free() const
   {
      return DHCPScopeFreeAddresses(total, used, declined);
   }

   double utilization() const
   {
      return DHCPScopeUtilization(total, used);
   }
};

/**
 * Cached scope list
 */
static shared_ptr<ObjectArray<KeaScope>> s_scopes;
static time_t s_scopesTimestamp = 0;
static Mutex s_scopesLock;

/**
 * Get control socket path for given IP version. Returns nullptr if socket is not available.
 */
static const char *GetSocketPath(int ipVersion)
{
   bool autoDetect = (ipVersion == 4) ? s_autoSocketV4 : s_autoSocketV6;
   if (!autoDetect)
   {
      const char *path = (ipVersion == 4) ? s_socketV4 : s_socketV6;
      return (*path != 0) ? path : nullptr;
   }

   for(const char **p = (ipVersion == 4) ? s_defaultSocketsV4 : s_defaultSocketsV6; *p != nullptr; p++)
   {
      struct stat st;
      if ((stat(*p, &st) == 0) && S_ISSOCK(st.st_mode))
         return *p;
   }
   return nullptr;
}

/**
 * Check if accumulated response forms complete JSON document. Tracks nesting
 * depth incrementally starting from given offset.
 */
static bool IsCompleteDocument(const char *data, size_t size, size_t *offset, int *depth, bool *inString, bool *escape, bool *started)
{
   for(size_t i = *offset; i < size; i++)
   {
      char c = data[i];
      if (*inString)
      {
         if (*escape)
            *escape = false;
         else if (c == '\\')
            *escape = true;
         else if (c == '"')
            *inString = false;
         continue;
      }
      switch(c)
      {
         case '"':
            *inString = true;
            break;
         case '{':
         case '[':
            (*depth)++;
            *started = true;
            break;
         case '}':
         case ']':
            (*depth)--;
            if (*started && (*depth == 0))
            {
               *offset = i + 1;
               return true;
            }
            break;
      }
   }
   *offset = size;
   return false;
}

/**
 * Execute command on Kea server. Returns command response object (caller must decref) or nullptr on communication failure.
 */
static json_t *ExecuteKeaCommand(const char *socketPath, const char *command, json_t *arguments)
{
   json_t *request = json_object();
   json_object_set_new(request, "command", json_string(command));
   if (arguments != nullptr)
      json_object_set_new(request, "arguments", arguments);
   char *requestText = json_dumps(request, JSON_COMPACT);
   json_decref(request);

   SOCKET s = socket(AF_UNIX, SOCK_STREAM, 0);
   if (s == INVALID_SOCKET)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ExecuteKeaCommand(%hs): cannot create socket (%s)"), command, _tcserror(errno));
      MemFree(requestText);
      return nullptr;
   }

   struct sockaddr_un addr;
   memset(&addr, 0, sizeof(addr));
   addr.sun_family = AF_UNIX;
   strlcpy(addr.sun_path, socketPath, sizeof(addr.sun_path));
   if (connect(s, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteKeaCommand(%hs): cannot connect to control socket %hs (%s)"), command, socketPath, _tcserror(errno));
      closesocket(s);
      MemFree(requestText);
      return nullptr;
   }

   size_t requestLen = strlen(requestText);
   bool sent = (SendEx(s, requestText, requestLen, 0, nullptr, s_timeout) == static_cast<ssize_t>(requestLen));
   MemFree(requestText);
   if (!sent)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteKeaCommand(%hs): cannot send request to %hs"), command, socketPath);
      closesocket(s);
      return nullptr;
   }

   ByteStream responseData(65536);
   responseData.setAllocationStep(65536);
   size_t offset = 0;
   int depth = 0;
   bool inString = false, escape = false, started = false, complete = false;
   char buffer[16384];
   while(true)
   {
      ssize_t bytes = RecvEx(s, buffer, sizeof(buffer), 0, s_timeout);
      if (bytes <= 0)
         break;
      responseData.write(buffer, bytes);
      if (IsCompleteDocument(reinterpret_cast<const char*>(responseData.buffer()), responseData.size(), &offset, &depth, &inString, &escape, &started))
      {
         complete = true;
         break;
      }
   }
   closesocket(s);

   if (!complete)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteKeaCommand(%hs): incomplete response from %hs (%u bytes)"), command, socketPath, static_cast<uint32_t>(responseData.size()));
      return nullptr;
   }

   json_error_t error;
   json_t *response = json_loadb(reinterpret_cast<const char*>(responseData.buffer()), offset, 0, &error);
   if (response == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteKeaCommand(%hs): cannot parse response from %hs (%hs)"), command, socketPath, error.text);
      return nullptr;
   }

   // Control agent wraps responses into array, direct daemon connection does not
   if (json_is_array(response))
   {
      json_t *element = json_incref(json_array_get(response, 0));
      json_decref(response);
      response = element;
   }
   if (!json_is_object(response))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteKeaCommand(%hs): unexpected response format from %hs"), command, socketPath);
      json_decref(response);
      return nullptr;
   }
   return response;
}

/**
 * Get result code from Kea response
 */
static inline int GetResultCode(json_t *response)
{
   return json_object_get_int32(response, "result", KEA_RESULT_ERROR);
}

/**
 * Get latest value of Kea statistic
 */
static uint64_t GetStatisticValue(json_t *stats, const char *name)
{
   json_t *samples = json_object_get(stats, name);
   json_t *value = json_array_get(json_array_get(samples, 0), 0);
   if (json_is_integer(value))
      return static_cast<uint64_t>(json_integer_value(value));
   if (json_is_real(value))
      return static_cast<uint64_t>(json_real_value(value));
   return 0;
}

/**
 * Get name for subnet from user context or shared network name
 */
static String GetSubnetName(json_t *subnet, json_t *sharedNetwork)
{
   json_t *userContext = json_object_get(subnet, "user-context");
   const char *name = json_object_get_string_utf8(userContext, "name", nullptr);
   if (name == nullptr)
      name = json_object_get_string_utf8(userContext, "comment", nullptr);
   if ((name == nullptr) && (sharedNetwork != nullptr))
      name = json_object_get_string_utf8(sharedNetwork, "name", nullptr);
   return String(CHECK_NULL_EX_A(name), "utf8");
}

/**
 * Build address range list from subnet pools. Pools are specified either as "first - last" or as prefix.
 */
static String GetSubnetRanges(json_t *subnet)
{
   StringBuffer ranges;
   json_t *pools = json_object_get(subnet, "pools");
   size_t i;
   json_t *pool;
   json_array_foreach(pools, i, pool)
   {
      const char *text = json_object_get_string_utf8(pool, "pool", nullptr);
      if (text == nullptr)
         continue;

      if (!ranges.isEmpty())
         ranges.append(_T(", "));
      for(const char *p = text; *p != 0; p++)
      {
         if (*p != ' ')
            ranges.append(static_cast<TCHAR>(*p));
      }
   }
   return ranges;
}

/**
 * Add host reservations defined in subnet configuration
 */
static void AddReservations(KeaScope *scope, json_t *subnet)
{
   json_t *reservations = json_object_get(subnet, "reservations");
   size_t i;
   json_t *r;
   json_array_foreach(reservations, i, r)
   {
      String mac = json_object_get_string(r, "hw-address", _T(""));
      String clientId = json_object_get_string(r, (scope->ipVersion == 4) ? "client-id" : "duid", _T(""));
      String hostname = json_object_get_string(r, "hostname", _T(""));
      if (scope->ipVersion == 4)
      {
         const char *ip = json_object_get_string_utf8(r, "ip-address", nullptr);
         if (ip != nullptr)
            scope->reservations.add(new KeaReservation(String(ip, "utf8"), mac, clientId, hostname, 32));
      }
      else
      {
         size_t j;
         json_t *e;
         json_array_foreach(json_object_get(r, "ip-addresses"), j, e)
         {
            if (json_is_string(e))
               scope->reservations.add(new KeaReservation(String(json_string_value(e), "utf8"), mac, clientId, hostname, 128));
         }
         json_array_foreach(json_object_get(r, "prefixes"), j, e)
         {
            if (!json_is_string(e))
               continue;
            const char *prefix = json_string_value(e);
            const char *slash = strchr(prefix, '/');
            if (slash == nullptr)
               continue;
            String address(prefix, slash - prefix, "utf8");
            scope->reservations.add(new KeaReservation(address, mac, clientId, hostname, atoi(slash + 1)));
         }
      }
   }
}

/**
 * Read subnets from given server and add them to list. Returns false if server configuration cannot be read.
 */
static bool ReadScopes(int ipVersion, ObjectArray<KeaScope> *scopes)
{
   const char *socketPath = GetSocketPath(ipVersion);
   if (socketPath == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("ReadScopes: control socket for DHCPv%d server is not available"), ipVersion);
      return false;
   }

   json_t *response = ExecuteKeaCommand(socketPath, "config-get", nullptr);
   if (response == nullptr)
      return false;
   if (GetResultCode(response) != KEA_RESULT_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ReadScopes: config-get failed on %hs (%hs)"), socketPath, json_object_get_string_utf8(response, "text", ""));
      json_decref(response);
      return false;
   }

   json_t *config = json_object_get(json_object_get(response, "arguments"), (ipVersion == 4) ? "Dhcp4" : "Dhcp6");
   const char *subnetKey = (ipVersion == 4) ? "subnet4" : "subnet6";
   int firstScope = scopes->size();

   json_t *subnetLists[2] = { json_object_get(config, subnetKey), nullptr };
   json_t *sharedNetworks = json_object_get(config, "shared-networks");
   size_t sn = 0;
   json_t *sharedNetwork = nullptr;
   while(true)
   {
      size_t i;
      json_t *subnet;
      json_array_foreach(subnetLists[0], i, subnet)
      {
         const char *prefix = json_object_get_string_utf8(subnet, "subnet", nullptr);
         if (prefix == nullptr)
            continue;
         auto scope = new KeaScope(ipVersion, json_object_get_uint32(subnet, "id"), String(prefix, "utf8"), GetSubnetName(subnet, sharedNetwork), GetSubnetRanges(subnet));
         AddReservations(scope, subnet);
         scopes->add(scope);
      }

      // Continue with subnets from next shared network
      if (sn >= json_array_size(sharedNetworks))
         break;
      sharedNetwork = json_array_get(sharedNetworks, sn++);
      subnetLists[0] = json_object_get(sharedNetwork, subnetKey);
   }
   json_decref(response);

   if (scopes->size() == firstScope)
      return true;

   response = ExecuteKeaCommand(socketPath, "statistic-get-all", nullptr);
   if (response == nullptr)
      return true;
   if (GetResultCode(response) == KEA_RESULT_SUCCESS)
   {
      json_t *stats = json_object_get(response, "arguments");
      for(int i = firstScope; i < scopes->size(); i++)
      {
         KeaScope *scope = scopes->get(i);
         char name[128];
         snprintf(name, sizeof(name), (ipVersion == 4) ? "subnet[%u].total-addresses" : "subnet[%u].total-nas", scope->subnetId);
         scope->total = GetStatisticValue(stats, name);
         snprintf(name, sizeof(name), (ipVersion == 4) ? "subnet[%u].assigned-addresses" : "subnet[%u].assigned-nas", scope->subnetId);
         scope->used = GetStatisticValue(stats, name);
         snprintf(name, sizeof(name), "subnet[%u].declined-addresses", scope->subnetId);
         scope->declined = GetStatisticValue(stats, name);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ReadScopes: statistic-get-all failed on %hs (%hs)"), socketPath, json_object_get_string_utf8(response, "text", ""));
   }
   json_decref(response);
   return true;
}

/**
 * Get scope list (from cache if still valid). Returns nullptr if no Kea server is reachable.
 */
static shared_ptr<ObjectArray<KeaScope>> GetScopes()
{
   LockGuard lockGuard(s_scopesLock);
   time_t now = time(nullptr);
   if ((s_scopes != nullptr) && (now - s_scopesTimestamp < static_cast<time_t>(s_cacheTime)))
      return s_scopes;

   auto scopes = make_shared<ObjectArray<KeaScope>>(0, 32, Ownership::True);
   bool v4 = ReadScopes(4, scopes.get());
   bool v6 = ReadScopes(6, scopes.get());
   if (v4 || v6)
   {
      s_scopes = scopes;
      s_scopesTimestamp = now;
   }
   else
   {
      s_scopes.reset();
   }
   return s_scopes;
}

/**
 * Find scope by ID
 */
static KeaScope *FindScope(ObjectArray<KeaScope> *scopes, const TCHAR *id)
{
   for(int i = 0; i < scopes->size(); i++)
   {
      KeaScope *s = scopes->get(i);
      if (!_tcsicmp(s->network, id))
         return s;
   }
   return nullptr;
}

/**
 * Find scope by subnet ID
 */
static KeaScope *FindScope(ObjectArray<KeaScope> *scopes, int ipVersion, uint32_t subnetId)
{
   for(int i = 0; i < scopes->size(); i++)
   {
      KeaScope *s = scopes->get(i);
      if ((s->ipVersion == ipVersion) && (s->subnetId == subnetId))
         return s;
   }
   return nullptr;
}

/**
 * Handler for DHCP.Scope.* metrics
 */
static LONG H_ScopeMetric(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[256];
   if (!AgentGetParameterArg(metric, 1, id, 256))
      return SYSINFO_RC_UNSUPPORTED;

   shared_ptr<ObjectArray<KeaScope>> scopes = GetScopes();
   if (scopes == nullptr)
      return SYSINFO_RC_ERROR;

   KeaScope *scope = FindScope(scopes.get(), id);
   if (scope == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   switch(*arg)
   {
      case 'D':
         ret_uint64(value, scope->declined);
         break;
      case 'F':
         ret_uint64(value, scope->free());
         break;
      case 'T':
         ret_uint64(value, scope->total);
         break;
      case 'U':
         ret_uint64(value, scope->used);
         break;
      case 'P':
         ret_double(value, scope->utilization(), 2);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for DHCP.Scopes list
 */
static LONG H_ScopeList(const TCHAR *metric, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   shared_ptr<ObjectArray<KeaScope>> scopes = GetScopes();
   if (scopes == nullptr)
      return SYSINFO_RC_ERROR;
   for(int i = 0; i < scopes->size(); i++)
      value->add(scopes->get(i)->network);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for DHCP.Scopes table
 */
static LONG H_ScopeTable(const TCHAR *metric, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   shared_ptr<ObjectArray<KeaScope>> scopes = GetScopes();
   if (scopes == nullptr)
      return SYSINFO_RC_ERROR;

   CreateDHCPScopeTableColumns(value);
   for(int i = 0; i < scopes->size(); i++)
   {
      KeaScope *s = scopes->get(i);
      AddDHCPScopeTableRow(value, s->network, s->ipVersion, s->network, s->name, s->ranges, true, s->total, s->used, s->declined);
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Set table cell from UTF-8 string
 */
static inline void SetUTF8Cell(Table *table, int column, const char *value)
{
   if (value != nullptr)
      table->setPreallocated(column, TStringFromUTF8String(value));
}

/**
 * Read leases from given server into table. Returns false if lease information is not available.
 */
static bool ReadLeases(int ipVersion, ObjectArray<KeaScope> *scopes, KeaScope *scopeFilter, Table *table)
{
   const char *socketPath = GetSocketPath(ipVersion);
   if (socketPath == nullptr)
      return false;

   json_t *args = nullptr;
   if (scopeFilter != nullptr)
   {
      args = json_object();
      json_t *subnets = json_array();
      json_array_append_new(subnets, json_integer(scopeFilter->subnetId));
      json_object_set_new(args, "subnets", subnets);
   }

   const char *command = (ipVersion == 4) ? "lease4-get-all" : "lease6-get-all";
   json_t *response = ExecuteKeaCommand(socketPath, command, args);
   if (response == nullptr)
      return false;

   int rc = GetResultCode(response);
   if ((rc != KEA_RESULT_SUCCESS) && (rc != KEA_RESULT_EMPTY))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ReadLeases: %hs failed on %hs (%hs)"), command, socketPath, json_object_get_string_utf8(response, "text", ""));
      json_decref(response);
      return false;
   }

   StringSet leasedAddresses;
   time_t now = time(nullptr);
   size_t i;
   json_t *lease;
   json_array_foreach(json_object_get(json_object_get(response, "arguments"), "leases"), i, lease)
   {
      const char *ipAddress = json_object_get_string_utf8(lease, "ip-address", nullptr);
      if (ipAddress == nullptr)
         continue;

      KeaScope *scope = FindScope(scopes, ipVersion, json_object_get_uint32(lease, "subnet-id"));
      String ip(ipAddress, "utf8");
      leasedAddresses.add(ip);

      int prefixLength = 32;
      if (ipVersion == 6)
      {
         const char *type = json_object_get_string_utf8(lease, "type", "IA_NA");
         prefixLength = !strcmp(type, "IA_PD") ? json_object_get_int32(lease, "prefix-len", 128) : 128;
      }

      int64_t cltt = json_object_get_int64(lease, "cltt");
      uint32_t validLifetime = json_object_get_uint32(lease, "valid-lft");
      int64_t expiration = (validLifetime != 0xFFFFFFFF) ? cltt + validLifetime : 0;

      int state;
      switch(json_object_get_int32(lease, "state"))
      {
         case KEA_LEASE_STATE_DECLINED:
            state = DHCP_LEASE_STATE_DECLINED;
            break;
         case KEA_LEASE_STATE_EXPIRED_RECLAIMED:
            state = DHCP_LEASE_STATE_EXPIRED;
            break;
         default:
            state = ((expiration != 0) && (expiration < now)) ? DHCP_LEASE_STATE_EXPIRED : DHCP_LEASE_STATE_ACTIVE;
            break;
      }

      bool reserved = false;
      if (scope != nullptr)
      {
         for(int j = 0; j < scope->reservations.size(); j++)
         {
            if (!_tcsicmp(scope->reservations.get(j)->ipAddress, ip))
            {
               reserved = true;
               break;
            }
         }
      }

      table->addRow();
      table->set(DHCP_LEASE_COL_IP_ADDRESS, ip.cstr());
      table->set(DHCP_LEASE_COL_PREFIX_LENGTH, static_cast<int32_t>(prefixLength));
      if (scope != nullptr)
         table->set(DHCP_LEASE_COL_SCOPE_ID, scope->network.cstr());
      SetUTF8Cell(table, DHCP_LEASE_COL_MAC_ADDRESS, json_object_get_string_utf8(lease, "hw-address", nullptr));
      SetUTF8Cell(table, DHCP_LEASE_COL_CLIENT_ID, json_object_get_string_utf8(lease, (ipVersion == 4) ? "client-id" : "duid", nullptr));
      SetUTF8Cell(table, DHCP_LEASE_COL_HOSTNAME, json_object_get_string_utf8(lease, "hostname", nullptr));
      table->set(DHCP_LEASE_COL_TYPE, static_cast<int32_t>(reserved ? DHCP_LEASE_TYPE_RESERVATION : DHCP_LEASE_TYPE_DYNAMIC));
      table->set(DHCP_LEASE_COL_STATE, static_cast<int32_t>(state));
      table->set(DHCP_LEASE_COL_LAST_TRANSACTION, cltt);
      table->set(DHCP_LEASE_COL_EXPIRATION, expiration);
   }
   json_decref(response);

   // Add reservations without active lease
   for(int i = 0; i < scopes->size(); i++)
   {
      KeaScope *scope = scopes->get(i);
      if ((scope->ipVersion != ipVersion) || ((scopeFilter != nullptr) && (scope != scopeFilter)))
         continue;

      for(int j = 0; j < scope->reservations.size(); j++)
      {
         KeaReservation *r = scope->reservations.get(j);
         if (leasedAddresses.contains(r->ipAddress))
            continue;

         table->addRow();
         table->set(DHCP_LEASE_COL_IP_ADDRESS, r->ipAddress.cstr());
         table->set(DHCP_LEASE_COL_PREFIX_LENGTH, static_cast<int32_t>(r->prefixLength));
         table->set(DHCP_LEASE_COL_SCOPE_ID, scope->network.cstr());
         table->set(DHCP_LEASE_COL_MAC_ADDRESS, r->macAddress.cstr());
         table->set(DHCP_LEASE_COL_CLIENT_ID, r->clientId.cstr());
         table->set(DHCP_LEASE_COL_HOSTNAME, r->hostname.cstr());
         table->set(DHCP_LEASE_COL_TYPE, static_cast<int32_t>(DHCP_LEASE_TYPE_RESERVATION));
         table->set(DHCP_LEASE_COL_STATE, static_cast<int32_t>(DHCP_LEASE_STATE_INACTIVE));
         table->set(DHCP_LEASE_COL_LAST_TRANSACTION, static_cast<int64_t>(0));
         table->set(DHCP_LEASE_COL_EXPIRATION, static_cast<int64_t>(0));
      }
   }
   return true;
}

/**
 * Handler for DHCP.Leases and DHCP.Leases(*) tables
 */
static LONG H_LeaseTable(const TCHAR *metric, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   shared_ptr<ObjectArray<KeaScope>> scopes = GetScopes();
   if (scopes == nullptr)
      return SYSINFO_RC_ERROR;

   KeaScope *scopeFilter = nullptr;
   if (*arg == 'S')
   {
      TCHAR id[256];
      if (!AgentGetParameterArg(metric, 1, id, 256))
         return SYSINFO_RC_UNSUPPORTED;
      scopeFilter = FindScope(scopes.get(), id);
      if (scopeFilter == nullptr)
         return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   CreateDHCPLeaseTableColumns(value);
   bool success = false;
   if ((scopeFilter == nullptr) || (scopeFilter->ipVersion == 4))
      success = ReadLeases(4, scopes.get(), scopeFilter, value);
   if ((scopeFilter == nullptr) || (scopeFilter->ipVersion == 6))
      success = ReadLeases(6, scopes.get(), scopeFilter, value) || success;
   return success ? SYSINFO_RC_SUCCESS : SYSINFO_RC_UNSUPPORTED;
}

/**
 * Read socket path from configuration
 */
static void ConfigureSocket(Config *config, const TCHAR *name, char *path, bool *autoDetect)
{
   const TCHAR *value = config->getValue(name);
   if (value == nullptr)
      return;
   *autoDetect = false;
   size_t bytes = tchar_to_utf8(value, -1, path, MAX_PATH - 1);
   path[bytes] = 0;

   struct sockaddr_un addr;
   if (strlen(path) >= sizeof(addr.sun_path))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Control socket path \"%s\" is too long (maximum %u bytes)"), value, static_cast<uint32_t>(sizeof(addr.sun_path) - 1));
      path[0] = 0;
   }
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
   ConfigureSocket(config, _T("/KEA/DHCPv4Socket"), s_socketV4, &s_autoSocketV4);
   ConfigureSocket(config, _T("/KEA/DHCPv6Socket"), s_socketV6, &s_autoSocketV6);
   s_timeout = config->getValueAsUInt(_T("/KEA/Timeout"), s_timeout);
   s_cacheTime = config->getValueAsUInt(_T("/KEA/CacheTime"), s_cacheTime);

   nxlog_debug_tag(DEBUG_TAG, 2, _T("DHCPv4 control socket: %hs"), s_autoSocketV4 ? "auto" : (*s_socketV4 != 0 ? s_socketV4 : "disabled"));
   nxlog_debug_tag(DEBUG_TAG, 2, _T("DHCPv6 control socket: %hs"), s_autoSocketV6 ? "auto" : (*s_socketV6 != 0 ? s_socketV6 : "disabled"));
   return true;
}

/**
 * Supported metrics
 */
static NETXMS_SUBAGENT_PARAM s_metrics[] =
{
   { _T("DHCP.Scope.Declined(*)"), H_ScopeMetric, _T("D"), DCI_DT_UINT64, DCIDESC_DHCP_SCOPE_DECLINED },
   { _T("DHCP.Scope.Free(*)"), H_ScopeMetric, _T("F"), DCI_DT_UINT64, DCIDESC_DHCP_SCOPE_FREE },
   { _T("DHCP.Scope.Total(*)"), H_ScopeMetric, _T("T"), DCI_DT_UINT64, DCIDESC_DHCP_SCOPE_TOTAL },
   { _T("DHCP.Scope.Used(*)"), H_ScopeMetric, _T("U"), DCI_DT_UINT64, DCIDESC_DHCP_SCOPE_USED },
   { _T("DHCP.Scope.Utilization(*)"), H_ScopeMetric, _T("P"), DCI_DT_FLOAT, DCIDESC_DHCP_SCOPE_UTILIZATION }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("DHCP.Scopes"), H_ScopeList, nullptr }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("DHCP.Leases"), H_LeaseTable, _T("A"), _T("IP_ADDRESS"), DCTDESC_DHCP_LEASES },
   { _T("DHCP.Leases(*)"), H_LeaseTable, _T("S"), _T("IP_ADDRESS"), DCTDESC_DHCP_SCOPE_LEASES },
   { _T("DHCP.Scopes"), H_ScopeTable, nullptr, _T("ID"), DCTDESC_DHCP_SCOPES }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("KEA"), NETXMS_VERSION_STRING,
   SubAgentInit,              // init
   nullptr,                   // shutdown
   nullptr,                   // command handler
   nullptr,                   // notify
   nullptr,                   // metric filter
   sizeof(s_metrics) / sizeof(NETXMS_SUBAGENT_PARAM), s_metrics,
   sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST), s_lists,
   sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE), s_tables,
   0, nullptr,                // actions
   0, nullptr                 // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(KEA)
{
   *ppInfo = &s_info;
   return true;
}
