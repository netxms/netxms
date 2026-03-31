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
** File: ua.cpp
**/

#include "asterisk.h"
#include <netxms-version.h>
#include <tls_conn.h>

/**
 * SIP status codes
 */
static CodeLookupElement s_sipStatusCodes[] =
{
   { 901, _T("DNS Resolution Failed") },
   { 902, _T("Connection Failed") },
   { 903, _T("TLS Handshake Failed") },
   { 904, _T("Send Failed") },
   { 100, _T("Trying") },
   { 180, _T("Ringing") },
   { 181, _T("Call is Being Forwarded") },
   { 182, _T("Queued") },
   { 183, _T("Session Progress") },
   { 199, _T("Early Dialog Terminated") },
   { 200, _T("OK") },
   { 202, _T("Accepted") },
   { 204, _T("No Notification") },
   { 300, _T("Multiple Choices") },
   { 301, _T("Moved Permanently") },
   { 302, _T("Moved Temporarily") },
   { 305, _T("Use Proxy") },
   { 380, _T("Alternative Service") },
   { 400, _T("Bad Request") },
   { 401, _T("Unauthorized") },
   { 402, _T("Payment Required") },
   { 403, _T("Forbidden") },
   { 404, _T("Not Found") },
   { 405, _T("Method Not Allowed") },
   { 406, _T("Not Acceptable") },
   { 407, _T("Proxy Authentication Required") },
   { 408, _T("Request Timeout") },
   { 409, _T("Conflict") },
   { 410, _T("Gone") },
   { 411, _T("Length Required") },
   { 412, _T("Conditional Request Failed") },
   { 413, _T("Request Entity Too Large") },
   { 414, _T("Request-URI Too Long") },
   { 415, _T("Unsupported Media Type") },
   { 416, _T("Unsupported URI Scheme") },
   { 417, _T("Unknown Resource-Priority") },
   { 420, _T("Bad Extension") },
   { 421, _T("Extension Required") },
   { 422, _T("Session Interval Too Small") },
   { 423, _T("Interval Too Brief") },
   { 424, _T("Bad Location Information") },
   { 428, _T("Use Identity Header") },
   { 429, _T("Provide Referrer Identity") },
   { 430, _T("Flow Failed") },
   { 433, _T("Anonymity Disallowed") },
   { 436, _T("Bad Identity-Info") },
   { 437, _T("Unsupported Certificate") },
   { 438, _T("Invalid Identity Header") },
   { 439, _T("First Hop Lacks Outbound Support") },
   { 440, _T("Max-Breadth Exceeded") },
   { 469, _T("Bad Info Package") },
   { 470, _T("Consent Needed") },
   { 480, _T("Temporarily Unavailable") },
   { 481, _T("Call/Transaction Does Not Exist") },
   { 482, _T("Loop Detected") },
   { 483, _T("Too Many Hops") },
   { 484, _T("Address Incomplete") },
   { 485, _T("Ambiguous") },
   { 486, _T("Busy Here") },
   { 487, _T("Request Terminated") },
   { 488, _T("Not Acceptable Here") },
   { 489, _T("Bad Event") },
   { 491, _T("Request Pending") },
   { 493, _T("Undecipherable") },
   { 494, _T("Security Agreement Required") },
   { 500, _T("Internal Server Error") },
   { 501, _T("Not Implemented") },
   { 502, _T("Bad Gateway") },
   { 503, _T("Service Unavailable") },
   { 504, _T("Server Time-out") },
   { 505, _T("Version Not Supported") },
   { 513, _T("Message Too Large") },
   { 555, _T("Push Notification Service Not Supported") },
   { 580, _T("Precondition Failure") },
   { 600, _T("Busy Everywhere") },
   { 603, _T("Decline") },
   { 604, _T("Does Not Exist Anywhere") },
   { 606, _T("Not Acceptable") },
   { 607, _T("Unwanted") },
   { 608, _T("Rejected") },
   { 0, nullptr }
};

/**
 * SIP transport type
 */
enum SIPTransport
{
   SIP_UDP,
   SIP_TCP,
   SIP_TLS
};

/**
 * Parsed SIP proxy address
 */
struct SIPProxyAddress
{
   char host[256];
   uint16_t port;
   SIPTransport transport;
};

/**
 * Check if a string looks like an IPv6 address (contains two or more colons, or is bracketed)
 */
static inline bool IsIPv6Address(const char *s)
{
   return (s[0] == '[') || (strchr(s, ':') != nullptr && strchr(strchr(s, ':') + 1, ':') != nullptr);
}

/**
 * Parse SIP proxy URI into host, port, and transport.
 * Supported formats:
 *   sip:host            -> UDP:5060
 *   sip:host:port       -> UDP:port
 *   sip:host;transport=tcp -> TCP:5060
 *   sip:host;transport=udp -> UDP:5060
 *   sips:host           -> TLS:5061
 *   sips:host:port      -> TLS:port
 */
static bool ParseSIPProxyURI(const char *uri, SIPProxyAddress *addr)
{
   bool isSips = false;
   if (!strnicmp(uri, "sips:", 5))
   {
      isSips = true;
      uri += 5;
   }
   else if (!strnicmp(uri, "sip:", 4))
   {
      uri += 4;
   }
   else
   {
      return false;
   }

   addr->transport = isSips ? SIP_TLS : SIP_UDP;
   addr->port = isSips ? 5061 : 5060;

   // Split off ;parameters from the host:port part
   char buffer[512];
   strlcpy(buffer, uri, sizeof(buffer));

   char *params = strchr(buffer, ';');
   if (params != nullptr)
   {
      *params = '\0';
      params++;

      // Parse transport parameter (ignored for sips: URIs which always require TLS)
      const char *tp = stristr(params, "transport=");
      if (tp != nullptr && !isSips)
      {
         tp += 10;
         if (!strnicmp(tp, "tcp", 3))
            addr->transport = SIP_TCP;
         else if (!strnicmp(tp, "udp", 3))
            addr->transport = SIP_UDP;
         else if (!strnicmp(tp, "tls", 3))
         {
            addr->transport = SIP_TLS;
            addr->port = 5061;
         }
      }
   }

   // Parse host and optional port
   // Handle IPv6 addresses in brackets: [::1]:port
   if (buffer[0] == '[')
   {
      char *closeBracket = strchr(buffer, ']');
      if (closeBracket == nullptr)
         return false;

      *closeBracket = '\0';
      strlcpy(addr->host, &buffer[1], sizeof(addr->host));

      char *portStr = closeBracket + 1;
      if (*portStr == ':')
      {
         unsigned long p = strtoul(portStr + 1, nullptr, 10);
         if (p == 0 || p > 65535)
            return false;
         addr->port = static_cast<uint16_t>(p);
      }
   }
   else
   {
      char *colon = strchr(buffer, ':');
      if (colon != nullptr)
      {
         // Reject bare IPv6 addresses without brackets (e.g. "2001:db8::1")
         if (strchr(colon + 1, ':') != nullptr)
            return false;
         *colon = '\0';
         unsigned long p = strtoul(colon + 1, nullptr, 10);
         if (p == 0 || p > 65535)
            return false;
         addr->port = static_cast<uint16_t>(p);
      }
      strlcpy(addr->host, buffer, sizeof(addr->host));
   }

   return addr->host[0] != '\0';
}

/**
 * Get SIP transport name string
 */
static const char *SIPTransportName(SIPTransport transport)
{
   switch(transport)
   {
      case SIP_TCP:
         return "TCP";
      case SIP_TLS:
         return "TLS";
      case SIP_UDP:
      default:
         return "UDP";
   }
}

