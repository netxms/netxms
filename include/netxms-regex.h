/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: netxms-regex.h
**
**/

#ifndef _netxms_regex_h
#define _netxms_regex_h

#if USE_PCRE2

#define PCRE2_CODE_UNIT_WIDTH 8

#if HAVE_PCRE2_H || defined(_WIN32)
#include <pcre2.h>
#elif HAVE_PCRE2_PCRE2_H
#include <pcre2/pcre2.h>
#endif

// PCRE2 type aliases for compatibility
typedef pcre2_code_8 pcre;
typedef pcre2_match_data_8 pcre_extra;

// Additional PCRE2 width definitions for Unicode support
#ifdef UNICODE_UCS2
typedef pcre2_code_16 pcre16;
typedef pcre2_match_data_16 pcre16_extra;
#elif defined(UNICODE_UCS4)
typedef pcre2_code_32 pcre32;
typedef pcre2_match_data_32 pcre32_extra;
#endif

#ifdef UNICODE_UCS2
#define PCRE_WCHAR              PCRE2_UCHAR16
#define PCREW                   pcre16
#define PCRE_UNICODE_FLAGS      PCRE2_UTF
#define _pcre_compile_w         pcre16_compile
#define _pcre_exec_w            pcre16_exec
#define _pcre_fullinfo_w        pcre2_fullinfo_16
#define _pcre_free_w            pcre16_free
#else
#define PCRE_WCHAR              PCRE2_UCHAR32
#define PCREW                   pcre32
#define PCRE_UNICODE_FLAGS      PCRE2_UTF
#define _pcre_compile_w         pcre32_compile
#define _pcre_exec_w            pcre32_exec
#define _pcre_fullinfo_w        pcre2_fullinfo_32
#define _pcre_free_w            pcre32_free
#endif

#ifdef UNICODE
#define PCRE_TCHAR              PCRE_WCHAR
#define PCRE                    PCREW
#define _pcre_compile_t         _pcre_compile_w
#define _pcre_exec_t            _pcre_exec_w
#define _pcre_fullinfo_t        _pcre_fullinfo_w
#define _pcre_free_t            _pcre_free_w
#else   /* UNICODE */
#define PCRE_TCHAR              char
#define PCRE                    pcre2_8
#define _pcre_compile_t         pcre2_compile
#define _pcre_exec_t            pcre2_match
#define _pcre_fullinfo_t        pcre2_fullinfo
#define _pcre_free_t            pcre2_code_free
#endif

#define PCRE_COMMON_FLAGS_W     (PCRE_UNICODE_FLAGS | PCRE2_DOTALL | PCRE2_BSR_UNICODE | PCRE2_NEWLINE_ANY)
#define PCRE_COMMON_FLAGS_A     (PCRE2_DOTALL | PCRE2_BSR_ANYCRLF | PCRE2_NEWLINE_ANYCRLF)

// Additional PCRE1 compatibility constants
#define PCRE_CASELESS           PCRE2_CASELESS
#define PCRE_DOTALL             PCRE2_DOTALL
#define PCRE_INFO_NAMECOUNT     PCRE2_INFO_NAMECOUNT
#define PCRE_INFO_NAMETABLE     PCRE2_INFO_NAMETABLE
#define PCRE_INFO_NAMEENTRYSIZE PCRE2_INFO_NAMEENTRYSIZE
#define PCRE_MULTILINE          PCRE2_MULTILINE

#ifdef UNICODE
#define PCRE_COMMON_FLAGS       PCRE_COMMON_FLAGS_W
#else
#define PCRE_COMMON_FLAGS       PCRE_COMMON_FLAGS_A
#endif

// PCRE2 wrapper functions for compatibility
// ASCII/8-bit wrappers - force 8-bit width for these functions
static inline pcre2_code_8* pcre_compile(const char* pattern, int options, const char** errptr, int* erroffset, const char* tableptr)
{
   int errorcode;
   PCRE2_SIZE erroroffset;
   pcre2_code_8* result = pcre2_compile_8((PCRE2_SPTR8)pattern, PCRE2_ZERO_TERMINATED, options, &errorcode, &erroroffset, nullptr);
   if (erroffset)
      *erroffset = (int)erroroffset;
   return result;
}

