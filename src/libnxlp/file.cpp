/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: file.cpp
**
**/

#include "libnxlp.h"

#ifdef _NETWARE
#include <fsio.h>
#endif

#ifdef _WIN32
#include <share.h>

#define read _read
#define close _close

#endif


#if defined(_WIN32)
#define NX_STAT _tstati64
#define NX_STAT_STRUCT struct _stati64
#elif HAVE_STAT64 && HAVE_STRUCT_STAT64
#define NX_STAT stat64
#define NX_STAT_STRUCT struct stat64
#else
#define NX_STAT stat
#define NX_STAT_STRUCT struct stat
#endif

#if defined(_WIN32)
#define NX_FSTAT _fstati64
#elif HAVE_FSTAT64 && HAVE_STRUCT_STAT64
#define NX_FSTAT fstat64
#else
#define NX_FSTAT fstat
#endif

#if defined(UNICODE) && !defined(_WIN32)
inline int __call_stat(const WCHAR *f, NX_STAT_STRUCT *s)
{
	char *mbf = MBStringFromWideString(f);
	int rc = NX_STAT(mbf, s);
	free(mbf);
	return rc;
}
#define CALL_STAT(f, s) __call_stat(f, s)
#else
#define CALL_STAT(f, s) NX_STAT(f, s)
#endif


/**
 * Constants
 */
#define READ_BUFFER_SIZE      4096

/**
 * Find byte sequence in the stream
 */
static char *FindSequence(char *start, int length, const char *sequence, int seqLength)
{
	char *curr = start;
	int count = 0;
	while(length - count >= seqLength)
	{
		if (!memcmp(curr, sequence, seqLength))
			return curr;
		curr += seqLength;
		count += seqLength;
	}
	return NULL;
}

/**
 * Find end-of-line marker
 */
static char *FindEOL(char *start, int length, int encoding)
{
   char *eol = NULL;
	switch(encoding)
	{
		case LP_FCP_UCS2:
			eol = FindSequence(start, length, "\n\0", 2);
			break;
		case LP_FCP_UCS2_LE:
			eol = FindSequence(start, length, "\0\n", 2);
			break;
		case LP_FCP_UCS4:
			eol = FindSequence(start, length, "\n\0\0\0", 4);
			break;
		case LP_FCP_UCS4_LE:
			eol = FindSequence(start, length, "\0\0\0\n", 4);
			break;
		default:
			eol = (char *)memchr(start, '\n', length);
			break;
	}

   if (eol == NULL)
   {
      // Try to find CR
	   switch(encoding)
	   {
		   case LP_FCP_UCS2:
			   eol = FindSequence(start, length, "\r\0", 2);
				break;
		   case LP_FCP_UCS2_LE:
			   eol = FindSequence(start, length, "\0\r", 2);
				break;
		   case LP_FCP_UCS4:
			   eol = FindSequence(start, length, "\r\0\0\0", 4);
				break;
		   case LP_FCP_UCS4_LE:
			   eol = FindSequence(start, length, "\0\0\0\r", 4);
				break;
		   default:
			   eol = (char *)memchr(start, '\r', length);
				break;
	   }
   }

   return eol;
}

/**
 * Parse new log records
 */
static void ParseNewRecords(LogParser *parser, int fh)
{
   char *ptr, *eptr, buffer[READ_BUFFER_SIZE];
   int bytes, bufPos = 0;
	int encoding = parser->getFileEncoding();
	TCHAR text[READ_BUFFER_SIZE];

   do
   {
      if ((bytes = read(fh, &buffer[bufPos], READ_BUFFER_SIZE - bufPos)) > 0)
      {
         bytes += bufPos;
         for(ptr = buffer;; ptr = eptr + 1)
         {
            bufPos = (int)(ptr - buffer);
				eptr = FindEOL(ptr, bytes - bufPos, encoding);
            if (eptr == NULL)
            {
					bufPos = bytes - bufPos;
               memmove(buffer, ptr, bufPos);
               break;
            }

				// remove possible CR character and put 0 to indicate end of line
				switch(encoding)
				{
					case LP_FCP_UCS2:
						if ((eptr - ptr >= 2) && !memcmp(eptr - 2, "\r\0", 2))
							eptr -= 2;
						*eptr = 0;
						*(eptr + 1) = 0;
						break;
					case LP_FCP_UCS2_LE:
						if ((eptr - ptr >= 2) && !memcmp(eptr - 2, "\0\r", 2))
							eptr -= 2;
						*eptr = 0;
						*(eptr + 1) = 0;
						break;
					case LP_FCP_UCS4:
						if ((eptr - ptr >= 4) && !memcmp(eptr - 4, "\r\0\0\0", 4))
							eptr -= 4;
						memset(eptr, 0, 4);
						break;
					case LP_FCP_UCS4_LE:
						if ((eptr - ptr >= 4) && !memcmp(eptr - 4, "\0\0\0\r", 4))
							eptr -= 4;
						memset(eptr, 0, 4);
						break;
					default:
						if (*(eptr - 1) == '\r')
							eptr--;
						*eptr = 0;
						break;
				}

				// Now ptr points to null-terminated string in original encoding
				// Do the conversion to platform encoding
/* TODO: implement all conversions */
#ifdef UNICODE
				switch(encoding)
				{
					case LP_FCP_ACP:
						MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ptr, -1, text, READ_BUFFER_SIZE);
						break;
					case LP_FCP_UTF8:
						MultiByteToWideChar(CP_UTF8, 0, ptr, -1, text, READ_BUFFER_SIZE);
						break;
					default:
						break;
				}
#else
				switch(encoding)
				{
					case LP_FCP_ACP:
						nx_strncpy(text, ptr, READ_BUFFER_SIZE);
						break;
					default:
						break;
				}
#endif

				parser->matchLine(text);
         }
      }
      else
      {
         bytes = 0;
      }
   } while(bytes == READ_BUFFER_SIZE);
}