/**
 * SIP timer constants for UDP retransmission (RFC 3261 Section 17.1.2.2)
 */
static const int SIP_T1 = 500;
static const int SIP_T2 = 4000;

/**
 * Build SIP REGISTER message
 */
static bool BuildSIPRegister(char *buffer, size_t bufferSize, const char *login, const char *domain,
   const SIPProxyAddress *addr, const char *localIp, uint16_t localPort, const char *callId,
   int cseq, const char *branch, const char *fromTag, int expires, const char *authHeader)
{
   const char *transport = SIPTransportName(addr->transport);
   char contactTransport[32] = "";
   if (addr->transport != SIP_UDP)
      snprintf(contactTransport, sizeof(contactTransport), ";transport=%s", transport);

   // Wrap IPv6 addresses in brackets for Via, Contact, Call-ID, and domain headers
   bool isIPv6Local = (strchr(localIp, ':') != nullptr);
   const char *lob = isIPv6Local ? "[" : "";
   const char *lcb = isIPv6Local ? "]" : "";

   bool isIPv6Domain = IsIPv6Address(domain);
   const char *dob = (isIPv6Domain && domain[0] != '[') ? "[" : "";
   const char *dcb = (isIPv6Domain && domain[0] != '[') ? "]" : "";

   int offset = snprintf(buffer, bufferSize,
      "REGISTER sip:%s%s%s SIP/2.0\r\n"
      "Via: SIP/2.0/%s %s%s%s:%u;rport;branch=z9hG4bK-%s\r\n"
      "Max-Forwards: 70\r\n"
      "From: <sip:%s@%s%s%s>;tag=%s\r\n"
      "To: <sip:%s@%s%s%s>\r\n"
      "Call-ID: %s@%s%s%s\r\n"
      "CSeq: %d REGISTER\r\n"
      "Contact: <sip:%s@%s%s%s:%u%s>\r\n"
      "User-Agent: NetXMS/" NETXMS_BUILD_TAG_A "\r\n"
      "Expires: %d\r\n",
      dob, domain, dcb,
      transport, lob, localIp, lcb, localPort, branch,
      login, dob, domain, dcb, fromTag,
      login, dob, domain, dcb,
      callId, lob, localIp, lcb,
      cseq,
      login, lob, localIp, lcb, localPort, contactTransport,
      expires);

   if (offset < 0 || static_cast<size_t>(offset) >= bufferSize)
      return false;

   if (authHeader != nullptr)
   {
      int n = snprintf(buffer + offset, bufferSize - offset, "%s\r\n", authHeader);
      if (n < 0 || static_cast<size_t>(n) >= bufferSize - offset)
         return false;
      offset += n;
   }

   int finalOffset = snprintf(buffer + offset, bufferSize - offset, "Content-Length: 0\r\n\r\n");
   return (finalOffset > 0) && (static_cast<size_t>(finalOffset) < bufferSize - offset);
}

/**
 * Parse SIP response status code from first line (e.g. "SIP/2.0 200 OK")
 */
static int ParseSIPResponse(const char *response, size_t length)
{
   if ((length < 12) || strncmp(response, "SIP/2.0 ", 8))
      return -1;
   return (int)strtol(response + 8, nullptr, 10);
}

/**
 * Extract a SIP header value from a response message.
 * If searchFrom is not null, searching starts from *searchFrom (or from response if *searchFrom is null).
 * On match, *searchFrom is updated to point past the matched line, allowing iteration over multiple
 * headers with the same name.
 */
static bool ExtractSIPHeader(const char *response, size_t length, const char *headerName, char *value, size_t valueSize, const char **searchFrom = nullptr)
{
   size_t nameLen = strlen(headerName);
   const char *end = response + length;
   const char *p = (searchFrom != nullptr && *searchFrom != nullptr) ? *searchFrom : response;

   while(p < end)
   {
      // Find next line
      const char *lineEnd = static_cast<const char*>(memchr(p, '\n', end - p));
      if (lineEnd == nullptr)
         lineEnd = end;

      size_t lineLen = lineEnd - p;
      // Strip trailing \r
      if ((lineLen > 0) && (p[lineLen - 1] == '\r'))
         lineLen--;

      // Empty line means end of headers
      if (lineLen == 0)
         break;

      // Check if line starts with the header name followed by ':'
      if ((lineLen > nameLen + 1) && !strnicmp(p, headerName, nameLen) && (p[nameLen] == ':'))
      {
         const char *val = p + nameLen + 1;
         size_t valLen = lineLen - nameLen - 1;
         // Skip leading whitespace
         while((valLen > 0) && (*val == ' ' || *val == '\t'))
         {
            val++;
            valLen--;
         }
         if (valLen >= valueSize)
            valLen = valueSize - 1;
         memcpy(value, val, valLen);
         size_t written = valLen;

         // Unfold continuation lines (RFC 3261 Section 7.3.1)
         const char *nextLine = lineEnd + 1;
         while(nextLine < end && (*nextLine == ' ' || *nextLine == '\t'))
         {
            const char *contEnd = static_cast<const char*>(memchr(nextLine, '\n', end - nextLine));
            if (contEnd == nullptr)
               contEnd = end;
            size_t contLen = contEnd - nextLine;
            if ((contLen > 0) && (nextLine[contLen - 1] == '\r'))
               contLen--;
            // Skip leading whitespace on continuation line
            const char *contVal = nextLine;
            size_t contValLen = contLen;
            while((contValLen > 0) && (*contVal == ' ' || *contVal == '\t'))
            {
               contVal++;
               contValLen--;
            }
            // Replace leading folding whitespace with a single space; use trimmed length for bounds check
            if (written + 1 + contValLen < valueSize)
            {
               value[written++] = ' ';
               memcpy(value + written, contVal, contValLen);
               written += contValLen;
            }
            nextLine = contEnd + 1;
            lineEnd = contEnd;
         }
         value[written] = '\0';

         if (searchFrom != nullptr)
            *searchFrom = lineEnd + 1;
         return true;
      }

      p = lineEnd + 1;
   }
   return false;
}

/**
 * Parsed digest authentication challenge
 */
struct DigestChallenge
{
   char realm[256];
   char nonce[256];
   char opaque[256];
   char qop[64];
   char algorithm[32];
};

/**
 * Parse a quoted or unquoted value from a digest challenge string.
 * Returns pointer past the parsed value, or nullptr on failure.
 */
static const char *ParseDigestParam(const char *p, char *value, size_t valueSize)
{
   if (*p == '"')
   {
      p++;
      size_t i = 0;
      while(*p != '\0' && *p != '"')
      {
         if (*p == '\\' && *(p + 1) != '\0')
         {
            p++;
            if (i < valueSize - 1)
               value[i++] = *p;
         }
         else if (*p != '\r' && *p != '\n')
         {
            if (i < valueSize - 1)
               value[i++] = *p;
         }
         p++;
      }
      value[i] = '\0';
      if (*p == '"')
         p++;
   }
   else
   {
      size_t i = 0;
      while(*p != '\0' && *p != ',' && *p != ' ' && *p != '\t')
      {
         if (i < valueSize - 1 && *p != '\r' && *p != '\n')
            value[i++] = *p;
         p++;
      }
      value[i] = '\0';
   }
   return p;
}

/**
 * Parse a Digest challenge from WWW-Authenticate header value
 */
