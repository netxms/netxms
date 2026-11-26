/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: firmware.cpp
**/

#include "libnxsrv.h"
#include <nddrv.h>

/**
 * Get symbolic name of firmware role
 */
const wchar_t LIBNXSRV_EXPORTABLE *FirmwareRoleName(FirmwareRole role)
{
   switch(role)
   {
      case FirmwareRole::MAIN:
         return L"main";
      case FirmwareRole::PATCH:
         return L"patch";
      case FirmwareRole::BOOTLOADER:
         return L"bootloader";
      case FirmwareRole::MGMT_CONTROLLER:
         return L"management-controller";
      default:
         return L"other";
   }
}

/**
 * Built-in firmware platform registry
 */
static const FirmwarePlatform s_platforms[] =
{
   { L"aos-s", L"hpe", L"ArubaOS-Switch", FirmwareVersionScheme::INTDOT },
   { L"aoscx", L"hpe", L"AOS-CX", FirmwareVersionScheme::INTDOT },
   { L"asa", L"cisco", L"ASA", FirmwareVersionScheme::EXACT },
   { L"cip-device", L"", L"EtherNet/IP device", FirmwareVersionScheme::INTDOT },
   { L"ilo4", L"hpe", L"iLO 4", FirmwareVersionScheme::INTDOT },
   { L"ilo5", L"hpe", L"iLO 5", FirmwareVersionScheme::INTDOT },
   { L"ilo6", L"hpe", L"iLO 6", FirmwareVersionScheme::INTDOT },
   { L"ilo7", L"hpe", L"iLO 7", FirmwareVersionScheme::INTDOT },
   { L"ios", L"cisco", L"IOS", FirmwareVersionScheme::EXACT },
   { L"ios-xe", L"cisco", L"IOS XE", FirmwareVersionScheme::EXACT },
   { L"junos", L"juniper", L"Junos OS", FirmwareVersionScheme::JUNOS },
   { L"junos-evo", L"juniper", L"Junos OS Evolved", FirmwareVersionScheme::JUNOS },
   { L"nx-os", L"cisco", L"NX-OS", FirmwareVersionScheme::EXACT },
   { L"routerboot", L"mikrotik", L"RouterBOOT", FirmwareVersionScheme::ROUTEROS },
   { L"routeros", L"mikrotik", L"RouterOS", FirmwareVersionScheme::ROUTEROS }
};

/**
 * Find firmware platform in built-in registry. Returns nullptr if platform is not known.
 */
const FirmwarePlatform LIBNXSRV_EXPORTABLE *FindFirmwarePlatform(const wchar_t *id)
{
   for (size_t i = 0; i < sizeof(s_platforms) / sizeof(FirmwarePlatform); i++)
      if (!wcscmp(s_platforms[i].id, id))
         return &s_platforms[i];
   return nullptr;
}

/**
 * Version scheme names
 */
static const struct
{
   const wchar_t *name;
   FirmwareVersionScheme scheme;
} s_schemeNames[] =
{
   { L"exact", FirmwareVersionScheme::EXACT },
   { L"intdot", FirmwareVersionScheme::INTDOT },
   { L"junos", FirmwareVersionScheme::JUNOS },
   { L"routeros", FirmwareVersionScheme::ROUTEROS },
   { L"semver", FirmwareVersionScheme::SEMVER }
};

/**
 * Get version scheme by name. Returns false if scheme is not known.
 */
bool LIBNXSRV_EXPORTABLE ParseFirmwareVersionSchemeName(const wchar_t *name, FirmwareVersionScheme *scheme)
{
   for (size_t i = 0; i < sizeof(s_schemeNames) / sizeof(s_schemeNames[0]); i++)
   {
      if (!wcscmp(s_schemeNames[i].name, name))
      {
         *scheme = s_schemeNames[i].scheme;
         return true;
      }
   }
   return false;
}

/**
 * Check if character is ASCII decimal digit (locale independent)
 */
static inline bool IsDigit(wchar_t ch)
{
   return (ch >= L'0') && (ch <= L'9');
}

/**
 * Check if character is ASCII letter (locale independent)
 */