/**
 * File parser thread
 */
bool LogParser::monitorFile(CONDITION stopCondition, bool readFromCurrPos)
{
	TCHAR fname[MAX_PATH], temp[MAX_PATH];
	NX_STAT_STRUCT st, stn;
	size_t size;
	int fh;
	bool readFromStart = !readFromCurrPos;

	if (m_fileName == NULL)
	{
		LogParserTrace(0, _T("LogParser: parser thread will not start, file name not set"));
		return false;
	}

	LogParserTrace(0, _T("LogParser: parser thread for file \"%s\" started"), m_fileName);
	while(1)
	{
		ExpandFileName(getFileName(), fname, MAX_PATH, true);
		if (CALL_STAT(fname, &st) == 0)
		{
#ifdef _WIN32
			fh = _tsopen(fname, O_RDONLY, _SH_DENYNO);
#else
			fh = _topen(fname, O_RDONLY);
#endif
			if (fh != -1)
			{
				setStatus(LPS_RUNNING);
				LogParserTrace(3, _T("LogParser: file \"%s\" (pattern \"%s\") successfully opened"), fname, m_fileName);

				size = (size_t)st.st_size;
				if (readFromStart)
				{
					LogParserTrace(5, _T("LogParser: parsing existing records in file \"%s\""), fname);
					ParseNewRecords(this, fh);
					readFromStart = false;
				}
				else
				{
					lseek(fh, 0, SEEK_END);
				}

				while(1)
				{
					if (ConditionWait(stopCondition, 5000))
						goto stop_parser;

					// Check if file name was changed
					ExpandFileName(getFileName(), temp, MAX_PATH, true);
					if (_tcscmp(temp, fname))
					{
						LogParserTrace(5, _T("LogParser: file name change for \"%s\" (\"%s\" -> \"%s\")"), m_fileName, fname, temp);
						readFromStart = true;
						break;
					}

#ifdef _NETWARE
					if (fgetstat(fh, &st, ST_SIZE_BIT | ST_NAME_BIT) < 0)
					{
						LogParserTrace(1, _T("LogParser: fgetstat(%d) failed, errno=%d"), fh, errno);
						readFromStart = true;
						break;
					}
#else
					if (NX_FSTAT(fh, &st) < 0)
					{
						LogParserTrace(1, _T("LogParser: fstat(%d) failed, errno=%d"), fh, errno);
						readFromStart = true;
						break;
					}
#endif

					if (CALL_STAT(fname, &stn) < 0)
					{
						LogParserTrace(1, _T("LogParser: stat(%s) failed, errno=%d"), fname, errno);
						readFromStart = true;
						break;
					}

#ifdef _WIN32
					if (st.st_size > stn.st_size)
					{
						LogParserTrace(3, _T("LogParser: file size for fstat(%d) is greater then for stat(%s), assume file rename"), fh, fname);
						readFromStart = true;
						break;
					}
#else
					if ((st.st_ino != stn.st_ino) || (st.st_dev != stn.st_dev))
					{
						LogParserTrace(3, _T("LogParser: file device or inode differs for stat(%d) and fstat(%s), assume file rename"), fh, fname);
						readFromStart = true;
						break;
					}
#endif

					if ((size_t)st.st_size != size)
					{
						if ((size_t)st.st_size < size)
						{
							// File was cleared, start from the beginning
							lseek(fh, 0, SEEK_SET);
							LogParserTrace(3, _T("LogParser: file \"%s\" st_size != size"), fname);
						}
						size = (size_t)st.st_size;
						LogParserTrace(6, _T("LogParser: new data avialable in file \"%s\""), fname);
						ParseNewRecords(this, fh);
					}
				}
				close(fh);
			}
			else
			{
				setStatus(LPS_OPEN_ERROR);
			}
		}
		else
		{
			setStatus(LPS_NO_FILE);
			if (ConditionWait(stopCondition, 10000))
				break;
		}
	}

stop_parser:
	LogParserTrace(0, _T("LogParser: parser thread for file \"%s\" stopped"), m_fileName);
	return true;
}