static bool ParseDigestChallenge(const char *wwwAuth, DigestChallenge *challenge)
{
   memset(challenge, 0, sizeof(DigestChallenge));

   // Skip "Digest " prefix (case-insensitive)
   while(*wwwAuth == ' ' || *wwwAuth == '\t')
      wwwAuth++;
   if (strnicmp(wwwAuth, "Digest ", 7) != 0)
      return false;
   wwwAuth += 7;

   while(*wwwAuth != '\0')
   {
      // Skip whitespace and commas
      while(*wwwAuth == ' ' || *wwwAuth == '\t' || *wwwAuth == ',')
         wwwAuth++;
      if (*wwwAuth == '\0')
         break;

      // Read parameter name
      const char *eq = strchr(wwwAuth, '=');
      if (eq == nullptr)
         break;

      size_t nameLen = eq - wwwAuth;
      // Trim trailing whitespace from name
      while(nameLen > 0 && (wwwAuth[nameLen - 1] == ' ' || wwwAuth[nameLen - 1] == '\t'))
         nameLen--;

      const char *val = eq + 1;
      // Skip optional whitespace after '=' (RFC 7235 BWS)
      while(*val == ' ' || *val == '\t')
         val++;
      if (!strnicmp(wwwAuth, "realm", nameLen) && nameLen == 5)
         wwwAuth = ParseDigestParam(val, challenge->realm, sizeof(challenge->realm));
      else if (!strnicmp(wwwAuth, "nonce", nameLen) && nameLen == 5)
         wwwAuth = ParseDigestParam(val, challenge->nonce, sizeof(challenge->nonce));
      else if (!strnicmp(wwwAuth, "opaque", nameLen) && nameLen == 6)
         wwwAuth = ParseDigestParam(val, challenge->opaque, sizeof(challenge->opaque));
      else if (!strnicmp(wwwAuth, "qop", nameLen) && nameLen == 3)
         wwwAuth = ParseDigestParam(val, challenge->qop, sizeof(challenge->qop));
      else if (!strnicmp(wwwAuth, "algorithm", nameLen) && nameLen == 9)
         wwwAuth = ParseDigestParam(val, challenge->algorithm, sizeof(challenge->algorithm));
      else
      {
         // Skip unknown parameter
         char dummy[256];
         wwwAuth = ParseDigestParam(val, dummy, sizeof(dummy));
      }
   }

   return challenge->realm[0] != '\0' && challenge->nonce[0] != '\0';
}

/**
 * Check if digest algorithm is supported by this implementation.
 */
static bool IsSupportedDigestAlgorithm(const char *algorithm)
{
   return algorithm[0] == '\0' ||
      !stricmp(algorithm, "MD5") || !stricmp(algorithm, "MD5-sess") ||
      !stricmp(algorithm, "SHA-256") || !stricmp(algorithm, "SHA-256-sess");
}

/**
 * Check if qop options include "auth" (or qop is absent, meaning RFC 2069 compat).
 */
static bool HasSupportedQop(const char *qop)
{
   if (qop[0] == '\0')
      return true;
   const char *p = qop;
   while(*p != '\0')
   {
      while(*p == ' ' || *p == ',')
         p++;
      if (*p == '\0')
         break;
      const char *tokenStart = p;
      while(*p != '\0' && *p != ',' && *p != ' ')
         p++;
      size_t tokenLen = p - tokenStart;
      if (tokenLen == 4 && !strnicmp(tokenStart, "auth", 4))
         return true;
   }
   return false;
}

/**
 * Compute digest response per RFC 2617 / RFC 7616.
 * Supports MD5, MD5-sess, SHA-256, and SHA-256-sess algorithms.
 */
static void ComputeDigestResponse(const char *login, const char *realm, const char *password,
   const char *nonce, const char *method, const char *uri, const char *nc,
   const char *cnonce, const char *qop, const char *algorithm, char *response, size_t responseSize)
{
   bool useSha256 = (algorithm != nullptr) && (!stricmp(algorithm, "SHA-256") || !stricmp(algorithm, "SHA-256-sess"));
   bool useMD5 = (algorithm == nullptr) || (algorithm[0] == '\0') || !stricmp(algorithm, "MD5") || !stricmp(algorithm, "MD5-sess");
   bool useSess = (algorithm != nullptr) && (!stricmp(algorithm, "MD5-sess") || !stricmp(algorithm, "SHA-256-sess"));
   if (!useSha256 && !useMD5)
   {
      strlcpy(response, "", responseSize);
      return;
   }
   size_t hashSize = useSha256 ? SHA256_DIGEST_SIZE : MD5_DIGEST_SIZE;

   // All variables declared before first goto to comply with ISO C++ [stmt.dcl]/3
   char ha1Hex[SHA256_DIGEST_SIZE * 2 + 1];
   char ha2Hex[SHA256_DIGEST_SIZE * 2 + 1];
   BYTE hash[SHA256_DIGEST_SIZE];
   char ha1Input[1024];
   char ha2Input[1024];
   char responseInput[2048];
   int ha1Written, ha2Written, respWritten;
   size_t ha1Len, ha2Len, responseLen;

   strlcpy(response, "", responseSize);

   // HA1 = H(login:realm:password) for base algorithms
   // HA1 = H(H(login:realm:password):nonce:cnonce) for -sess variants
   ha1Written = snprintf(ha1Input, sizeof(ha1Input), "%s:%s:%s", login, realm, password);
   if (ha1Written < 0)
      goto cleanup;
   ha1Len = std::min(static_cast<size_t>(ha1Written), sizeof(ha1Input) - 1);
   if (useSha256)
      CalculateSHA256Hash(ha1Input, ha1Len, hash);
   else
      CalculateMD5Hash(ha1Input, ha1Len, hash);
   BinToStrAL(hash, hashSize, ha1Hex);

   if (useSess)
   {
      ha1Written = snprintf(ha1Input, sizeof(ha1Input), "%s:%s:%s", ha1Hex, nonce, cnonce);
      if (ha1Written < 0)
         goto cleanup;
      ha1Len = std::min(static_cast<size_t>(ha1Written), sizeof(ha1Input) - 1);
      if (useSha256)
         CalculateSHA256Hash(ha1Input, ha1Len, hash);
      else
         CalculateMD5Hash(ha1Input, ha1Len, hash);
      BinToStrAL(hash, hashSize, ha1Hex);
   }

   // HA2 = H(method:uri)
   ha2Written = snprintf(ha2Input, sizeof(ha2Input), "%s:%s", method, uri);
   if (ha2Written < 0)
      goto cleanup;
   ha2Len = std::min(static_cast<size_t>(ha2Written), sizeof(ha2Input) - 1);
   if (useSha256)
      CalculateSHA256Hash(ha2Input, ha2Len, hash);
   else
      CalculateMD5Hash(ha2Input, ha2Len, hash);
   BinToStrAL(hash, hashSize, ha2Hex);

   // Final response
   if (qop != nullptr && *qop != '\0')
   {
      // response = H(HA1:nonce:nc:cnonce:qop:HA2)
      respWritten = snprintf(responseInput, sizeof(responseInput), "%s:%s:%s:%s:%s:%s",
         ha1Hex, nonce, nc, cnonce, qop, ha2Hex);
   }
   else
   {
      // response = H(HA1:nonce:HA2)
      respWritten = snprintf(responseInput, sizeof(responseInput), "%s:%s:%s",
         ha1Hex, nonce, ha2Hex);
   }
   if (respWritten < 0)
      goto cleanup;
   responseLen = std::min(static_cast<size_t>(respWritten), sizeof(responseInput) - 1);
   if (useSha256)
      CalculateSHA256Hash(responseInput, responseLen, hash);
   else
      CalculateMD5Hash(responseInput, responseLen, hash);
   BinToStrAL(hash, hashSize, responseInput);

   strlcpy(response, responseInput, responseSize);

cleanup:
   SecureZeroMemory(ha1Input, sizeof(ha1Input));
   SecureZeroMemory(ha2Input, sizeof(ha2Input));
   SecureZeroMemory(responseInput, sizeof(responseInput));
   SecureZeroMemory(ha1Hex, sizeof(ha1Hex));
   SecureZeroMemory(ha2Hex, sizeof(ha2Hex));
   SecureZeroMemory(hash, sizeof(hash));
}

