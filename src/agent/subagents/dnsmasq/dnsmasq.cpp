/*
** NetXMS subagent for dnsmasq DHCP server
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
** File: dnsmasq.cpp
**
** Provides generic DHCP.* scope and lease information for dnsmasq. Scopes are built from
** dhcp-range options in dnsmasq configuration, reservations from dhcp-host options (including
** dhcp-hostsfile and dhcp-hostsdir), and leases are read from dnsmasq lease file.
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxcldefs.h>
#include <netxms-version.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <string>
#include <vector>

#if HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#define DEBUG_TAG _T("dnsmasq")

/**
 * Maximum include depth for conf-file / conf-dir
 */
#define MAX_INCLUDE_DEPTH  8

/**
 * Default file locations (first existing one is used)
 */
static const char *s_defaultConfigFiles[] = { "/etc/dnsmasq.conf", "/usr/local/etc/dnsmasq.conf", "/opt/homebrew/etc/dnsmasq.conf", nullptr };
static const char *s_defaultLeaseFiles[] = { "/var/lib/misc/dnsmasq.leases", "/var/lib/dnsmasq/dnsmasq.leases", "/var/db/dnsmasq.leases", "/tmp/dhcp.leases", nullptr };

/**
 * Configuration
 */
static char s_configFile[MAX_PATH] = "";
static char s_leaseFile[MAX_PATH] = "";
static uint32_t s_cacheTime = 5;

/**
 * Host reservation (dhcp-host with address)
 */
struct Reservation
{
   InetAddress address;
   std::string macAddress;
   std::string clientId;
   std::string hostname;
};

/**
 * Scope built from one or more dhcp-range options
 */
struct Scope
{
   MutableString id;
   InetAddress network;    // invalid if network cannot be determined
   std::string name;
   StringBuffer ranges;
   std::vector<std::pair<InetAddress, InetAddress>> rangeList;
   uint64_t total;
   uint64_t used;
   bool staticOnly;
   std::vector<Reservation> reservations;

   Scope() : total(0), used(0), staticOnly(true) { }

   int ipVersion() const
   {
      const InetAddress& a = network.isValid() ? network : rangeList.front().first;
      return (a.getFamily() == AF_INET6) ? 6 : 4;
   }

   bool contains(const InetAddress& addr) const
   {
      if (network.isValid())
         return network.contains(addr);
      for(const auto& r : rangeList)
      {
         if (addr.inRange(r.first, r.second))
            return true;
      }
      return false;
   }
};

/**
 * Parsed dnsmasq configuration and leases
 */
struct ServerState
{
   std::vector<Scope> scopes;
   std::string leaseFile;  // from dhcp-leasefile option
};

/**
 * Cached state
 */
static shared_ptr<ServerState> s_state;
static time_t s_stateTimestamp = 0;
static Mutex s_stateLock;

/**
 * Trim whitespace in place
 */
static std::string Trim(const std::string& s)
{
   size_t start = s.find_first_not_of(" \t\r\n");
   if (start == std::string::npos)
      return std::string();
   size_t end = s.find_last_not_of(" \t\r\n");
   return s.substr(start, end - start + 1);
}

/**
 * Split option value by commas
 */
static std::vector<std::string> SplitValue(const std::string& value)
{
   std::vector<std::string> tokens;
   size_t start = 0;
   while(true)
   {
      size_t pos = value.find(',', start);
      tokens.push_back(Trim(value.substr(start, (pos == std::string::npos) ? std::string::npos : pos - start)));
      if (pos == std::string::npos)
         break;
      start = pos + 1;
   }
   return tokens;
}

/**
 * Check if string starts with given prefix
 */
static inline bool StartsWith(const std::string& s, const char *prefix)
{
   return s.compare(0, strlen(prefix), prefix) == 0;
}

/**
 * Parse IP address token. IPv6 addresses in dhcp-host are enclosed in square brackets.
 */
static InetAddress ParseAddress(const std::string& token)
{
   if ((token.size() > 2) && (token.front() == '[') && (token.back() == ']'))
      return InetAddress::parse(token.substr(1, token.size() - 2).c_str());
   return InetAddress::parse(token.c_str());
}

/**
 * Check if token is MAC address (possibly with hardware type prefix or wildcards)
 */