static inline int pcre_exec(const pcre2_code_8* code, const pcre_extra* extra, const char* subject, int length, int startoffset, int options, int* ovector, int ovecsize)
{
   pcre2_match_data_8* match_data = pcre2_match_data_create_from_pattern_8(code, nullptr);
   if (!match_data)
      return -1;

   int result = pcre2_match_8(code, (PCRE2_SPTR8)subject, length, startoffset, options, match_data, nullptr);
   if (result > 0)
   {
      PCRE2_SIZE* pcre2_ovector = pcre2_get_ovector_pointer_8(match_data);
      int copy_count = (result * 2 < ovecsize) ? result * 2 : ovecsize;
      for (int i = 0; i < copy_count; i++)
         ovector[i] = (int)pcre2_ovector[i];
   }

   pcre2_match_data_free_8(match_data);
   return result;
}

static inline void pcre_free(pcre2_code_8* code)
{
   if (code)
      pcre2_code_free_8(code);
}

static inline int pcre_fullinfo(const pcre2_code_8* code, const pcre2_match_data_8* mdata, uint32_t what, void* where)
{
   return pcre2_pattern_info_8(code, what, where);
}

// Unicode wrappers
#ifdef UNICODE_UCS2

static inline pcre2_code_16* pcre16_compile(const PCRE2_UCHAR16* pattern, int options, const char** errptr, int* erroffset, const char* tableptr)
{
   int errorcode;
   PCRE2_SIZE erroroffset;
   pcre2_code_16* result = pcre2_compile_16(pattern, PCRE2_ZERO_TERMINATED, options, &errorcode, &erroroffset, nullptr);
   if (erroffset)
      *erroffset = (int)erroroffset;
   return result;
}

static inline int pcre16_exec(const pcre2_code_16* code, const pcre_extra* extra, const PCRE2_UCHAR16* subject, int length, int startoffset, int options, int* ovector, int ovecsize)
{
   pcre2_match_data_16* match_data = pcre2_match_data_create_from_pattern_16(code, nullptr);
   if (!match_data)
      return -1;

   int result = pcre2_match_16(code, subject, length, startoffset, options, match_data, nullptr);
   if (result > 0)
   {
      PCRE2_SIZE* pcre2_ovector = pcre2_get_ovector_pointer_16(match_data);
      int copy_count = (result * 2 < ovecsize) ? result * 2 : ovecsize;
      for (int i = 0; i < copy_count; i++)
         ovector[i] = (int)pcre2_ovector[i];
   }

   pcre2_match_data_free_16(match_data);
   return result;
}

static inline void pcre16_free(pcre2_code_16* code)
{
   if (code)
      pcre2_code_free_16(code);
}

static inline int pcre2_fullinfo_16(const pcre2_code_16* code, const pcre2_match_data_16* mdata, uint32_t what, void* where)
{
   return pcre2_pattern_info_16(code, what, where);
}

#else /* UNICODE_UCS2 */

static inline pcre2_code_32* pcre32_compile(const PCRE2_UCHAR32* pattern, int options, const char** errptr, int* erroffset, const char* tableptr)
{
   int errorcode;
   PCRE2_SIZE erroroffset;
   pcre2_code_32* result = pcre2_compile_32(pattern, PCRE2_ZERO_TERMINATED, options, &errorcode, &erroroffset, nullptr);
   if (erroffset)
      *erroffset = (int)erroroffset;
   return result;
}

static inline int pcre32_exec(const pcre2_code_32* code, const pcre_extra* extra, const PCRE2_UCHAR32* subject, int length, int startoffset, int options, int* ovector, int ovecsize)
{
   pcre2_match_data_32* match_data = pcre2_match_data_create_from_pattern_32(code, nullptr);
   if (!match_data)
      return -1;

   int result = pcre2_match_32(code, subject, length, startoffset, options, match_data, nullptr);
   if (result > 0)
   {
      PCRE2_SIZE* pcre2_ovector = pcre2_get_ovector_pointer_32(match_data);
      int copy_count = (result * 2 < ovecsize) ? result * 2 : ovecsize;
      for (int i = 0; i < copy_count; i++)
         ovector[i] = (int)pcre2_ovector[i];
   }

   pcre2_match_data_free_32(match_data);
   return result;
}