/**
 * Write a quoted-string value with proper backslash escaping per RFC 7230 section 3.2.6.
 * Returns number of characters written, or -1 if buffer is too small.
 */
static int WriteQuotedString(char *buffer, size_t bufferSize, const char *value)
{
   size_t pos = 0;
   if (pos >= bufferSize)
      return -1;
   buffer[pos++] = '"';
   for (const char *p = value; *p != '\0'; p++)
   {
      if (*p == '"' || *p == '\\')
      {
         if (pos + 2 >= bufferSize)
            return -1;
         buffer[pos++] = '\\';
         buffer[pos++] = *p;
      }
      else
      {
         if (pos + 1 >= bufferSize)
            return -1;
         buffer[pos++] = *p;
      }
   }
   if (pos + 1 >= bufferSize)
      return -1;
   buffer[pos++] = '"';
   buffer[pos] = '\0';
   return static_cast<int>(pos);
}

/**
 * Build Authorization header for SIP digest authentication
 */
static bool BuildAuthorizationHeader(char *buffer, size_t bufferSize, const char *login,
   const DigestChallenge *challenge, const char *uri, const char *response,
   const char *qop, const char *nc, const char *cnonce, bool proxyAuth)
{
   int offset = snprintf(buffer, bufferSize, "%s: Digest username=",
      proxyAuth ? "Proxy-Authorization" : "Authorization");
   if (offset < 0 || static_cast<size_t>(offset) >= bufferSize)
   {
      buffer[0] = '\0';
      return false;
   }

   int qs = WriteQuotedString(buffer + offset, bufferSize - offset, login);
   if (qs < 0)
   {
      buffer[0] = '\0';
      return false;
   }
   offset += qs;

   if (static_cast<size_t>(offset) + 9 >= bufferSize)
   {
      buffer[0] = '\0';
      return false;
   }
   memcpy(buffer + offset, ", realm=", 8);
   offset += 8;

   qs = WriteQuotedString(buffer + offset, bufferSize - offset, challenge->realm);
   if (qs < 0)
   {
      buffer[0] = '\0';
      return false;
   }
   offset += qs;

   if (static_cast<size_t>(offset) + 9 >= bufferSize)
   {
      buffer[0] = '\0';
      return false;
   }
   memcpy(buffer + offset, ", nonce=", 8);
   offset += 8;

   qs = WriteQuotedString(buffer + offset, bufferSize - offset, challenge->nonce);
   if (qs < 0)
   {
      buffer[0] = '\0';
      return false;
   }
   offset += qs;

   if (static_cast<size_t>(offset) >= bufferSize)
   {
      buffer[0] = '\0';
      return false;
   }

   int n = snprintf(buffer + offset, bufferSize - offset, ", uri=\"%s\", response=\"%s\"", uri, response);
   if (n < 0 || static_cast<size_t>(n) >= bufferSize - offset)
   {
      buffer[0] = '\0';
      return false;
   }
   offset += n;

   if (challenge->algorithm[0] != '\0')
   {
      n = snprintf(buffer + offset, bufferSize - offset, ", algorithm=%s", challenge->algorithm);
      if (n < 0 || static_cast<size_t>(n) >= bufferSize - offset)
      {
         buffer[0] = '\0';
         return false;
      }
      offset += n;
   }

   if (qop != nullptr && *qop != '\0')
   {
      n = snprintf(buffer + offset, bufferSize - offset, ", qop=%s, nc=%s, cnonce=\"%s\"", qop, nc, cnonce);
      if (n < 0 || static_cast<size_t>(n) >= bufferSize - offset)
      {
         buffer[0] = '\0';
         return false;
      }
      offset += n;
   }
   else if (!stricmp(challenge->algorithm, "MD5-sess") || !stricmp(challenge->algorithm, "SHA-256-sess"))
   {
      n = snprintf(buffer + offset, bufferSize - offset, ", cnonce=\"%s\"", cnonce);
      if (n < 0 || static_cast<size_t>(n) >= bufferSize - offset)
      {
         buffer[0] = '\0';
         return false;
      }
      offset += n;
   }

   if (challenge->opaque[0] != '\0' && offset > 0 && static_cast<size_t>(offset) < bufferSize)
   {
      if (static_cast<size_t>(offset) + 10 >= bufferSize)
      {
         buffer[0] = '\0';
         return false;
      }
      memcpy(buffer + offset, ", opaque=", 9);
      offset += 9;
      int oqs = WriteQuotedString(buffer + offset, bufferSize - offset, challenge->opaque);
      if (oqs < 0)
      {
         buffer[0] = '\0';
         return false;
      }
      offset += oqs;
   }

   if (static_cast<size_t>(offset) >= bufferSize)
   {
      buffer[0] = '\0';
      return false;
   }
   return true;
}

/**
 * Receive a complete SIP response via UDP socket with timeout.
 * Returns number of bytes read, or -1 on error/timeout.
 */
static ssize_t ReceiveSIPResponseUDP(SOCKET s, char *buffer, size_t bufferSize, uint32_t timeoutMs)
{
   SocketPoller sp;
   sp.add(s);
   if (sp.poll(timeoutMs) <= 0)
      return -1;
   ssize_t bytes = RecvEx(s, buffer, bufferSize - 1, 0, 0);
   if (bytes > 0)
      buffer[bytes] = '\0';
   return bytes;
}

/**
 * State for TCP/TLS stream parsing to preserve data across calls
 * when multiple SIP messages arrive in a single recv()
 */
struct SIPStreamState
{
   char data[4096];
   size_t len;
};

/**
 * Receive a complete SIP response via TCP/TLS stream with timeout.
 * Reads until double CRLF (end of headers) and consumes any message body
 * indicated by Content-Length. Preserves excess bytes beyond the full message
 * boundary for subsequent calls.
 * Returns number of bytes read, or -1 on error/timeout.
 */