static bool IsMacAddressToken(const std::string& token)
{
   int groups = 1;
   for(char c : token)
   {
      if (c == ':' || c == '-')
         groups++;
      else if (!isxdigit(static_cast<unsigned char>(c)) && (c != '*'))
         return false;
   }
   return (groups >= 6) && (token.find("::") == std::string::npos);
}

/**
 * Check if token is lease time (number with optional unit or "infinite")
 */
static bool IsLeaseTimeToken(const std::string& token)
{
   if (token == "infinite")
      return true;
   if (token.empty() || !isdigit(static_cast<unsigned char>(token[0])))
      return false;
   size_t i = 0;
   while((i < token.size()) && isdigit(static_cast<unsigned char>(token[i])))
      i++;
   return (i == token.size()) || ((i == token.size() - 1) && strchr("smhdw", token[i]) != nullptr);
}

/**
 * Find network of local interface address containing given address. Returns invalid address if not found.
 * If interface name is given, first global address of matching family on that interface is returned.
 */
static InetAddress FindLocalNetwork(const InetAddress& addr, const char *ifName = nullptr)
{
#if HAVE_IFADDRS_H
   struct ifaddrs *ifaList;
   if (getifaddrs(&ifaList) != 0)
      return InetAddress();

   InetAddress result;
   for(struct ifaddrs *ifa = ifaList; ifa != nullptr; ifa = ifa->ifa_next)
   {
      if ((ifa->ifa_addr == nullptr) || (ifa->ifa_netmask == nullptr))
         continue;
      if ((ifName != nullptr) && strcmp(ifa->ifa_name, ifName))
         continue;

      InetAddress local = InetAddress::createFromSockaddr(ifa->ifa_addr);
      if (!local.isValid() || (local.getFamily() != addr.getFamily()))
         continue;
      InetAddress mask = InetAddress::createFromSockaddr(ifa->ifa_netmask);
      int bits = 0;
      if (mask.getFamily() == AF_INET)
      {
         for(uint32_t m = mask.getAddressV4(); m != 0; m <<= 1)
            bits++;
      }
      else
      {
         const BYTE *m = mask.getAddressV6();
         for(int i = 0; i < 16; i++)
            for(BYTE b = m[i]; b != 0; b <<= 1)
               bits++;
      }
      local.setMaskBits(bits);

      if (ifName != nullptr)
      {
         if (!local.isLinkLocal())
         {
            result = local.getSubnetAddress();
            break;
         }
      }
      else if (local.getSubnetAddress().contains(addr))
      {
         result = local.getSubnetAddress();
         break;
      }
   }
   freeifaddrs(ifaList);
   return result;
#else
   return InetAddress();
#endif
}

/**
 * Calculate number of addresses in range
 */
static uint64_t RangeSize(const InetAddress& start, const InetAddress& end)
{
   if (start.compareTo(end) > 0)
      return 0;
   if (start.getFamily() == AF_INET)
      return static_cast<uint64_t>(end.getAddressV4()) - static_cast<uint64_t>(start.getAddressV4()) + 1;

   const BYTE *s = start.getAddressV6();
   const BYTE *e = end.getAddressV6();
   if (memcmp(s, e, 8) != 0)
      return UINT64_MAX;
   uint64_t sl = 0, el = 0;
   for(int i = 8; i < 16; i++)
   {
      sl = (sl << 8) | s[i];
      el = (el << 8) | e[i];
   }
   return (el - sl == UINT64_MAX) ? UINT64_MAX : el - sl + 1;
}

/**
 * Apply interface prefix to IPv6 address template (constructor: syntax)
 */
static InetAddress ApplyPrefix(const InetAddress& prefix, const InetAddress& suffix)
{
   BYTE addr[16];
   const BYTE *p = prefix.getAddressV6();
   const BYTE *s = suffix.getAddressV6();
   int bits = prefix.getMaskBits();
   for(int i = 0; i < 16; i++)
   {
      int maskBits = std::min(std::max(bits - i * 8, 0), 8);
      BYTE mask = static_cast<BYTE>(0xFF << (8 - maskBits));
      addr[i] = (p[i] & mask) | (s[i] & ~mask);
   }
   return InetAddress(addr);
}

/**
 * Get scope for given network/range, creating new one if needed
 */