static inline void pcre32_free(pcre2_code_32* code)
{
   if (code)
      pcre2_code_free_32(code);
}

static inline int pcre2_fullinfo_32(const pcre2_code_32* code, const pcre2_match_data_32* mdata, uint32_t what, void* where)
{
   return pcre2_pattern_info_32(code, what, where);
}

#endif

#else /* USE_PCRE2 */

#if HAVE_PCRE_H || defined(_WIN32)
#include <pcre.h>
#elif HAVE_PCRE_PCRE_H
#include <pcre/pcre.h>
#endif

#ifdef UNICODE_UCS2
#define PCRE_WCHAR              PCRE_UCHAR16
#define PCREW                   pcre16
#define PCRE_UNICODE_FLAGS      PCRE_UTF16
#define _pcre_compile_w         pcre16_compile
#define _pcre_exec_w            pcre16_exec
#define _pcre_fullinfo_w        pcre16_fullinfo
#define _pcre_free_w            pcre16_free
#else
#define PCRE_WCHAR              PCRE_UCHAR32
#define PCREW                   pcre32
#define PCRE_UNICODE_FLAGS      PCRE_UTF32
#define _pcre_compile_w         pcre32_compile
#define _pcre_exec_w            pcre32_exec
#define _pcre_fullinfo_w        pcre32_fullinfo
#define _pcre_free_w            pcre32_free
#endif

#ifdef UNICODE
#define PCRE_TCHAR              PCRE_WCHAR
#define PCRE                    PCREW
#define _pcre_compile_t         _pcre_compile_w
#define _pcre_exec_t            _pcre_exec_w
#define _pcre_fullinfo_t        _pcre_fullinfo_w
#define _pcre_free_t            _pcre_free_w
#else   /* UNICODE */
#define PCRE_TCHAR              char
#define PCRE                    pcre
#define _pcre_compile_t         pcre_compile
#define _pcre_exec_t            pcre_exec
#define _pcre_fullinfo_t        pcre_fullinfo
#define _pcre_free_t            pcre_free
#endif

#define PCRE_COMMON_FLAGS_W     (PCRE_UNICODE_FLAGS | PCRE_DOTALL | PCRE_BSR_UNICODE | PCRE_NEWLINE_ANY)
#define PCRE_COMMON_FLAGS_A     (PCRE_DOTALL | PCRE_BSR_ANYCRLF | PCRE_NEWLINE_ANYCRLF)

#ifdef UNICODE
#define PCRE_COMMON_FLAGS       PCRE_COMMON_FLAGS_W
#else
#define PCRE_COMMON_FLAGS       PCRE_COMMON_FLAGS_A
#endif

#endif   /* USE_PCRE2 */

/**
 * Extract integer from capture group (wide character version)
 */
static inline uint32_t IntegerFromCGroupW(const WCHAR *text, int *cgroups, int cgindex)
{
   WCHAR buffer[32];
   int len = cgroups[cgindex * 2 + 1] - cgroups[cgindex * 2];
   if (len > 31)
      len = 31;
   memcpy(buffer, &text[cgroups[cgindex * 2]], len * sizeof(WCHAR));
   buffer[len] = 0;
   return wcstoul(buffer, nullptr, 10);
}

/**
 * Extract integer from capture group
 */
static inline uint32_t IntegerFromCGroupA(const char *text, int *cgroups, int cgindex)
{
   char buffer[32];
   int len = cgroups[cgindex * 2 + 1] - cgroups[cgindex * 2];
   if (len > 31)
      len = 31;
   memcpy(buffer, &text[cgroups[cgindex * 2]], len);
   buffer[len] = 0;
   return strtoul(buffer, nullptr, 10);
}

#ifdef UNICODE
#define IntegerFromCGroup IntegerFromCGroupW
#else
#define IntegerFromCGroup IntegerFromCGroupA
#endif

#endif	/* _netxms_regex_h */