static ssize_t ReceiveSIPResponseStream(TLSConnection *conn, char *buffer, size_t bufferSize, int64_t deadline, SIPStreamState *state)
{
   size_t totalRead = 0;

   // Consume any residual data from previous call
   if (state->len > 0)
   {
      totalRead = std::min(state->len, bufferSize - 1);
      memcpy(buffer, state->data, totalRead);
      if (state->len > totalRead)
         memmove(state->data, state->data + totalRead, state->len - totalRead);
      state->len -= totalRead;
      buffer[totalRead] = '\0';
   }

   // Main receive loop: strip keepalives, search for header terminator, read more data as needed
   char *endOfMsg = nullptr;
   for (;;)
   {
      // Strip leading CRLF keepalive bytes (RFC 5626 Section 4.4.1)
      size_t skip = 0;
      while(skip + 1 < totalRead && buffer[skip] == '\r' && buffer[skip + 1] == '\n')
         skip += 2;
      if (skip > 0)
      {
         totalRead -= skip;
         memmove(buffer, buffer + skip, totalRead);
         buffer[totalRead] = '\0';
      }

      // Check if data contains end of headers
      endOfMsg = (totalRead > 0) ? strstr(buffer, "\r\n\r\n") : nullptr;
      if (endOfMsg != nullptr)
         break;

      // If buffer is full but no header terminator found, the response headers exceed the buffer size
      if (totalRead >= bufferSize - 1)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ReceiveSIPResponseStream: SIP response headers exceed %u byte buffer, dropping message"), static_cast<unsigned int>(bufferSize));
         return -1;
      }

      // Read more data from the connection
      int64_t remaining = deadline - GetMonotonicClockTime();
      if (remaining <= 0)
         return -1;

      ssize_t bytes = conn->recv(buffer + totalRead, bufferSize - 1 - totalRead, static_cast<uint32_t>(remaining));
      if (bytes <= 0)
         return -1;

      totalRead += bytes;
      buffer[totalRead] = '\0';
   }

   size_t headerLen = static_cast<size_t>(endOfMsg - buffer) + 4;

   // Determine body size from Content-Length header
   size_t bodyLen = 0;
   char clValue[32];
   if (ExtractSIPHeader(buffer, headerLen, "Content-Length", clValue, sizeof(clValue)) ||
       ExtractSIPHeader(buffer, headerLen, "l", clValue, sizeof(clValue)))
      bodyLen = static_cast<size_t>(strtoul(clValue, nullptr, 10));

   size_t fullMsgLen = headerLen + bodyLen;
   size_t cappedMsgLen = fullMsgLen;
   if (cappedMsgLen >= bufferSize)
      cappedMsgLen = bufferSize - 1;

   // Read remaining body bytes if not yet fully received
   while(totalRead < cappedMsgLen && totalRead < bufferSize - 1)
   {
      int64_t remaining = deadline - GetMonotonicClockTime();
      if (remaining <= 0)
         return -1;

      ssize_t bytes = conn->recv(buffer + totalRead, bufferSize - 1 - totalRead, static_cast<uint32_t>(remaining));
      if (bytes <= 0)
         return -1;

      totalRead += bytes;
      buffer[totalRead] = '\0';
   }

   // Drain any body bytes that didn't fit in the buffer to maintain stream framing
   if (fullMsgLen > totalRead)
   {
      size_t toDrain = fullMsgLen - totalRead;
      char drainBuf[1024];
      while(toDrain > 0)
      {
         int64_t remaining = deadline - GetMonotonicClockTime();
         if (remaining <= 0)
            return -1;

         size_t readSize = std::min(toDrain, sizeof(drainBuf));
         ssize_t bytes = conn->recv(drainBuf, readSize, static_cast<uint32_t>(remaining));
         if (bytes <= 0)
            return -1;

         toDrain -= static_cast<size_t>(bytes);
      }
   }
   else if (totalRead > fullMsgLen)
   {
      // Save any bytes beyond the full message for next call
      size_t excess = totalRead - fullMsgLen;
      size_t space = sizeof(state->data) - state->len;
      if (excess > space)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ReceiveSIPResponseStream: excess data (%u bytes) exceeds state buffer (%u bytes), stream desynchronized"),
            static_cast<unsigned int>(excess), static_cast<unsigned int>(space));
         return -1;
      }
      memcpy(state->data + state->len, buffer + fullMsgLen, excess);
      state->len += excess;
   }
   buffer[cappedMsgLen] = '\0';
   return static_cast<ssize_t>(cappedMsgLen);
}

/**
 * Limit on concurrent DNS resolution threads to prevent accumulation
 * when the resolver is slow or unreachable.
 */
static VolatileCounter s_activeDNSThreads = 0;
static const int32_t MAX_CONCURRENT_DNS_THREADS = 8;

/**
 * Context for asynchronous DNS resolution with timeout support
 */
struct DNSResolveData
{
   char host[256];
   InetAddress result;
   Condition completed;
   VolatileCounter refCount;

   DNSResolveData() : completed(false), refCount(2) {}

   void release()
   {
      if (InterlockedDecrement(&refCount) == 0)
         delete this;
   }
};

/**
 * Background thread for DNS resolution
 */
static void DNSResolveWorker(DNSResolveData *data)
{
   data->result = InetAddress::resolveHostName(data->host);
   data->completed.set();
   data->release();
   InterlockedDecrement(&s_activeDNSThreads);
}

/**
 * SIP connection context for grouping connection parameters
 */
struct SIPConnectionContext
{
   SIPTransport transport;
   SOCKET udpSocket;
   TLSConnection *tlsConn;
   SIPStreamState *streamState;
};

/**
 * Receive and validate a SIP response, handling UDP retransmission with T1->T2 backoff,
 * TLS stream receive, response parsing, CSeq/Call-ID validation, and provisional (1xx) skipping.
 * Returns SIP status code (>=100) on success, -1 on timeout.
 */
static int ReceiveAndValidateSIPResponse(
   SIPConnectionContext *conn,
   char *responseBuffer,
   size_t responseBufferSize,
   ssize_t *responseLength,
   int64_t deadline,
   const char *retransmitMsg,
   size_t retransmitMsgLen,
   int expectedCSeq,
   const char *expectedCallId,
   const char *uri,
   const char *proxy)
{
   int udpRetransmitInterval = SIP_T1;
   int64_t udpNextRetransmit = GetMonotonicClockTime() + udpRetransmitInterval;

   for(;;)
   {
      int64_t now = GetMonotonicClockTime();
      int64_t remaining = deadline - now;
      if (remaining <= 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): timeout waiting for response"), uri, proxy);
         return -1;
      }

      ssize_t bytes;
      if (conn->transport == SIP_UDP)
      {
         uint32_t pollTimeout = static_cast<uint32_t>(std::min(remaining, std::max(static_cast<int64_t>(1), udpNextRetransmit - now)));
         bytes = ReceiveSIPResponseUDP(conn->udpSocket, responseBuffer, responseBufferSize, pollTimeout);
         if (bytes <= 0)
         {
            now = GetMonotonicClockTime();
            if (now >= deadline)
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): timeout waiting for response"), uri, proxy);
               return -1;
            }
            if (now >= udpNextRetransmit)
            {
               nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): retransmitting REGISTER (interval %d ms)"), uri, proxy, udpRetransmitInterval);
               SendEx(conn->udpSocket, retransmitMsg, retransmitMsgLen, 0, nullptr);
               udpRetransmitInterval = std::min(udpRetransmitInterval * 2, SIP_T2);
               udpNextRetransmit = now + udpRetransmitInterval;
            }
            continue;
         }
      }
      else
      {
         bytes = ReceiveSIPResponseStream(conn->tlsConn, responseBuffer, responseBufferSize, deadline, conn->streamState);
         if (bytes <= 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): timeout waiting for response"), uri, proxy);
            return -1;
         }
      }

      int sipStatus = ParseSIPResponse(responseBuffer, static_cast<size_t>(bytes));
      if (sipStatus == -1)
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): received non-response SIP message, skipping"), uri, proxy);
         continue;
      }

      char cseqHeader[64];
      if (!ExtractSIPHeader(responseBuffer, static_cast<size_t>(bytes), "CSeq", cseqHeader, sizeof(cseqHeader)))
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): ignoring malformed response without CSeq header"), uri, proxy);
         continue;
      }
      int responseCSeq = static_cast<int>(strtol(cseqHeader, nullptr, 10));
      if (responseCSeq != expectedCSeq)
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): ignoring stale response with CSeq %d (expected %d)"), uri, proxy, responseCSeq, expectedCSeq);
         continue;
      }

      char responseCallId[128];
      if (!ExtractSIPHeader(responseBuffer, static_cast<size_t>(bytes), "Call-ID", responseCallId, sizeof(responseCallId)))
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): ignoring malformed response without Call-ID header"), uri, proxy);
         continue;
      }
      char *at = strchr(responseCallId, '@');
      if (at != nullptr)
         *at = '\0';
      if (strcmp(responseCallId, expectedCallId) != 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): ignoring response with mismatched Call-ID"), uri, proxy);
         continue;
      }

      nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): received response %d"), uri, proxy, sipStatus);
      if (sipStatus < 100 || sipStatus >= 200)
      {
         *responseLength = bytes;
         return sipStatus;
      }

      // RFC 3261 Section 17.1.2.2: in Proceeding state, retransmit timer must be reset to T2
      if (conn->transport == SIP_UDP)
      {
         udpRetransmitInterval = SIP_T2;
         udpNextRetransmit = GetMonotonicClockTime() + SIP_T2;
      }
      nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): skipping provisional %d response"), uri, proxy, sipStatus);
   }
}