static Scope *GetScope(ServerState *state, const InetAddress& network, const InetAddress& start, const InetAddress& end)
{
   String id = network.isValid() ? network.toString() + _T("/") + String::toString(static_cast<int32_t>(network.getMaskBits())) :
         start.toString() + _T("-") + end.toString();
   for(Scope& s : state->scopes)
   {
      if (!_tcscmp(s.id, id))
         return &s;
   }
   state->scopes.emplace_back();
   Scope *scope = &state->scopes.back();
   scope->id = id;
   scope->network = network;
   return scope;
}

/**
 * Parse dhcp-range option
 */
static void ParseRange(ServerState *state, const std::string& value)
{
   std::vector<std::string> tokens = SplitValue(value);
   size_t i = 0;
   std::string tag, constructorIf;
   for(; i < tokens.size(); i++)
   {
      const std::string& t = tokens[i];
      if (StartsWith(t, "set:") || StartsWith(t, "net:"))
      {
         if (tag.empty())
            tag = t.substr(4);
      }
      else if (StartsWith(t, "tag:") || StartsWith(t, "interface:"))
      {
         continue;
      }
      else if (StartsWith(t, "constructor:"))
      {
         constructorIf = t.substr(12);
      }
      else if (ParseAddress(t).isValid())
      {
         break;
      }
      else if (i == 0)
      {
         tag = t;   // Legacy network ID without prefix
      }
   }
   if (i >= tokens.size())
      return;

   InetAddress start = ParseAddress(tokens[i++]);
   InetAddress end = start;
   bool staticMode = false;
   if (i < tokens.size())
   {
      InetAddress a = ParseAddress(tokens[i]);
      if (a.isValid() && (a.getFamily() == start.getFamily()))
      {
         end = a;
         i++;
      }
   }

   InetAddress netmask;
   int prefixLength = -1;
   for(; i < tokens.size(); i++)
   {
      const std::string& t = tokens[i];
      if ((t == "proxy") || (t == "ra-only") || (t == "ra-stateless"))
         return;  // No addresses are leased from this range
      if (t == "static")
      {
         staticMode = true;
      }
      else if (start.getFamily() == AF_INET)
      {
         InetAddress a = ParseAddress(t);
         if (a.isValid() && !netmask.isValid())
            netmask = a;
      }
      else if ((prefixLength == -1) && !t.empty() && (t.find_first_not_of("0123456789") == std::string::npos) && (atoi(t.c_str()) <= 128))
      {
         prefixLength = atoi(t.c_str());
      }
   }

   InetAddress network;
   if (start.getFamily() == AF_INET)
   {
      if (netmask.isValid())
      {
         int bits = 0;
         for(uint32_t m = netmask.getAddressV4(); m != 0; m <<= 1)
            bits++;
         network = InetAddress(start.getAddressV4() & netmask.getAddressV4());
         network.setMaskBits(bits);
      }
      else
      {
         network = FindLocalNetwork(start);
      }
   }
   else
   {
      if (!constructorIf.empty())
      {
         InetAddress prefix = FindLocalNetwork(start, constructorIf.c_str());
         if (!prefix.isValid())
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot find IPv6 prefix for interface %hs (dhcp-range=%hs)"), constructorIf.c_str(), value.c_str());
            return;
         }
         start = ApplyPrefix(prefix, start);
         end = ApplyPrefix(prefix, end);
         if (prefixLength == -1)
            prefixLength = prefix.getMaskBits();
         if (tag.empty())
            tag = constructorIf;
      }
      InetAddress a = start;
      a.setMaskBits((prefixLength != -1) ? prefixLength : 64);
      network = a.getSubnetAddress();
   }

   Scope *scope = GetScope(state, network, start, end);
   if (scope->name.empty())
      scope->name = tag;
   scope->rangeList.emplace_back(start, end);
   if (!scope->ranges.isEmpty())
      scope->ranges.append(_T(", "));
   scope->ranges.append(start.toString());
   if (!staticMode && start.compareTo(end) != 0)
   {
      scope->ranges.append(_T("-"));
      scope->ranges.append(end.toString());
   }
   if (staticMode)
   {
      scope->ranges.append(_T(" (static)"));
   }
   else
   {
      uint64_t size = RangeSize(start, end);
      scope->total = (scope->total > UINT64_MAX - size) ? UINT64_MAX : scope->total + size;
      scope->staticOnly = false;
   }
}