static inline bool IsLetter(wchar_t ch)
{
   return ((ch >= L'A') && (ch <= L'Z')) || ((ch >= L'a') && (ch <= L'z'));
}

/**
 * Check if character is ASCII whitespace
 */
static inline bool IsSpace(wchar_t ch)
{
   return (ch == L' ') || (ch == L'\t') || (ch == L'\r') || (ch == L'\n');
}

/**
 * Convert comparison result in strcmp() form to version order
 */
static inline FirmwareVersionOrder ToVersionOrder(int rc)
{
   return (rc < 0) ? FirmwareVersionOrder::LESS : ((rc > 0) ? FirmwareVersionOrder::GREATER : FirmwareVersionOrder::EQUAL);
}

/**
 * Compare two runs of decimal digits as numbers. Works on digits, so value is not limited by size of any
 * integer type. Empty run is equal to zero.
 */
static int CompareNumbers(const wchar_t *a, size_t alen, const wchar_t *b, size_t blen)
{
   while ((alen > 0) && (*a == L'0'))
   {
      a++;
      alen--;
   }
   while ((blen > 0) && (*b == L'0'))
   {
      b++;
      blen--;
   }
   if (alen != blen)
      return (alen < blen) ? -1 : 1;
   for (size_t i = 0; i < alen; i++)
      if (a[i] != b[i])
         return (a[i] < b[i]) ? -1 : 1;
   return 0;
}

/**
 * Get length of longest prefix of given string that is dot-separated list of integers (\d+(\.\d+)*).
 * Returns 0 if string does not start with digit.
 */
static size_t ScanIntDot(const wchar_t *s)
{
   if (!IsDigit(*s))
      return 0;

   const wchar_t *p = s;
   while (true)
   {
      while (IsDigit(*p))
         p++;
      if ((*p != L'.') || !IsDigit(p[1]))
         break;
      p++;
   }
   return p - s;
}

/**
 * Compare two dot-separated lists of integers. Missing trailing components count as zero.
 */
static int CompareIntDot(const wchar_t *a, size_t alen, const wchar_t *b, size_t blen)
{
   const wchar_t *aend = a + alen, *bend = b + blen;
   while ((a < aend) || (b < bend))
   {
      const wchar_t *anext = a;
      while ((anext < aend) && (*anext != L'.'))
         anext++;
      const wchar_t *bnext = b;
      while ((bnext < bend) && (*bnext != L'.'))
         bnext++;

      int rc = CompareNumbers(a, anext - a, b, bnext - b);
      if (rc != 0)
         return rc;

      a = (anext < aend) ? anext + 1 : aend;
      b = (bnext < bend) ? bnext + 1 : bend;
   }
   return 0;
}

/**
 * Read unsigned integer of up to 9 digits and advance pointer
 */
static bool ReadNumber(const wchar_t **p, uint32_t *value)
{
   const wchar_t *s = *p;
   uint32_t n = 0;
   int digits = 0;
   while (IsDigit(*s))
   {
      if (++digits > 9)
         return false;
      n = n * 10 + static_cast<uint32_t>(*s - L'0');
      s++;
   }
   if (digits == 0)
      return false;
   *value = n;
   *p = s;
   return true;
}

/**
 * Parsed Junos version
 */
struct JunosVersion
{
   uint32_t major;
   uint32_t minor;
   wchar_t type;     // release type letter (R, B, X, F, ...)
   uint32_t number;  // number after release type letter
   uint32_t service; // -S<n>
   uint32_t drop;    // -D<n>
   uint32_t respin;
};

/**
 * Parse Junos version in canonical form: <maj>.<min><T><n>[-S<s>|-D<d>][.<respin>]
 */