/**
 * Do SIP client registration
 */
static int32_t RegisterSIPClient(const char *login, const char *password, const char *domain, const char *proxy, int32_t timeout, int *status)
{
   *status = 0;

   char uri[256];
   snprintf(uri, sizeof(uri), "sip:%s@%s", login, domain);
   nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): starting registration (timeout %d seconds)"), uri, proxy, timeout);

   SIPProxyAddress addr;
   if (!ParseSIPProxyURI(proxy, &addr))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): failed to parse proxy URI"), uri, proxy);
      *status = 901;
      return -1;
   }

   int64_t startTime = GetMonotonicClockTime();
   int64_t deadline = startTime + static_cast<int64_t>(timeout) * 1000;

   InetAddress proxyAddr;
   if (InterlockedIncrement(&s_activeDNSThreads) > MAX_CONCURRENT_DNS_THREADS)
   {
      InterlockedDecrement(&s_activeDNSThreads);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): too many concurrent DNS resolution threads"), uri, proxy);
      *status = 901;
      return -1;
   }

   DNSResolveData *dnsData = new DNSResolveData();
   strlcpy(dnsData->host, addr.host, sizeof(dnsData->host));

   if (ThreadCreate(DNSResolveWorker, dnsData))
   {
      int64_t remaining = deadline - GetMonotonicClockTime();
      bool resolved = (remaining > 0) && dnsData->completed.wait(static_cast<uint32_t>(remaining));
      if (resolved)
         proxyAddr = dnsData->result;
      dnsData->release();
      if (!resolved)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): deadline exceeded during DNS resolution"), uri, proxy);
         *status = 408;
         return -1;
      }
   }
   else
   {
      InterlockedDecrement(&s_activeDNSThreads);
      delete dnsData;
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): failed to create DNS resolver thread"), uri, proxy);
      *status = 901;
      return -1;
   }

   if (!proxyAddr.isValidUnicast() && !proxyAddr.isLoopback())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): failed to resolve proxy host"), uri, proxy);
      *status = 901;
      return -1;
   }

   SOCKET udpSocket = INVALID_SOCKET;
   TLSConnection *tlsConn = nullptr;

   if (addr.transport == SIP_UDP)
   {
      udpSocket = ConnectToHostUDP(proxyAddr, addr.port);
      if (udpSocket == INVALID_SOCKET)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): UDP connect failed"), uri, proxy);
         *status = 902;
         return -1;
      }
   }
   else
   {
      int64_t remaining = deadline - GetMonotonicClockTime();
      if (remaining <= 0)
      {
         *status = 408;
         return -1;
      }
      tlsConn = new TLSConnection(DEBUG_TAG);
      if (addr.transport == SIP_TLS)
         tlsConn->enablePeerVerification();
      // Use separate TCP connect and TLS handshake with individual deadline-based timeouts
      if (!tlsConn->connect(proxyAddr, addr.port, false, static_cast<uint32_t>(remaining)))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): TCP connect failed"), uri, proxy);
         *status = 902;
         delete tlsConn;
         return -1;
      }
      if (addr.transport == SIP_TLS)
      {
         remaining = deadline - GetMonotonicClockTime();
         if (remaining <= 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): deadline exceeded after TCP connect"), uri, proxy);
            *status = 408;
            delete tlsConn;
            return -1;
         }
         if (!tlsConn->startTLS(static_cast<uint32_t>(remaining), addr.host))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): TLS handshake failed"), uri, proxy);
            *status = 903;
            delete tlsConn;
            return -1;
         }
      }
   }

   // Obtain local IP and port from connected socket
   char localIp[64] = "0.0.0.0";
   uint16_t localPort = 0;
   {
      struct sockaddr_storage localAddr;
      socklen_t addrLen = sizeof(localAddr);
      SOCKET sock = (addr.transport == SIP_UDP) ? udpSocket : tlsConn->getSocket();
      if (sock != INVALID_SOCKET && getsockname(sock, reinterpret_cast<struct sockaddr*>(&localAddr), &addrLen) == 0)
      {
         if (localAddr.ss_family == AF_INET)
         {
            struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in*>(&localAddr);
            InetAddress(ntohl(sin->sin_addr.s_addr)).toStringA(localIp);
            localPort = ntohs(sin->sin_port);
         }
         else if (localAddr.ss_family == AF_INET6)
         {
            struct sockaddr_in6 *sin6 = reinterpret_cast<struct sockaddr_in6*>(&localAddr);
            InetAddress(sin6->sin6_addr.s6_addr).toStringA(localIp);
            localPort = ntohs(sin6->sin6_port);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): getsockname() failed"), uri, proxy);
         if (udpSocket != INVALID_SOCKET)
            closesocket(udpSocket);
         delete tlsConn;
         *status = 902;
         return -1;
      }
   }

   // Generate Call-ID, branch tag, from-tag using UUIDs
   char callId[48], branch[48], fromTag[48];
   uuid::generate().toStringA(callId);
   uuid::generate().toStringA(branch);
   uuid::generate().toStringA(fromTag);

   char regUri[512];
   bool isIPv6Reg = IsIPv6Address(domain);
   if (isIPv6Reg && domain[0] != '[')
      snprintf(regUri, sizeof(regUri), "sip:[%s]", domain);
   else
      snprintf(regUri, sizeof(regUri), "sip:%s", domain);

   // Build and send initial REGISTER (CSeq 1, no auth)
   char msgBuffer[4096];
   if (!BuildSIPRegister(msgBuffer, sizeof(msgBuffer), login, domain, &addr, localIp, localPort,
      callId, 1, branch, fromTag, timeout, nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("RegisterSIPClient(%hs via %hs): SIP message buffer overflow"), uri, proxy);
      *status = 500;
      if (addr.transport == SIP_UDP)
         closesocket(udpSocket);
      else
         delete tlsConn;
      return -1;
   }

   nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): sending initial REGISTER"), uri, proxy);

   ssize_t sent;
   if (addr.transport == SIP_UDP)
   {
      sent = SendEx(udpSocket, msgBuffer, strlen(msgBuffer), 0, nullptr);
   }
   else
   {
      int64_t remaining = deadline - GetMonotonicClockTime();
      if (remaining <= 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): deadline exceeded before send"), uri, proxy);
         *status = 408;
         delete tlsConn;
         return -1;
      }
      sent = tlsConn->send(msgBuffer, strlen(msgBuffer), static_cast<uint32_t>(remaining));
   }

   if (sent <= 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): send failed"), uri, proxy);
      *status = 904;
      if (addr.transport == SIP_UDP)
         closesocket(udpSocket);
      else
         delete tlsConn;
      return -1;
   }

   // Receive first response, skipping any 1xx provisional responses
   char responseBuffer[4096];
   SIPStreamState streamState;
   memset(&streamState, 0, sizeof(streamState));
   ssize_t responseLen;
   int sipStatus;
   int cseq = 1;

   SIPConnectionContext conn;
   conn.transport = addr.transport;
   conn.udpSocket = udpSocket;
   conn.tlsConn = tlsConn;
   conn.streamState = &streamState;

   sipStatus = ReceiveAndValidateSIPResponse(&conn, responseBuffer, sizeof(responseBuffer),
      &responseLen, deadline, msgBuffer, strlen(msgBuffer), cseq, callId, uri, proxy);
   if (sipStatus == -1)
   {
      *status = 408;
      if (addr.transport == SIP_UDP)
         closesocket(udpSocket);
      else
         delete tlsConn;
      return -1;
   }

   // Handle up to 3 authentication challenge rounds
   // (e.g. proxy 407 followed by registrar 401 followed by stale nonce retry)
   // Save challenge data per auth type so headers can be recomputed with correct nc values
   DigestChallenge savedChallenge[2] = {};  // [0] = WWW-Auth (401), [1] = Proxy-Auth (407)
   int ncCount[2] = { 0, 0 };
   bool challengeSaved[2] = { false, false };

   for (int authRound = 0; authRound < 3 && (sipStatus == 401 || sipStatus == 407); authRound++)
   {
      bool isProxyAuth = (sipStatus == 407);
      int slot = isProxyAuth ? 1 : 0;
      const char *challengeHeaderName = isProxyAuth ? "Proxy-Authenticate" : "WWW-Authenticate";
      char wwwAuth[1024];
      DigestChallenge challenge;
      bool digestFound = false;
      const char *headerCursor = nullptr;
      while (ExtractSIPHeader(responseBuffer, static_cast<size_t>(responseLen), challengeHeaderName, wwwAuth, sizeof(wwwAuth), &headerCursor))
      {
         if (ParseDigestChallenge(wwwAuth, &challenge) && IsSupportedDigestAlgorithm(challenge.algorithm) && HasSupportedQop(challenge.qop))
         {
            digestFound = true;
            break;
         }
      }
      if (!digestFound)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): no usable Digest challenge in %d response"), uri, proxy, sipStatus);
         *status = sipStatus;
         if (addr.transport == SIP_UDP)
            closesocket(udpSocket);
         else
            delete tlsConn;
         return -1;
      }

      nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): received %d (round %d), authenticating (realm=%hs, algorithm=%hs)"),
         uri, proxy, sipStatus, authRound + 1, challenge.realm, challenge.algorithm[0] ? challenge.algorithm : "MD5");

      // Save challenge for this auth type (new nonce resets count, same nonce increments)
      if (challengeSaved[slot] && strcmp(savedChallenge[slot].nonce, challenge.nonce) == 0)
         ncCount[slot]++;
      else
         ncCount[slot] = 1;
      savedChallenge[slot] = challenge;
      challengeSaved[slot] = true;

      // Increment nonce count for previously saved challenges being reused
      int otherSlot = 1 - slot;
      if (challengeSaved[otherSlot])
         ncCount[otherSlot]++;

      // Build authorization headers for all saved challenges with correct nc values
      char combinedAuth[4096] = "";
      int combinedOffset = 0;
      for (int i = 0; i < 2; i++)
      {
         if (!challengeSaved[i])
            continue;

         char nc[16];
         snprintf(nc, sizeof(nc), "%08x", ncCount[i]);
         char cnonce[48];
         uuid::generate().toStringA(cnonce);

         const char *qop = (savedChallenge[i].qop[0] != '\0') ? "auth" : nullptr;

         char digestResponse[128];
         ComputeDigestResponse(login, savedChallenge[i].realm, password, savedChallenge[i].nonce,
            "REGISTER", regUri, nc, cnonce, qop, savedChallenge[i].algorithm, digestResponse, sizeof(digestResponse));

         char header[2048];
         if (!BuildAuthorizationHeader(header, sizeof(header), login, &savedChallenge[i], regUri, digestResponse, qop, nc, cnonce, i == 1))
            continue;

         int cn;
         if (combinedOffset > 0)
            cn = snprintf(combinedAuth + combinedOffset, sizeof(combinedAuth) - combinedOffset, "\r\n%s", header);
         else
            cn = snprintf(combinedAuth + combinedOffset, sizeof(combinedAuth) - combinedOffset, "%s", header);
         if (cn < 0 || static_cast<size_t>(cn) >= sizeof(combinedAuth) - combinedOffset)
            break;
         combinedOffset += cn;
      }

      // New branch for the authenticated request
      cseq++;
      char newBranch[48];
      uuid::generate().toStringA(newBranch);

      if (!BuildSIPRegister(msgBuffer, sizeof(msgBuffer), login, domain, &addr, localIp, localPort,
         callId, cseq, newBranch, fromTag, timeout, combinedOffset > 0 ? combinedAuth : nullptr))
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("RegisterSIPClient(%hs via %hs): SIP message buffer overflow during auth"), uri, proxy);
         *status = 500;
         if (addr.transport == SIP_UDP)
            closesocket(udpSocket);
         else
            delete tlsConn;
         return -1;
      }

      nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): sending authenticated REGISTER (round %d)"), uri, proxy, authRound + 1);

      if (addr.transport == SIP_UDP)
      {
         sent = SendEx(udpSocket, msgBuffer, strlen(msgBuffer), 0, nullptr);
      }
      else
      {
         int64_t remaining = deadline - GetMonotonicClockTime();
         if (remaining <= 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): deadline exceeded before authenticated send"), uri, proxy);
            *status = 408;
            delete tlsConn;
            return -1;
         }
         sent = tlsConn->send(msgBuffer, strlen(msgBuffer), static_cast<uint32_t>(remaining));
      }

      if (sent <= 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): send of authenticated REGISTER failed"), uri, proxy);
         *status = 904;
         if (addr.transport == SIP_UDP)
            closesocket(udpSocket);
         else
            delete tlsConn;
         return -1;
      }

      sipStatus = ReceiveAndValidateSIPResponse(&conn, responseBuffer, sizeof(responseBuffer),
         &responseLen, deadline, msgBuffer, strlen(msgBuffer), cseq, callId, uri, proxy);
      if (sipStatus == -1)
      {
         *status = 408;
         if (addr.transport == SIP_UDP)
            closesocket(udpSocket);
         else
            delete tlsConn;
         return -1;
      }
   }

   // Clean up connection
   if (addr.transport == SIP_UDP)
      closesocket(udpSocket);
   else
      delete tlsConn;

   *status = (sipStatus >= 100) ? sipStatus : 400;
   if (sipStatus == 200)
   {
      int32_t elapsedTime = static_cast<int32_t>(GetMonotonicClockTime() - startTime);
      nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): registration successful (elapsed %d ms)"), uri, proxy, elapsedTime);
      return elapsedTime;
   }

   nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): registration failed with status %d"), uri, proxy, sipStatus);
   return -1;
}