/**
 * Parse dhcp-host option (or line from dhcp-hostsfile)
 */
static void ParseHost(std::vector<Reservation> *reservations, const std::string& value)
{
   Reservation r;
   bool ignore = false;
   for(const std::string& t : SplitValue(value))
   {
      if (t.empty() || StartsWith(t, "set:") || StartsWith(t, "tag:") || StartsWith(t, "net:"))
         continue;
      if (t == "ignore")
      {
         ignore = true;
      }
      else if (StartsWith(t, "id:"))
      {
         if (r.clientId.empty() && (t != "id:*"))
            r.clientId = t.substr(3);
      }
      else if (IsMacAddressToken(t))
      {
         if (r.macAddress.empty())
            r.macAddress = t;
      }
      else if (ParseAddress(t).isValid())
      {
         if (!r.address.isValid())
            r.address = ParseAddress(t);
      }
      else if (IsLeaseTimeToken(t))
      {
         continue;
      }
      else if (r.hostname.empty())
      {
         r.hostname = t;
      }
   }
   if (!ignore && r.address.isValid())
      reservations->push_back(r);
}

/**
 * Read dhcp-hostsfile (one dhcp-host value per line)
 */
static void ReadHostsFile(std::vector<Reservation> *reservations, const char *path)
{
   FILE *f = fopen(path, "r");
   if (f == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot open hosts file %hs (%s)"), path, _tcserror(errno));
      return;
   }
   char line[1024];
   while(fgets(line, sizeof(line), f) != nullptr)
   {
      std::string l = Trim(line);
      if (!l.empty() && (l[0] != '#'))
         ParseHost(reservations, l);
   }
   fclose(f);
}

/**
 * Check if file in configuration directory should be skipped (dnsmasq rules)
 */
static bool SkipDirectoryFile(const char *name, const std::vector<std::string>& filters)
{
   size_t len = strlen(name);
   if ((len == 0) || (name[0] == '.') || (name[len - 1] == '~') || ((name[0] == '#') && (name[len - 1] == '#')))
      return true;

   bool hasInclude = false, included = false;
   for(const std::string& f : filters)
   {
      bool include = !f.empty() && (f[0] == '*');
      const std::string ext = include ? f.substr(1) : f;
      bool match = (len >= ext.size()) && !strcmp(name + len - ext.size(), ext.c_str());
      if (include)
      {
         hasInclude = true;
         if (match)
            included = true;
      }
      else if (match)
      {
         return true;
      }
   }
   return hasInclude && !included;
}

/**
 * List regular files in directory (sorted, as dnsmasq reads them in alphabetical order)
 */
static std::vector<std::string> ListDirectory(const std::string& dir, const std::vector<std::string>& filters)
{
   std::vector<std::string> files;
   DIR *d = opendir(dir.c_str());
   if (d == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot open directory %hs (%s)"), dir.c_str(), _tcserror(errno));
      return files;
   }
   struct dirent *e;
   while((e = readdir(d)) != nullptr)
   {
      if (SkipDirectoryFile(e->d_name, filters))
         continue;
      std::string path = dir + "/" + e->d_name;
      struct stat st;
      if ((stat(path.c_str(), &st) == 0) && S_ISREG(st.st_mode))
         files.push_back(path);
   }
   closedir(d);
   std::sort(files.begin(), files.end());
   return files;
}

/**
 * Read dnsmasq configuration file
 */