static bool ParseJunosVersion(const wchar_t *s, JunosVersion *v)
{
   memset(v, 0, sizeof(JunosVersion));

   const wchar_t *p = s;
   if (!ReadNumber(&p, &v->major) || (*p != L'.'))
      return false;
   p++;
   if (!ReadNumber(&p, &v->minor) || (*p < L'A') || (*p > L'Z'))
      return false;
   v->type = *p++;
   if (!ReadNumber(&p, &v->number))
      return false;

   if (*p == L'-')
   {
      p++;
      uint32_t *target;
      if (*p == L'S')
         target = &v->service;
      else if (*p == L'D')
         target = &v->drop;
      else
         return false;
      p++;
      if (!ReadNumber(&p, target))
         return false;
   }

   if (*p == L'.')
   {
      p++;
      if (!ReadNumber(&p, &v->respin))
         return false;
   }

   return *p == 0;
}

/**
 * Compare Junos versions. Versions with different major.minor are ordered by major.minor regardless of train.
 * Within same major.minor versions are ordered only within same train: R and B releases form train "R"
 * (B before R), each X<n> is separate train ordered by -D<n>, any other release type is train by itself.
 */
static FirmwareVersionOrder CompareJunosVersions(const JunosVersion& a, const JunosVersion& b)
{
   if (a.major != b.major)
      return (a.major < b.major) ? FirmwareVersionOrder::LESS : FirmwareVersionOrder::GREATER;
   if (a.minor != b.minor)
      return (a.minor < b.minor) ? FirmwareVersionOrder::LESS : FirmwareVersionOrder::GREATER;

   wchar_t trainA = (a.type == L'B') ? L'R' : a.type;
   wchar_t trainB = (b.type == L'B') ? L'R' : b.type;
   if ((trainA != trainB) || ((trainA == L'X') && (a.number != b.number)))
      return FirmwareVersionOrder::INCOMPARABLE;

   uint32_t keyA[4], keyB[4];
   if (trainA == L'X')
   {
      keyA[0] = 0;
      keyA[1] = 0;
      keyA[2] = a.drop;
      keyB[0] = 0;
      keyB[1] = 0;
      keyB[2] = b.drop;
   }
   else
   {
      keyA[0] = (a.type == L'B') ? 0 : 1;
      keyA[1] = a.number;
      keyA[2] = a.service;
      keyB[0] = (b.type == L'B') ? 0 : 1;
      keyB[1] = b.number;
      keyB[2] = b.service;
   }
   keyA[3] = a.respin;
   keyB[3] = b.respin;

   for (int i = 0; i < 4; i++)
      if (keyA[i] != keyB[i])
         return (keyA[i] < keyB[i]) ? FirmwareVersionOrder::LESS : FirmwareVersionOrder::GREATER;
   return FirmwareVersionOrder::EQUAL;
}

/**
 * Parsed RouterOS version (pointers into original string)
 */
struct RouterOSVersion
{
   size_t numericLength;         // length of dot-separated numeric part
   int stage;                    // 0 = beta, 1 = rc, 2 = release
   const wchar_t *stageNumber;   // number after "beta" or "rc"
   size_t stageNumberLength;
};

/**
 * Parse RouterOS version in canonical form: \d+(\.\d+)*((beta|rc)\d+)?
 */
static bool ParseRouterOSVersion(const wchar_t *s, RouterOSVersion *v)
{
   v->numericLength = ScanIntDot(s);
   if (v->numericLength == 0)
      return false;

   const wchar_t *p = s + v->numericLength;
   if (*p == 0)
   {
      v->stage = 2;
      v->stageNumber = p;
      v->stageNumberLength = 0;
      return true;
   }

   if (!wcsncmp(p, L"beta", 4))
   {
      v->stage = 0;
      p += 4;
   }
   else if (!wcsncmp(p, L"rc", 2))
   {
      v->stage = 1;
      p += 2;
   }
   else
   {
      return false;
   }

   v->stageNumber = p;
   while (IsDigit(*p))
      p++;
   v->stageNumberLength = p - v->stageNumber;
   return (v->stageNumberLength > 0) && (*p == 0);
}

/**
 * Parsed semantic version (pointers into original string)
 */
struct SemanticVersion
{
   const wchar_t *core[3];       // major, minor, patch
   size_t coreLength[3];
   const wchar_t *preRelease;    // pre-release identifiers without leading hyphen, nullptr if absent
   size_t preReleaseLength;
};