/**
 * Handler for Asterisk.SIP.TestRegistration parameter
 */
LONG H_SIPTestRegistration(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(3);

   char login[128], password[128], domain[128];
   GET_ARGUMENT_A(1, login, 128);
   GET_ARGUMENT_A(2, password, 128);
   GET_ARGUMENT_A(3, domain, 128);

   int status;
   char proxy[256], ipStr[64];
   sys->getIpAddress().toStringA(ipStr);
   snprintf(proxy, sizeof(proxy), (sys->getIpAddress().getFamily() == AF_INET6) ? "sip:[%s]" : "sip:%s", ipStr);
   ret_int64(value, RegisterSIPClient(login, password, domain, proxy, 5, &status));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Registration test constructor
 */
SIPRegistrationTest::SIPRegistrationTest(ConfigEntry *config, const char *defaultProxy) : m_mutex(MutexType::FAST)
{
   m_name = MemCopyString(config->getName());
#ifdef UNICODE
   m_login = UTF8StringFromWideString(config->getSubEntryValue(L"Login", 0, L"netxms"));
   m_password = UTF8StringFromWideString(config->getSubEntryValue(L"Password", 0, L"netxms"));
   m_domain = UTF8StringFromWideString(config->getSubEntryValue(L"Domain", 0, L""));
   const WCHAR *proxy = config->getSubEntryValue(L"Proxy", 0, nullptr);
   if (proxy != nullptr)
   {
      if (!wcsnicmp(proxy, L"sip:", 4) || !wcsnicmp(proxy, L"sips:", 5))
      {
         m_proxy = UTF8StringFromWideString(proxy);
      }
      else
      {
         // Check for bare IPv6 address (has multiple colons and no brackets)
         const WCHAR *firstColon = wcschr(proxy, L':');
         if (firstColon != nullptr && wcschr(firstColon + 1, L':') != nullptr && proxy[0] != L'[')
         {
            size_t utf8len = wchar_utf8len(proxy, -1);
            m_proxy = MemAllocStringA(utf8len + 7);  // "sip:[" + "]" + null
            memcpy(m_proxy, "sip:[", 5);
            wchar_to_utf8(proxy, -1, &m_proxy[5], utf8len + 1);
            strcat(m_proxy, "]");
         }
         else
         {
            size_t len = wchar_utf8len(proxy, -1) + 5;
            m_proxy = MemAllocStringA(len);
            memcpy(m_proxy, "sip:", 4);
            wchar_to_utf8(proxy, -1, &m_proxy[4], len - 4);
         }
      }
   }
   else
   {
      m_proxy = MemCopyStringA(defaultProxy);
   }
#else
   m_login = MemCopyStringA(config->getSubEntryValue("Login", 0, "netxms"));
   m_password = MemCopyStringA(config->getSubEntryValue("Password", 0, "netxms"));
   m_domain = MemCopyStringA(config->getSubEntryValue("Domain", 0, ""));
   const char *proxy = config->getSubEntryValue("Proxy", 0, nullptr);
   if (proxy != nullptr)
   {
      if (!strnicmp(proxy, "sip:", 4) || !strnicmp(proxy, "sips:", 5))
      {
         m_proxy = MemCopyStringA(proxy);
      }
      else
      {
         // Check for bare IPv6 address (has multiple colons and no brackets)
         const char *firstColon = strchr(proxy, ':');
         if (firstColon != nullptr && strchr(firstColon + 1, ':') != nullptr && proxy[0] != '[')
         {
            m_proxy = MemAllocStringA(strlen(proxy) + 7);  // "sip:[" + "]" + null
            memcpy(m_proxy, "sip:[", 5);
            strcpy(&m_proxy[5], proxy);
            strcat(m_proxy, "]");
         }
         else
         {
            m_proxy = MemAllocStringA(strlen(proxy) + 5);
            memcpy(m_proxy, "sip:", 4);
            strcpy(&m_proxy[4], proxy);
         }
      }
   }
   else
   {
      m_proxy = MemCopyStringA(defaultProxy);
   }
#endif
   m_timeout = config->getSubEntryValueAsUInt(_T("Timeout"), 0, 30);
   m_interval = config->getSubEntryValueAsUInt(_T("Interval"), 0, 300) * 1000;
   m_lastRunTime = 0;
   m_elapsedTime = 0;
   m_status = 500;
   m_stop = false;
}

/**
 * Registration test destructor
 */
SIPRegistrationTest::~SIPRegistrationTest()
{
   MemFree(m_domain);
   MemFree(m_login);
   MemFree(m_name);
   MemFree(m_password);
   MemFree(m_proxy);
}

/**
 * Run test
 */
void SIPRegistrationTest::run()
{
   if (m_stop)
      return;

   int status;
   int32_t elapsed = RegisterSIPClient(m_login, m_password, m_domain, m_proxy, m_timeout, &status);
   m_mutex.lock();
   m_lastRunTime = time(nullptr);
   m_elapsedTime = elapsed;
   m_status = status;
   m_mutex.unlock();
   nxlog_debug_tag(DEBUG_TAG, 7, _T("SIP registration test for %hs@%hs via %hs completed (status=%03d elapsed=%d)"),
            m_login, m_domain, m_proxy, status, elapsed);

   // Re-schedule
   if (!m_stop)
      ThreadPoolScheduleRelative(g_asteriskThreadPool, m_interval, this, &SIPRegistrationTest::run);
}

/**
 * Start tests
 */
void SIPRegistrationTest::start()
{
   // First run in 5 seconds, then repeat with configured intervals
   ThreadPoolScheduleRelative(g_asteriskThreadPool, 5000, this, &SIPRegistrationTest::run);
}

/**
 * Stop tests
 */
void SIPRegistrationTest::stop()
{
   m_stop = true;
}

/**
 * Handler for Asterisk.SIP.RegistrationTests list
 */
LONG H_SIPRegistrationTestList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);
   ObjectArray<SIPRegistrationTest> *tests = sys->getRegistrationTests();
   for(int i = 0; i < tests->size(); i++)
      value->add(tests->get(i)->getName());
   delete tests;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Asterisk.SIP.RegistrationTests table
 */