static void ReadConfigFile(ServerState *state, std::vector<Reservation> *reservations, const char *path, int depth)
{
   if (depth > MAX_INCLUDE_DEPTH)
      return;

   FILE *f = fopen(path, "r");
   if (f == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot open configuration file %hs (%s)"), path, _tcserror(errno));
      return;
   }

   char line[4096];
   while(fgets(line, sizeof(line), f) != nullptr)
   {
      std::string l = Trim(line);
      if (l.empty() || (l[0] == '#'))
         continue;

      size_t eq = l.find('=');
      std::string name = Trim(l.substr(0, eq));
      std::string value = (eq != std::string::npos) ? Trim(l.substr(eq + 1)) : std::string();
      if (name == "dhcp-range")
      {
         ParseRange(state, value);
      }
      else if (name == "dhcp-host")
      {
         ParseHost(reservations, value);
      }
      else if (name == "dhcp-hostsfile")
      {
         ReadHostsFile(reservations, value.c_str());
      }
      else if (name == "dhcp-hostsdir")
      {
         for(const std::string& file : ListDirectory(value, std::vector<std::string>()))
            ReadHostsFile(reservations, file.c_str());
      }
      else if ((name == "dhcp-leasefile") || (name == "dhcp-lease-file"))
      {
         state->leaseFile = value;
      }
      else if (name == "conf-file")
      {
         ReadConfigFile(state, reservations, value.c_str(), depth + 1);
      }
      else if (name == "conf-dir")
      {
         std::vector<std::string> tokens = SplitValue(value);
         std::string dir = tokens[0];
         while((dir.size() > 1) && (dir.back() == '/'))
            dir.pop_back();
         tokens.erase(tokens.begin());
         for(const std::string& file : ListDirectory(dir, tokens))
            ReadConfigFile(state, reservations, file.c_str(), depth + 1);
      }
   }
   fclose(f);
}

/**
 * Find first existing file from list
 */
static const char *FindExistingFile(const char *configured, const char **defaults)
{
   if (*configured != 0)
      return configured;
   for(const char **p = defaults; *p != nullptr; p++)
   {
      struct stat st;
      if ((stat(*p, &st) == 0) && S_ISREG(st.st_mode))
         return *p;
   }
   return nullptr;
}

/**
 * Lease record from lease file
 */
struct Lease
{
   InetAddress address;
   std::string macAddress;
   std::string clientId;
   std::string hostname;
   time_t expiration;
};

/**
 * Read lease file. Returns false if file exists but cannot be read. Missing file is treated as empty
 * lease list because dnsmasq creates lease file only when first lease is granted.
 */
static bool ReadLeaseFile(const char *path, std::vector<Lease> *leases)
{
   FILE *f = fopen(path, "r");
   if (f == nullptr)
   {
      int err = errno;
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot open lease file %hs (%s)"), path, _tcserror(err));
      return err == ENOENT;
   }

   char line[1024];
   while(fgets(line, sizeof(line), f) != nullptr)
   {
      // IPv4: <expiry> <mac> <ip> <hostname> <client-id>
      // IPv6: <expiry> <iaid> <ip> <hostname> <duid>, preceded by "duid <server-duid>" line
      char *fields[5];
      int count = 0;
      for(char *p = strtok(line, " \t\r\n"); (p != nullptr) && (count < 5); p = strtok(nullptr, " \t\r\n"))
         fields[count++] = p;
      if ((count < 3) || !strcmp(fields[0], "duid"))
         continue;

      Lease lease;
      lease.address = InetAddress::parse(fields[2]);
      if (!lease.address.isValid())
         continue;
      lease.expiration = static_cast<time_t>(strtoll(fields[0], nullptr, 10));
      if (lease.address.getFamily() == AF_INET)
         lease.macAddress = fields[1];
      if ((count > 3) && strcmp(fields[3], "*"))
         lease.hostname = fields[3];
      if ((count > 4) && strcmp(fields[4], "*"))
         lease.clientId = fields[4];
      leases->push_back(lease);
   }
   fclose(f);
   return true;
}

/**
 * Get lease file path
 */
static const char *GetLeaseFilePath(const ServerState *state)
{
   if (*s_leaseFile != 0)
      return s_leaseFile;
   if (!state->leaseFile.empty())
      return state->leaseFile.c_str();
   return FindExistingFile("", s_defaultLeaseFiles);
}

/**
 * Check if lease is active
 */
static inline bool IsLeaseActive(const Lease& lease, time_t now)
{
   return (lease.expiration == 0) || (lease.expiration > now);
}

/**
 * Load server state (configuration and lease counts). Returns nullptr if DHCP is not configured.
 */