/**
 * Check list of dot-separated semantic version identifiers ([0-9A-Za-z-]+). Numeric identifiers
 * must not have leading zeros when checkLeadingZeros is set (pre-release identifiers).
 */
static bool ValidateSemVerIdentifiers(const wchar_t *s, size_t length, bool checkLeadingZeros)
{
   const wchar_t *end = s + length;
   while (true)
   {
      const wchar_t *start = s;
      bool numeric = true;
      while ((s < end) && (*s != L'.'))
      {
         if (!IsDigit(*s))
         {
            if (!IsLetter(*s) && (*s != L'-'))
               return false;
            numeric = false;
         }
         s++;
      }
      if (s == start)
         return false;  // empty identifier
      if (checkLeadingZeros && numeric && (s - start > 1) && (*start == L'0'))
         return false;
      if (s == end)
         return true;
      s++;  // skip dot
      if (s == end)
         return false;  // trailing dot
   }
}

/**
 * Parse semantic version (Semantic Versioning 2.0.0)
 */
static bool ParseSemanticVersion(const wchar_t *s, SemanticVersion *v)
{
   const wchar_t *p = s;
   for (int i = 0; i < 3; i++)
   {
      v->core[i] = p;
      while (IsDigit(*p))
         p++;
      v->coreLength[i] = p - v->core[i];
      if ((v->coreLength[i] == 0) || ((v->coreLength[i] > 1) && (*v->core[i] == L'0')))
         return false;
      if (i < 2)
      {
         if (*p != L'.')
            return false;
         p++;
      }
   }

   v->preRelease = nullptr;
   v->preReleaseLength = 0;
   if (*p == L'-')
   {
      p++;
      v->preRelease = p;
      while ((*p != 0) && (*p != L'+'))
         p++;
      v->preReleaseLength = p - v->preRelease;
      if (!ValidateSemVerIdentifiers(v->preRelease, v->preReleaseLength, true))
         return false;
   }

   if (*p == L'+')
   {
      p++;
      return ValidateSemVerIdentifiers(p, wcslen(p), false);
   }
   return *p == 0;
}

/**
 * Compare pre-release parts of two semantic versions
 */
static int CompareSemVerPreRelease(const SemanticVersion& a, const SemanticVersion& b)
{
   if (a.preRelease == nullptr)
      return (b.preRelease == nullptr) ? 0 : 1;
   if (b.preRelease == nullptr)
      return -1;

   const wchar_t *pa = a.preRelease, *enda = pa + a.preReleaseLength;
   const wchar_t *pb = b.preRelease, *endb = pb + b.preReleaseLength;
   while (true)
   {
      const wchar_t *starta = pa;
      bool numerica = true;
      while ((pa < enda) && (*pa != L'.'))
      {
         if (!IsDigit(*pa))
            numerica = false;
         pa++;
      }
      const wchar_t *startb = pb;
      bool numericb = true;
      while ((pb < endb) && (*pb != L'.'))
      {
         if (!IsDigit(*pb))
            numericb = false;
         pb++;
      }

      size_t lena = pa - starta, lenb = pb - startb;
      int rc;
      if (numerica && numericb)
      {
         rc = CompareNumbers(starta, lena, startb, lenb);
      }
      else if (numerica != numericb)
      {
         rc = numerica ? -1 : 1;  // numeric identifiers have lower precedence
      }
      else
      {
         rc = wcsncmp(starta, startb, std::min(lena, lenb));
         if ((rc == 0) && (lena != lenb))
            rc = (lena < lenb) ? -1 : 1;
      }
      if (rc != 0)
         return rc;

      if ((pa == enda) || (pb == endb))
         return (pa == enda) ? ((pb == endb) ? 0 : -1) : 1;  // larger set of identifiers has higher precedence
      pa++;
      pb++;
   }
}

/**
 * Check if string is valid version in "exact" scheme (not empty, no surrounding whitespace)
 */
static inline bool IsValidExactVersion(const wchar_t *s)
{
   return (*s != 0) && !IsSpace(*s) && !IsSpace(s[wcslen(s) - 1]);
}

/**
 * Check if version string is in canonical form of given scheme
 */