LONG H_SIPRegistrationTestTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);

   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("LOGIN"), DCI_DT_STRING, _T("Login"));
   value->addColumn(_T("DOMAIN"), DCI_DT_STRING, _T("Domain"));
   value->addColumn(_T("PROXY"), DCI_DT_STRING, _T("Proxy"));
   value->addColumn(_T("INTERVAL"), DCI_DT_INT, _T("Proxy"));
   value->addColumn(_T("STATUS"), DCI_DT_INT, _T("Status"));
   value->addColumn(_T("STATUS_TEXT"), DCI_DT_STRING, _T("Status Text"));
   value->addColumn(_T("ELAPSED_TIME"), DCI_DT_INT, _T("Elapsed Time"));
   value->addColumn(_T("TIMESTAMP"), DCI_DT_INT64, _T("Timestamp"));

   ObjectArray<SIPRegistrationTest> *tests = sys->getRegistrationTests();
   for(int i = 0; i < tests->size(); i++)
   {
      SIPRegistrationTest *t = tests->get(i);
      value->addRow();
      value->set(0, t->getName());
      value->set(1, t->getLogin());
      value->set(2, t->getDomain());
      value->set(3, t->getProxy());
      value->set(4, t->getInterval());
      value->set(5, t->getStatus());
      value->set(6, CodeToText(t->getStatus(), s_sipStatusCodes));
      value->set(7, t->getElapsedTime());
      value->set(8, static_cast<INT64>(t->getLastRunTime()));
   }
   delete tests;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Asterisk.SIP.RegistrationTest.* parameters
 */
LONG H_SIPRegistrationTestData(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(1);

   TCHAR name[128];
   GET_ARGUMENT(1, name, 128);

   SIPRegistrationTest *test = sys->getRegistartionTest(name);
   if (test == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   if (test->getLastRunTime() == 0)
      return SYSINFO_RC_ERROR;   // Data not collected yet

   switch(*arg)
   {
      case 'E':
         ret_int(value, test->getElapsedTime());
         break;
      case 'S':
         ret_int(value, test->getStatus());
         break;
      case 's':
         ret_string(value, CodeToText(test->getStatus(), s_sipStatusCodes));
         break;
      case 'T':
         ret_int64(value, test->getLastRunTime());
         break;
   }

   return SYSINFO_RC_SUCCESS;
}