static shared_ptr<ServerState> LoadState()
{
   const char *configFile = FindExistingFile(s_configFile, s_defaultConfigFiles);
   if (configFile == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("dnsmasq configuration file not found"));
      return shared_ptr<ServerState>();
   }

   auto state = make_shared<ServerState>();
   std::vector<Reservation> reservations;
   ReadConfigFile(state.get(), &reservations, configFile, 0);
   if (state->scopes.empty())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("No DHCP ranges defined in dnsmasq configuration"));
      return shared_ptr<ServerState>();
   }

   for(const Reservation& r : reservations)
   {
      for(Scope& s : state->scopes)
      {
         if (s.contains(r.address))
         {
            s.reservations.push_back(r);
            break;
         }
      }
   }

   // Static-only scopes serve reserved addresses only
   for(Scope& s : state->scopes)
   {
      if (s.staticOnly)
         s.total = s.reservations.size();
   }

   const char *leaseFile = GetLeaseFilePath(state.get());
   std::vector<Lease> leases;
   if ((leaseFile != nullptr) && ReadLeaseFile(leaseFile, &leases))
   {
      time_t now = time(nullptr);
      for(const Lease& l : leases)
      {
         if (!IsLeaseActive(l, now))
            continue;
         for(Scope& s : state->scopes)
         {
            if (s.contains(l.address))
            {
               s.used++;
               break;
            }
         }
      }
   }
   return state;
}

/**
 * Get server state (from cache if still valid)
 */
static shared_ptr<ServerState> GetState()
{
   LockGuard lockGuard(s_stateLock);
   time_t now = time(nullptr);
   if ((s_state == nullptr) || (now - s_stateTimestamp >= static_cast<time_t>(s_cacheTime)))
   {
      s_state = LoadState();
      s_stateTimestamp = now;
   }
   return s_state;
}

/**
 * Find scope by ID
 */