bool LIBNXSRV_EXPORTABLE IsValidFirmwareVersion(FirmwareVersionScheme scheme, const wchar_t *version)
{
   switch(scheme)
   {
      case FirmwareVersionScheme::EXACT:
         return IsValidExactVersion(version);
      case FirmwareVersionScheme::INTDOT:
         return (*version != 0) && (ScanIntDot(version) == wcslen(version));
      case FirmwareVersionScheme::SEMVER:
      {
         SemanticVersion v;
         return ParseSemanticVersion(version, &v);
      }
      case FirmwareVersionScheme::JUNOS:
      {
         JunosVersion v;
         return ParseJunosVersion(version, &v);
      }
      case FirmwareVersionScheme::ROUTEROS:
      {
         RouterOSVersion v;
         return ParseRouterOSVersion(version, &v);
      }
   }
   return false;
}

/**
 * Compare two versions in canonical form of given scheme. Versions are incomparable if scheme does not define
 * order for them or if any of them is not in canonical form.
 */
FirmwareVersionOrder LIBNXSRV_EXPORTABLE CompareFirmwareVersions(FirmwareVersionScheme scheme, const wchar_t *a, const wchar_t *b)
{
   switch(scheme)
   {
      case FirmwareVersionScheme::EXACT:
         return (IsValidExactVersion(a) && !wcscmp(a, b)) ? FirmwareVersionOrder::EQUAL : FirmwareVersionOrder::INCOMPARABLE;
      case FirmwareVersionScheme::INTDOT:
      {
         size_t alen = wcslen(a), blen = wcslen(b);
         if ((alen == 0) || (blen == 0) || (ScanIntDot(a) != alen) || (ScanIntDot(b) != blen))
            return FirmwareVersionOrder::INCOMPARABLE;
         return ToVersionOrder(CompareIntDot(a, alen, b, blen));
      }
      case FirmwareVersionScheme::SEMVER:
      {
         SemanticVersion va, vb;
         if (!ParseSemanticVersion(a, &va) || !ParseSemanticVersion(b, &vb))
            return FirmwareVersionOrder::INCOMPARABLE;
         for (int i = 0; i < 3; i++)
         {
            int rc = CompareNumbers(va.core[i], va.coreLength[i], vb.core[i], vb.coreLength[i]);
            if (rc != 0)
               return ToVersionOrder(rc);
         }
         return ToVersionOrder(CompareSemVerPreRelease(va, vb));
      }
      case FirmwareVersionScheme::JUNOS:
      {
         JunosVersion va, vb;
         if (!ParseJunosVersion(a, &va) || !ParseJunosVersion(b, &vb))
            return FirmwareVersionOrder::INCOMPARABLE;
         return CompareJunosVersions(va, vb);
      }
      case FirmwareVersionScheme::ROUTEROS:
      {
         RouterOSVersion va, vb;
         if (!ParseRouterOSVersion(a, &va) || !ParseRouterOSVersion(b, &vb))
            return FirmwareVersionOrder::INCOMPARABLE;
         int rc = CompareIntDot(a, va.numericLength, b, vb.numericLength);
         if (rc == 0)
            rc = va.stage - vb.stage;
         if (rc == 0)
            rc = CompareNumbers(va.stageNumber, va.stageNumberLength, vb.stageNumber, vb.stageNumberLength);
         return ToVersionOrder(rc);
      }
   }
   return FirmwareVersionOrder::INCOMPARABLE;
}

/**
 * Remove leading and trailing whitespace from character range
 */
static void TrimRange(const wchar_t **start, size_t *length)
{
   const wchar_t *s = *start;
   size_t l = *length;
   while ((l > 0) && IsSpace(*s))
   {
      s++;
      l--;
   }
   while ((l > 0) && IsSpace(s[l - 1]))
      l--;
   *start = s;
   *length = l;
}

/**
 * Store character range as normalized version if it fits into the buffer and is in canonical form of given scheme.
 * On failure output buffer contains empty string.
 */
static bool StoreVersion(FirmwareVersionScheme scheme, const wchar_t *start, size_t length, wchar_t *version, size_t size)
{
   if ((length > 0) && (length < size))
   {
      memcpy(version, start, length * sizeof(wchar_t));
      version[length] = 0;
      if (IsValidFirmwareVersion(scheme, version))
         return true;
   }
   version[0] = 0;
   return false;
}