static const Scope *FindScope(const ServerState *state, const TCHAR *id)
{
   for(const Scope& s : state->scopes)
   {
      if (!_tcsicmp(s.id, id))
         return &s;
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

   shared_ptr<ServerState> state = GetState();
   if (state == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   const Scope *scope = FindScope(state.get(), id);
   if (scope == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   switch(*arg)
   {
      case 'F':
         ret_uint64(value, DHCPScopeFreeAddresses(scope->total, scope->used, 0));
         break;
      case 'T':
         ret_uint64(value, scope->total);
         break;
      case 'U':
         ret_uint64(value, scope->used);
         break;
      case 'P':
         ret_double(value, DHCPScopeUtilization(scope->total, scope->used), 2);
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
   shared_ptr<ServerState> state = GetState();
   if (state == nullptr)
      return SYSINFO_RC_UNSUPPORTED;
   for(const Scope& s : state->scopes)
      value->add(s.id);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for DHCP.Scopes table
 */
static LONG H_ScopeTable(const TCHAR *metric, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   shared_ptr<ServerState> state = GetState();
   if (state == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   CreateDHCPScopeTableColumns(value);
   for(const Scope& s : state->scopes)
   {
      String network = s.network.isValid() ? s.id : String();
      String name(s.name.c_str(), "utf8");
      AddDHCPScopeTableRow(value, s.id, s.ipVersion(), network, name, s.ranges, true, s.total, s.used);  // dnsmasq does not track declined addresses
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Set table cell from UTF-8 string
 */
static inline void SetUTF8Cell(Table *table, int column, const std::string& value)
{
   if (!value.empty())
      table->setPreallocated(column, TStringFromUTF8String(value.c_str()));
}

/**
 * Handler for DHCP.Leases and DHCP.Leases(*) tables
 */
static LONG H_LeaseTable(const TCHAR *metric, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   shared_ptr<ServerState> state = GetState();
   if (state == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   const Scope *scopeFilter = nullptr;
   if (*arg == 'S')
   {
      TCHAR id[256];
      if (!AgentGetParameterArg(metric, 1, id, 256))
         return SYSINFO_RC_UNSUPPORTED;
      scopeFilter = FindScope(state.get(), id);
      if (scopeFilter == nullptr)
         return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   const char *leaseFile = GetLeaseFilePath(state.get());
   std::vector<Lease> leases;
   if ((leaseFile != nullptr) && !ReadLeaseFile(leaseFile, &leases))
      return SYSINFO_RC_ERROR;

   CreateDHCPLeaseTableColumns(value);
   time_t now = time(nullptr);
   std::vector<const Reservation*> usedReservations;
   for(const Lease& l : leases)
   {
      const Scope *scope = nullptr;
      for(const Scope& s : state->scopes)
      {
         if (s.contains(l.address))
         {
            scope = &s;
            break;
         }
      }
      if ((scopeFilter != nullptr) && (scope != scopeFilter))
         continue;

      bool reserved = false;
      if (scope != nullptr)
      {
         for(const Reservation& r : scope->reservations)
         {
            if (r.address.equals(l.address))
            {
               reserved = true;
               usedReservations.push_back(&r);
               break;
            }
         }
      }

      value->addRow();
      value->set(DHCP_LEASE_COL_IP_ADDRESS, l.address.toString().cstr());
      value->set(DHCP_LEASE_COL_PREFIX_LENGTH, static_cast<int32_t>((l.address.getFamily() == AF_INET) ? 32 : 128));
      if (scope != nullptr)
         value->set(DHCP_LEASE_COL_SCOPE_ID, scope->id.cstr());
      SetUTF8Cell(value, DHCP_LEASE_COL_MAC_ADDRESS, l.macAddress);
      SetUTF8Cell(value, DHCP_LEASE_COL_CLIENT_ID, l.clientId);
      SetUTF8Cell(value, DHCP_LEASE_COL_HOSTNAME, l.hostname);
      value->set(DHCP_LEASE_COL_TYPE, static_cast<int32_t>(reserved ? DHCP_LEASE_TYPE_RESERVATION : DHCP_LEASE_TYPE_DYNAMIC));
      value->set(DHCP_LEASE_COL_STATE, static_cast<int32_t>(IsLeaseActive(l, now) ? DHCP_LEASE_STATE_ACTIVE : DHCP_LEASE_STATE_EXPIRED));
      value->set(DHCP_LEASE_COL_LAST_TRANSACTION, static_cast<int64_t>(0));
      value->set(DHCP_LEASE_COL_EXPIRATION, static_cast<int64_t>(l.expiration));
   }

   // Reservations without lease
   for(const Scope& s : state->scopes)
   {
      if ((scopeFilter != nullptr) && (&s != scopeFilter))
         continue;
      for(const Reservation& r : s.reservations)
      {
         if (std::find(usedReservations.begin(), usedReservations.end(), &r) != usedReservations.end())
            continue;
         value->addRow();
         value->set(DHCP_LEASE_COL_IP_ADDRESS, r.address.toString().cstr());
         value->set(DHCP_LEASE_COL_PREFIX_LENGTH, static_cast<int32_t>((r.address.getFamily() == AF_INET) ? 32 : 128));
         value->set(DHCP_LEASE_COL_SCOPE_ID, s.id.cstr());
         SetUTF8Cell(value, DHCP_LEASE_COL_MAC_ADDRESS, r.macAddress);
         SetUTF8Cell(value, DHCP_LEASE_COL_CLIENT_ID, r.clientId);
         SetUTF8Cell(value, DHCP_LEASE_COL_HOSTNAME, r.hostname);
         value->set(DHCP_LEASE_COL_TYPE, static_cast<int32_t>(DHCP_LEASE_TYPE_RESERVATION));
         value->set(DHCP_LEASE_COL_STATE, static_cast<int32_t>(DHCP_LEASE_STATE_INACTIVE));
         value->set(DHCP_LEASE_COL_LAST_TRANSACTION, static_cast<int64_t>(0));
         value->set(DHCP_LEASE_COL_EXPIRATION, static_cast<int64_t>(0));
      }
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Read file path from configuration
 */
static void ConfigurePath(Config *config, const TCHAR *name, char *path)
{
   const TCHAR *value = config->getValue(name);
   if (value == nullptr)
      return;
   size_t bytes = tchar_to_utf8(value, -1, path, MAX_PATH - 1);
   path[bytes] = 0;
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
   ConfigurePath(config, _T("/DNSMASQ/ConfigFile"), s_configFile);
   ConfigurePath(config, _T("/DNSMASQ/LeaseFile"), s_leaseFile);
   s_cacheTime = config->getValueAsUInt(_T("/DNSMASQ/CacheTime"), s_cacheTime);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Configuration file: %hs"), (*s_configFile != 0) ? s_configFile : "auto");
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Lease file: %hs"), (*s_leaseFile != 0) ? s_leaseFile : "auto");
   return true;
}

/**
 * Supported metrics
 */
static NETXMS_SUBAGENT_PARAM s_metrics[] =
{
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
   _T("DNSMASQ"), NETXMS_VERSION_STRING,
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
DECLARE_SUBAGENT_ENTRY_POINT(DNSMASQ)
{
   *ppInfo = &s_info;
   return true;
}