/**
 * Normalize version for "exact" scheme: surrounding whitespace is removed. Returns false if version
 * is empty or does not fit into the buffer.
 */
bool LIBNXSRV_EXPORTABLE NormalizeExactFirmwareVersion(const wchar_t *raw, wchar_t *version, size_t size)
{
   size_t length = wcslen(raw);
   TrimRange(&raw, &length);
   return StoreVersion(FirmwareVersionScheme::EXACT, raw, length, version, size);
}

/**
 * Normalize version for "intdot" scheme. Removes surrounding whitespace and leading letters with optional
 * dot after them ("V2.9.4", image prefix in "FL.10.13.1080" or "KB.16.11.0021"). Returns false if the rest
 * is not dot-separated list of integers or does not fit into the buffer.
 */
bool LIBNXSRV_EXPORTABLE NormalizeIntDotFirmwareVersion(const wchar_t *raw, wchar_t *version, size_t size)
{
   size_t length = wcslen(raw);
   TrimRange(&raw, &length);
   if ((length > 0) && IsLetter(*raw))
   {
      while ((length > 0) && IsLetter(*raw))
      {
         raw++;
         length--;
      }
      if ((length > 0) && (*raw == L'.'))
      {
         raw++;
         length--;
      }
   }
   return StoreVersion(FirmwareVersionScheme::INTDOT, raw, length, version, size);
}

/**
 * Normalize Junos version. Input is either version string or sysDescr value ("... kernel JUNOS 21.4R3-S5.4,
 * Build date: ..."). Suffix "-EVO" is removed and reported through "evolved" (platform is Junos OS Evolved).
 * Returns false if version is not in canonical Junos form or does not fit into the buffer.
 */
bool LIBNXSRV_EXPORTABLE NormalizeJunosFirmwareVersion(const wchar_t *raw, wchar_t *version, size_t size, bool *evolved)
{
   *evolved = false;

   const wchar_t *start = raw;
   size_t length;
   const wchar_t *marker = wcsstr(raw, L"JUNOS ");
   if (marker != nullptr)
   {
      start = marker + 6;
      while (IsSpace(*start))
         start++;
      const wchar_t *end = start;
      while ((*end != 0) && !IsSpace(*end) && (*end != L','))
         end++;
      length = end - start;
   }
   else
   {
      length = wcslen(raw);
      TrimRange(&start, &length);
   }

   if ((length > 4) && !wcsncmp(&start[length - 4], L"-EVO", 4))
   {
      *evolved = true;
      length -= 4;
   }

   if (StoreVersion(FirmwareVersionScheme::JUNOS, start, length, version, size))
      return true;
   *evolved = false;
   return false;
}

/**
 * Normalize RouterOS or RouterBOOT version. Removes surrounding whitespace and release channel in parentheses
 * ("7.16.2 (stable)"), converts to lowercase. Returns false if the rest is not in canonical RouterOS form
 * or does not fit into the buffer.
 */
bool LIBNXSRV_EXPORTABLE NormalizeRouterOSFirmwareVersion(const wchar_t *raw, wchar_t *version, size_t size)
{
   size_t length = wcslen(raw);
   TrimRange(&raw, &length);
   if ((length > 0) && (raw[length - 1] == L')'))
   {
      size_t i = length - 1;
      while ((i > 0) && (raw[i] != L'('))
         i--;
      if (raw[i] == L'(')
      {
         length = i;
         TrimRange(&raw, &length);
      }
   }

   if ((length == 0) || (length >= size))
   {
      version[0] = 0;
      return false;
   }

   for (size_t i = 0; i < length; i++)
      version[i] = ((raw[i] >= L'A') && (raw[i] <= L'Z')) ? raw[i] + (L'a' - L'A') : raw[i];
   version[length] = 0;
   if (IsValidFirmwareVersion(FirmwareVersionScheme::ROUTEROS, version))
      return true;
   version[0] = 0;
   return false;
}
