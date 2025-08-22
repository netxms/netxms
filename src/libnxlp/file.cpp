/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2024 Raden Solutions
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
#include <nxstat.h>

#ifdef _WIN32
#include <share.h>
#include <comdef.h>
#endif

/**
 * File encoding names
 */
static const TCHAR *s_encodingName[] =
{
   _T("ACP"),
   _T("UTF-8"),
   _T("UCS-2"),
   _T("UCS-2LE"),
   _T("UCS-2BE"),
   _T("UCS-4"),
   _T("UCS-4LE"),
   _T("UCS-4BE")
};

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
#if WORDS_BIGENDIAN
			eol = FindSequence(start, length, "\0\n", 2);
#else
			eol = FindSequence(start, length, "\n\0", 2);
#endif
			break;
		case LP_FCP_UCS2_LE:
			eol = FindSequence(start, length, "\n\0", 2);
			break;
		case LP_FCP_UCS2_BE:
			eol = FindSequence(start, length, "\0\n", 2);
			break;
		case LP_FCP_UCS4:
#if WORDS_BIGENDIAN
         eol = FindSequence(start, length, "\0\0\0\n", 4);
#else
         eol = FindSequence(start, length, "\n\0\0\0", 4);
#endif
		   break;
		case LP_FCP_UCS4_LE:
			eol = FindSequence(start, length, "\n\0\0\0", 4);
			break;
		case LP_FCP_UCS4_BE:
			eol = FindSequence(start, length, "\0\0\0\n", 4);
			break;
		default:
			eol = (char *)memchr(start, '\n', length);
			break;
	}

   if (eol == nullptr)
   {
      // Try to find CR
	   switch(encoding)
	   {
		case LP_FCP_UCS2:
#if WORDS_BIGENDIAN
			eol = FindSequence(start, length, "\0\r", 2);
#else
			eol = FindSequence(start, length, "\r\0", 2);
#endif
			break;
		   case LP_FCP_UCS2_LE:
			   eol = FindSequence(start, length, "\r\0", 2);
				break;
		   case LP_FCP_UCS2_BE:
			   eol = FindSequence(start, length, "\0\r", 2);
				break;
		case LP_FCP_UCS4:
#if WORDS_BIGENDIAN
         eol = FindSequence(start, length, "\0\0\0\r", 4);
#else
         eol = FindSequence(start, length, "\r\0\0\0", 4);
#endif
		   break;
		   case LP_FCP_UCS4_LE:
			   eol = FindSequence(start, length, "\r\0\0\0", 4);
				break;
		   case LP_FCP_UCS4_BE:
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
off_t LogParser::processNewRecords(int fh, const TCHAR *fileName)
{
   int charSize;
   switch (m_fileEncoding)
   {
      case LP_FCP_UCS2:
      case LP_FCP_UCS2_LE:
      case LP_FCP_UCS2_BE:
         charSize = 2;
         break;
      case LP_FCP_UCS4:
      case LP_FCP_UCS4_LE:
      case LP_FCP_UCS4_BE:
         charSize = 4;
         break;
      default:
         charSize = 1;
         break;
   }

   if (m_readBuffer == nullptr)
   {
      m_readBufferSize = 4096;
      m_readBuffer = MemAllocStringA(m_readBufferSize);
      m_textBuffer = MemAllocString(m_readBufferSize);
   }

   int bytes, bufPos = 0;
   off_t resetPos = _lseek(fh, 0, SEEK_CUR);
   do
   {
      if ((bytes = _read(fh, &m_readBuffer[bufPos], m_readBufferSize - bufPos)) > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Read %d bytes into buffer at offset %d"), bytes, bufPos);
         bytes += bufPos;

         char *ptr, *eptr;
         for(ptr = m_readBuffer;; ptr = eptr + charSize)
         {
            bufPos = (int)(ptr - m_readBuffer);
				eptr = FindEOL(ptr, bytes - bufPos, m_fileEncoding);
            if (eptr == nullptr)
            {
					int remaining = bytes - bufPos;
               resetPos = _lseek(fh, 0, SEEK_CUR) - remaining;
               if (remaining == m_readBufferSize)
               {
                  // buffer is full, and no new line in buffer
                  m_readBufferSize += 4096;
                  m_readBuffer = MemRealloc(m_readBuffer, m_readBufferSize);
                  m_textBuffer = MemReallocArray(m_textBuffer, m_readBufferSize);
               }
               else if (remaining > 0)
               {
                  if (m_readBuffer != ptr)
                     memmove(m_readBuffer, ptr, remaining);
                  if (m_preallocatedFile && !memcmp(m_readBuffer, "\x00\x00\x00\x00", std::min(remaining, 4)))
                  {
                     // Found zeroes in preallocated file, next read should be after last known EOL
                     return resetPos;
                  }
               }
               bufPos = remaining;
               nxlog_debug_tag(DEBUG_TAG, 7, _T("Last line in data block for file \"%s\", resetPos=") UINT64_FMT _T(", remaining=%d"),
                     m_fileName, static_cast<uint64_t>(resetPos), remaining);
               break;
            }
            // remove possible CR character and put 0 to indicate end of line
            switch(m_fileEncoding)
            {
               case LP_FCP_UCS2:
#if WORDS_BIGENDIAN
                  if ((eptr - ptr >= 2) && !memcmp(eptr - 2, "\0\r", 2))
                     *(eptr - 1) = 0;
                  else
                     *(eptr + 1) = 0;
#else
                  if ((eptr - ptr >= 2) && !memcmp(eptr - 2, "\r\0", 2))
                     *(eptr - 2) = 0;
                  else
                     *eptr = 0;
#endif
                  break;
               case LP_FCP_UCS2_LE:
                  if ((eptr - ptr >= 2) && !memcmp(eptr - 2, "\r\0", 2))
                     *(eptr - 2) = 0;
                  else
                     *eptr = 0;
                  break;
               case LP_FCP_UCS2_BE:
                  if ((eptr - ptr >= 2) && !memcmp(eptr - 2, "\0\r", 2))
                     *(eptr - 1) = 0;
                  else
                     *(eptr + 1) = 0;
                  break;
               case LP_FCP_UCS4:
#if WORDS_BIGENDIAN
                  if ((eptr - ptr >= 4) && !memcmp(eptr - 4, "\0\0\0\r", 4))
                     *(eptr - 1) = 0;
                  else
                     *(eptr + 3) = 0;
#else
                  if ((eptr - ptr >= 4) && !memcmp(eptr - 4, "\r\0\0\0", 4))
                     *(eptr - 4) = 0;
                  else
                     *eptr = 0;
#endif
                  break;
               case LP_FCP_UCS4_LE:
                  if ((eptr - ptr >= 4) && !memcmp(eptr - 4, "\r\0\0\0", 4))
                     *(eptr - 4) = 0;
                  else
                     *eptr = 0;
                  break;
               case LP_FCP_UCS4_BE:
                  if ((eptr - ptr >= 4) && !memcmp(eptr - 4, "\0\0\0\r", 4))
                     *(eptr - 1) = 0;
                  else
                     *(eptr + 3) = 0;
                  break;
               default:
                  if ((eptr - ptr >= 1) && *(eptr - 1) == '\r')
                     *(eptr - 1) = 0;
                  else
                     *eptr = 0;
                  break;
            }

            // Now ptr points to null-terminated string in original encoding
            // Do the conversion to platform encoding
#ifdef UNICODE
            switch(m_fileEncoding)
            {
               case LP_FCP_ACP:
                  mb_to_wchar(ptr, -1, m_textBuffer, m_readBufferSize);
                  break;
               case LP_FCP_UTF8:
                  utf8_to_wchar(ptr, -1, m_textBuffer, m_readBufferSize);
                  break;
               case LP_FCP_UCS2_LE:
#if WORDS_BIGENDIAN
                  bswap_array_16((UINT16 *)ptr, -1);
#endif
#ifdef UNICODE_UCS2
                  wcslcpy(m_textBuffer, (WCHAR *)ptr, m_readBufferSize);
#else
                  ucs2_to_ucs4((UCS2CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
#endif
                  break;
               case LP_FCP_UCS2_BE:
#if !WORDS_BIGENDIAN
                  bswap_array_16((UINT16 *)ptr, -1);
#endif
#ifdef UNICODE_UCS2
                  wcslcpy(m_textBuffer, (WCHAR *)ptr, m_readBufferSize);
#else
                  ucs2_to_ucs4((UCS2CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
#endif
                  break;
               case LP_FCP_UCS2:
#ifdef UNICODE_UCS2
                  wcslcpy(m_textBuffer, (WCHAR *)ptr, m_readBufferSize);
#else
                  ucs2_to_ucs4((UCS2CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
#endif
                  break;
               case LP_FCP_UCS4_LE:
#if WORDS_BIGENDIAN
                  bswap_array_32((UINT32 *)ptr, -1);
#endif
#ifdef UNICODE_UCS2
                  ucs4_to_ucs2((UCS4CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
#else
                  wcslcpy(m_textBuffer, (WCHAR *)ptr, m_readBufferSize);
#endif
                  break;
               case LP_FCP_UCS4_BE:
#if !WORDS_BIGENDIAN
                  bswap_array_32((UINT32 *)ptr, -1);
#endif
#ifdef UNICODE_UCS2
                  ucs4_to_ucs2((UCS4CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
#else
                  wcslcpy(m_textBuffer, (WCHAR *)ptr, m_readBufferSize);
#endif
                  break;
               case LP_FCP_UCS4:
#ifdef UNICODE_UCS2
                  ucs4_to_ucs2((UCS4CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
#else
                  wcslcpy(m_textBuffer, (WCHAR *)ptr, m_readBufferSize);
#endif
                  break;
               default:
                  break;
            }
#else
            switch(m_fileEncoding)
            {
               case LP_FCP_ACP:
                  _tcslcpy(m_textBuffer, ptr, m_readBufferSize);
                  break;
               case LP_FCP_UTF8:
                  utf8_to_mb(ptr, -1, m_textBuffer, m_readBufferSize);
                  break;
               case LP_FCP_UCS2_LE:
#if WORDS_BIGENDIAN
                  bswap_array_16((UINT16 *)ptr, -1);
#endif
                  ucs2_to_mb((UCS2CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
                  break;
               case LP_FCP_UCS2_BE:
#if !WORDS_BIGENDIAN
                  bswap_array_16((UINT16 *)ptr, -1);
#endif
                  ucs2_to_mb((UCS2CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
                  break;
               case LP_FCP_UCS2:
                  ucs2_to_mb((UCS2CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
                  break;
               case LP_FCP_UCS4_LE:
#if WORDS_BIGENDIAN
                  bswap_array_32((UINT32 *)ptr, -1);
#endif
                  ucs4_to_mb((UCS4CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
                  break;
               case LP_FCP_UCS4_BE:
#if !WORDS_BIGENDIAN
                  bswap_array_32((UINT32 *)ptr, -1);
#endif
                  ucs4_to_mb((UCS4CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
                  break;
               case LP_FCP_UCS4:
                  ucs4_to_mb((UCS4CHAR *)ptr, -1, m_textBuffer, m_readBufferSize);
                  break;
               default:
                  break;
            }
#endif
            matchLine(m_textBuffer, fileName);
         }
      }
      else
      {
         bytes = 0;
      }
   } while(bytes > 0);
   return resetPos;
}

/**
 * Scan first 4 bytes of a file to find its encoding
 */
static int ScanFileEncoding(int fh)
{
   auto pos = _tell(fh);
   _lseek(fh, 0, SEEK_SET);

   int encoding = LP_FCP_ACP;

   char buffer[10];
   if (_read(fh, buffer, 4) > 3)
   {
      if (!memcmp(buffer, "\x00\x00\xFE\xFF", 4))
         encoding = LP_FCP_UCS4_BE;
      else if (!memcmp(buffer, "\xFF\xFE\x00\x00", 4))
         encoding = LP_FCP_UCS4_LE;
      else if (!memcmp(buffer, "\xEF\xBB\xBF", 3))
         encoding = LP_FCP_UTF8;
      else if (!memcmp(buffer, "\xFE\xFF", 2))
         encoding = LP_FCP_UCS2_BE;
      else if (!memcmp(buffer, "\xFF\xFE", 2))
         encoding = LP_FCP_UCS2_LE;
   }

   _lseek(fh, pos, SEEK_SET);
   return encoding;
}

/**
 * Seek file to the end of zeroes block using given character size
 * Reset position to the beginning of zero block if zeroes are till end of file
 */
template<typename T> bool SkipZeroBlock(int fh)
{
   off_t startPos = _lseek(fh, 0, SEEK_CUR);

   char buffer[4096];
   while(true)
   {
      int bytes = _read(fh, buffer, 4096);
      if (bytes <= 0)
         break;
      T *p = reinterpret_cast<T*>(buffer);
      for(int i = 0; i < bytes - static_cast<int>(sizeof(T)) + 1; i += sizeof(T), p++)
      {
         if (*p != 0)
         {
            off_t pos = _lseek(fh, i - bytes, SEEK_CUR);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("LogParser: end of zero block found at %ld"), (long)pos);
            return true;
         }
      }
   }

   _lseek(fh, startPos, SEEK_SET);
   return false;
}

/**
 * Seek file to the end of zeroes block
 * Reset position to the beginning of zero block if zeroes are till end of file
 */
bool LIBNXLP_EXPORTABLE SkipZeroBlock(int fh, int chsize)
{
   switch(chsize)
   {
      case 1:
         return SkipZeroBlock<char>(fh);
      case 2:
         return SkipZeroBlock<int16_t>(fh);
      case 4:
         return SkipZeroBlock<int32_t>(fh);
   }
   return false;
}

/**
 * Seek file to the beginning of zeroes block using given character size
 */
template<typename T> bool SeekToZero(int fh)
{
   char buffer[4096];
   while(true)
   {
      int bytes = _read(fh, buffer, 4096);
      if (bytes <= 0)
         break;
      T *p = reinterpret_cast<T*>(buffer);
      for(int i = 0; i < bytes - static_cast<int>(sizeof(T)) + 1; i += sizeof(T), p++)
      {
         if (*p == 0)
         {
            off_t pos = _lseek(fh, i - bytes, SEEK_CUR);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Beginning of zero block found at %ld"), (long)pos);
            return true;
         }
      }
   }
   return false;
}

/**
 * Seek file to the beginning of zeroes block
 */
static void SeekToZero(int fh, int chsize, bool detectBrokenPrealloc)
{
   bool found;
   switch(chsize)
   {
      case 1:
         found = SeekToZero<char>(fh);
         break;
      case 2:
         found = SeekToZero<int16_t>(fh);
         break;
      case 4:
         found = SeekToZero<int32_t>(fh);
         break;
      default:
         found = false;
         break;
   }

   if (found && detectBrokenPrealloc)
   {
      if (SkipZeroBlock(fh, chsize))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("LogParser: broken preallocated file detected"));
         _lseek(fh, 0, SEEK_END); // assume that file has broken preallocation, read from end of file
      }
   }
}

/**
 * Call appropriate stat() function
 */
static inline int __stat(LogParser *parser, const TCHAR *fname, NX_STAT_STRUCT* st)
{
#ifdef _WIN32
   return CALL_STAT(fname, st);
#else
   if (parser->isFollowSymlinks())
      return CALL_STAT_FOLLOW_SYMLINK(fname, st);

   int result = CALL_STAT(fname, st);
   if ((result == 0) && S_ISLNK(st->st_mode))
   {
      errno = ENOENT;
      result = -1;
   }
   return result;
#endif
}

/**
 * File parser thread
 */
bool LogParser::monitorFile(off_t startOffset)
{
	if (m_fileName == nullptr)
	{
		nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread will not start, file name not set"));
		return false;
	}

#ifdef _WIN32
   if (m_useSnapshot)
   {
	   nxlog_debug_tag(DEBUG_TAG, 0, _T("Using VSS snapshots for file \"%s\""), m_fileName);
      return monitorFileWithSnapshot(startOffset);
   }
#endif

   if (!m_keepFileOpen)
   {
	   nxlog_debug_tag(DEBUG_TAG, 0, _T("LogParser: \"keep open\" option disabled for file \"%s\""), m_fileName);
      return monitorFile2(startOffset);
   }

   bool readFromStart = (m_rescan || (startOffset == 0));

	nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" started"), m_fileName);
	bool exclusionPeriod = false;
	while(true)
	{
	   if (isExclusionPeriod())
	   {
	      if (!exclusionPeriod)
	      {
	         exclusionPeriod = true;
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Will not open file \"%s\" because of exclusion period"), getFileName());
            setStatus(LPS_SUSPENDED);
	      }
         if (m_stopCondition.wait(30000))
            break;
         continue;
	   }

	   if (exclusionPeriod)
	   {
	      exclusionPeriod = false;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Exclusion period for file \"%s\" ended"), getFileName());
	   }

      TCHAR fname[MAX_PATH];
      ExpandFileName(getFileName(), fname, MAX_PATH, true);
      NX_STAT_STRUCT st;
      if (__stat(this, fname, &st) != 0)
      {
         if (errno == ENOENT)
            readFromStart = true;
         setStatus(LPS_NO_FILE);
         if (m_stopCondition.wait(10000))
            break;
         continue;
      }

#ifdef _WIN32
      int fh = _tsopen(fname, O_RDONLY | O_BINARY, _SH_DENYNO);
#else
		int fh = _topen(fname, O_RDONLY | (m_followSymlinks ? 0 : O_NOFOLLOW));
#endif
		if (fh == -1)
      {
         setStatus(LPS_OPEN_ERROR);
         if (m_stopCondition.wait(10000))
            break;
         continue;
      }

		setStatus(LPS_RUNNING);
		nxlog_debug_tag(DEBUG_TAG, 3, _T("File \"%s\" (pattern \"%s\") successfully opened"), fname, m_fileName);

      if (m_fileEncoding == LP_FCP_AUTO)
      {
         m_fileEncoding = ScanFileEncoding(fh);
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Detected encoding %s for file \"%s\""), s_encodingName[m_fileEncoding], fname);
      }

		size_t size = (size_t)st.st_size;
		time_t mtime = st.st_mtime;
		if (readFromStart)
		{
			nxlog_debug_tag(DEBUG_TAG, 5, _T("Parsing existing records in file \"%s\""), fname);
			off_t resetPos = processNewRecords(fh, fname);
         _lseek(fh, resetPos, SEEK_SET);
         m_offset = resetPos;
         readFromStart = m_rescan;
         startOffset = -1;
      }
      else if (startOffset > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Parsing existing records in file \"%s\" starting at offset ") INT64_FMT, fname, static_cast<int64_t>(startOffset));
         _lseek(fh, startOffset, SEEK_SET);
         off_t resetPos = processNewRecords(fh, fname);
         _lseek(fh, resetPos, SEEK_SET);
         m_offset = resetPos;
         startOffset = -1;
      }
		else if (m_preallocatedFile)
		{
			SeekToZero(fh, getCharSize(), m_detectBrokenPrealloc);
		}
		else
		{
			_lseek(fh, 0, SEEK_END);
		}

		while(true)
		{
			if (m_stopCondition.wait(m_fileCheckInterval))
			{
			   _close(fh);
				goto stop_parser;
			}

			// Check if file name was changed
         TCHAR temp[MAX_PATH];
			ExpandFileName(getFileName(), temp, MAX_PATH, true);
			if (_tcscmp(temp, fname))
			{
				nxlog_debug_tag(DEBUG_TAG, 5, _T("File name change for \"%s\" (\"%s\" -> \"%s\")"), m_fileName, fname, temp);
				readFromStart = true;
				break;
			}

			if (NX_FSTAT(fh, &st) < 0)
			{
				nxlog_debug_tag(DEBUG_TAG, 1, _T("fstat(%d) failed, errno=%d"), fh, errno);
				readFromStart = true;
				break;
			}

         NX_STAT_STRUCT stn;
         if (__stat(this, fname, &stn) < 0)
			{
				nxlog_debug_tag(DEBUG_TAG, 1, _T("stat(%s) failed, errno=%d"), fname, errno);
				readFromStart = true;
				break;
			}

#ifdef _WIN32
			if (st.st_ctime != stn.st_ctime)
			{
				nxlog_debug_tag(DEBUG_TAG, 3, _T("Creation time for fstat(%d) is not equal to creation time for stat(%s), assume file rename"), fh, fname);
				readFromStart = true;
				break;
			}
#else
			if ((st.st_ino != stn.st_ino) || (st.st_dev != stn.st_dev))
			{
				nxlog_debug_tag(DEBUG_TAG, 3, _T("File device or inode differs for stat(%d) and fstat(%s), assume file rename"), fh, fname);
				readFromStart = true;
				break;
			}
#endif

			if (((size_t)st.st_size != size) ||
			    (!m_ignoreMTime && m_rescan && (mtime != st.st_mtime)))
			{
				if (((size_t)st.st_size < size) || m_rescan)
				{
					// File was cleared, start from the beginning
					_lseek(fh, 0, SEEK_SET);
					if (!m_rescan)
					   nxlog_debug_tag(DEBUG_TAG, 3, _T("File \"%s\" st_size < size, assume file rotation"), fname);
				}
				size = (size_t)st.st_size;
				mtime = st.st_mtime;
				nxlog_debug_tag(DEBUG_TAG, 6, _T("New data available in file \"%s\""), fname);
				off_t resetPos = processNewRecords(fh, fname);
				_lseek(fh, resetPos, SEEK_SET);
	         m_offset = resetPos;
			}
			else if (m_preallocatedFile)
			{
				char buffer[4];
				int bytes = _read(fh, buffer, 4);
				if ((bytes == 4) && memcmp(buffer, "\x00\x00\x00\x00", 4))
				{
               _lseek(fh, -4, SEEK_CUR);
	            nxlog_debug_tag(DEBUG_TAG, 6, _T("New data available in file \"%s\""), fname);
	            off_t resetPos = processNewRecords(fh, fname);
	            _lseek(fh, resetPos, SEEK_SET);
	            m_offset = resetPos;
				}
				else
				{
               off_t pos = _lseek(fh, -bytes, SEEK_CUR);
               if (pos > 0)
               {
                  int readSize = std::min(pos, (off_t)4);
                  _lseek(fh, -readSize, SEEK_CUR);
                  int bytes = _read(fh, buffer, readSize);
                  if ((bytes == readSize) && !memcmp(buffer, "\x00\x00\x00\x00", readSize))
                  {
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("Detected reset of preallocated file \"%s\""), fname);
                     _lseek(fh, 0, SEEK_SET);
                     off_t resetPos = processNewRecords(fh, fname);
                     _lseek(fh, resetPos, SEEK_SET);
                     m_offset = resetPos;
                  }
               }
				}
			}

			if (isExclusionPeriod())
			{
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Closing file \"%s\" because of exclusion period"), fname);
            exclusionPeriod = true;
            setStatus(LPS_SUSPENDED);
				break;
			}
		}
		_close(fh);
	}

stop_parser:
   nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" stopped"), m_fileName);
	return true;
}

/**
 * File parser thread (do not keep it open)
 */
bool LogParser::monitorFile2(off_t startOffset)
{
   size_t size = 0;
   time_t mtime = 0;
#ifdef _WIN32
   time_t ctime = 0; // creation time on Windows
#endif
   off_t lastPos = 0;
   bool readFromStart = (m_rescan || (startOffset == 0));
   bool firstRead = true;

   nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" started (\"keep open\" option disabled)"), m_fileName);
   bool exclusionPeriod = false;
   while(true)
   {
      if (isExclusionPeriod())
      {
         if (!exclusionPeriod)
         {
            exclusionPeriod = true;
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Will not open file \"%s\" because of exclusion period"), getFileName());
            setStatus(LPS_SUSPENDED);
         }
         if (m_stopCondition.wait(30000))
            break;
         continue;
      }

      if (exclusionPeriod)
      {
         exclusionPeriod = false;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Exclusion period for file \"%s\" ended"), getFileName());
      }

      TCHAR fname[MAX_PATH];
      ExpandFileName(getFileName(), fname, MAX_PATH, true);

      NX_STAT_STRUCT st;
      if (__stat(this, fname, &st) != 0)
      {
         if (errno == ENOENT)
         {
            readFromStart = true;
            firstRead = true;
            startOffset = -1;
         }
         setStatus(LPS_NO_FILE);
         if (m_stopCondition.wait(10000))
            break;
         continue;
      }

#ifdef _WIN32
      if (firstRead)
         ctime = st.st_ctime; // prevent incorrect rotation detection on first read
#endif

      if (!readFromStart && (startOffset == -1))
      {
#ifdef _WIN32
         if ((m_ignoreMTime && !m_preallocatedFile && (size == st.st_size) && (ctime == st.st_ctime)) ||
             (!m_ignoreMTime && (size == st.st_size) && (mtime == st.st_mtime) && (ctime == st.st_ctime)))
#else
         if ((m_ignoreMTime && !m_preallocatedFile && (size == st.st_size)) ||
             (!m_ignoreMTime && (size == st.st_size) && (mtime == st.st_mtime)))
#endif
         {
            if (m_stopCondition.wait(m_fileCheckInterval))
               break;
            continue;
         }
      }

#ifdef _WIN32
      int fh = _tsopen(fname, O_RDONLY | O_BINARY, _SH_DENYNO);
#else
      int fh = _topen(fname, O_RDONLY | (m_followSymlinks ? 0 : O_NOFOLLOW));
#endif
      if (fh == -1)
      {
         setStatus(LPS_OPEN_ERROR);
         if (m_stopCondition.wait(10000))  // retry in 10 seconds
            break;
         continue;
      }

      setStatus(LPS_RUNNING);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("File \"%s\" (pattern \"%s\") successfully opened"), fname, m_fileName);

#ifdef _WIN32
      if (!readFromStart && ((size > static_cast<size_t>(st.st_size)) || (ctime != st.st_ctime)))
#else
      if (!readFromStart && (size > static_cast<size_t>(st.st_size)))
#endif
      {
#ifdef _WIN32
         nxlog_debug_tag(DEBUG_TAG, 5, _T("File \"%s\" rotation detected (size=%llu/%llu, ctime=%llu/%llu)"), fname,
               static_cast<UINT64>(size), static_cast<UINT64>(st.st_size), static_cast<UINT64>(ctime), static_cast<UINT64>(st.st_ctime));
         ctime = st.st_ctime;
#else
         nxlog_debug_tag(DEBUG_TAG, 5, _T("File \"%s\" rotation detected (size=%llu/%llu)"), fname,
               static_cast<UINT64>(size), static_cast<UINT64>(st.st_size));
#endif
         readFromStart = true;   // Assume file rotation
         startOffset = -1;
      }

      if (m_fileEncoding == LP_FCP_AUTO)
      {
         m_fileEncoding = ScanFileEncoding(fh);
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Detected encoding %s for file \"%s\""), s_encodingName[m_fileEncoding], fname);
      }

      if (!readFromStart && !m_rescan)
      {
         if (firstRead)
         {
            if (startOffset > 0)
            {
               _lseek(fh, startOffset, SEEK_SET);
               startOffset = -1;
            }
            else
            {
               if (m_preallocatedFile)
                  SeekToZero(fh, getCharSize(), m_detectBrokenPrealloc);
               else
                  _lseek(fh, 0, SEEK_END);
            }
            firstRead = false;
         }
         else
         {
            _lseek(fh, lastPos, SEEK_SET);
            char buffer[4];
            int bytes = _read(fh, buffer, 4);
            if ((bytes == 4) && memcmp(buffer, "\x00\x00\x00\x00", 4))
            {
               _lseek(fh, -4, SEEK_CUR);
               nxlog_debug_tag(DEBUG_TAG, 6, _T("New data available in file \"%s\""), fname);
            }
            else
            {
               off_t pos = _lseek(fh, -bytes, SEEK_CUR);
               if (pos > 0)
               {
                  int readSize = std::min(pos, (off_t)4);
                  _lseek(fh, -readSize, SEEK_CUR);
                  int bytes = _read(fh, buffer, readSize);
                  if ((bytes == readSize) && !memcmp(buffer, "\x00\x00\x00\x00", readSize))
                  {
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("Detected reset of preallocated file \"%s\""), fname);
                     _lseek(fh, 0, SEEK_SET);
                  }
               }
            }
         }
      }
      else
      {
         firstRead = false;   // Could be set together with readFromStart if file was missing for some time
      }
      readFromStart = false;

      lastPos = processNewRecords(fh, fname);
      _close(fh);
      size = static_cast<size_t>(st.st_size);
      m_offset = static_cast<off_t>(st.st_size);
      mtime = st.st_mtime;

      if (m_stopCondition.wait(m_fileCheckInterval))
         break;
   }

   nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" stopped"), m_fileName);
   return true;
}

/**
 * Scan file for changes without monitoring loop
 */
off_t LogParser::scanFile(int fh, off_t startOffset, const TCHAR *fileName)
{
   if (m_fileEncoding == LP_FCP_AUTO)
   {
      m_fileEncoding = ScanFileEncoding(fh);
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Detected encoding %s for file \"%s\""), s_encodingName[m_fileEncoding], m_fileName);
   }

   _lseek(fh, startOffset, SEEK_SET);

   char buffer[4];
   int bytes = _read(fh, buffer, 4);
   if ((bytes == 4) && memcmp(buffer, "\x00\x00\x00\x00", 4))
   {
      _lseek(fh, -4, SEEK_CUR);
      nxlog_debug_tag(DEBUG_TAG, 6, _T("New data available in file \"%s\""), m_fileName);
   }
   else
   {
      off_t pos = _lseek(fh, -bytes, SEEK_CUR);
      if (pos > 0)
      {
         int readSize = std::min(pos, (off_t)4);
         _lseek(fh, -readSize, SEEK_CUR);
         int bytes = _read(fh, buffer, readSize);
         if ((bytes == readSize) && !memcmp(buffer, "\x00\x00\x00\x00", readSize))
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Detected reset of preallocated file \"%s\""), m_fileName);
            _lseek(fh, 0, SEEK_SET);
         }
      }
   }

   return processNewRecords(fh, fileName);
}

#ifdef _WIN32

/**
 * File parser thread (using VSS snapshots)
 */
bool LogParser::monitorFileWithSnapshot(off_t startOffset)
{
   HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   if (FAILED(hr))
   {
      _com_error err(hr);
      nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread will not start, COM initialization failed (%s)"), err.ErrorMessage());
      return false;
   }

   size_t size = 0;
   time_t mtime = 0;
   time_t ctime = 0;
   off_t lastPos = 0;
   bool readFromStart = m_rescan;
   bool firstRead = true;

   nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" started (using VSS snapshots)"), m_fileName);
   bool exclusionPeriod = false;
   while(true)
   {
      if (isExclusionPeriod())
      {
         if (!exclusionPeriod)
         {
            exclusionPeriod = true;
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Will not open file \"%s\" because of exclusion period"), getFileName());
            setStatus(LPS_SUSPENDED);
         }
         if (m_stopCondition.wait(30000))
            break;
         continue;
      }

      if (exclusionPeriod)
      {
         exclusionPeriod = false;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Exclusion period for file \"%s\" ended"), getFileName());
      }

      TCHAR fname[MAX_PATH];
      ExpandFileName(getFileName(), fname, MAX_PATH, true);

      NX_STAT_STRUCT st;
      if (CALL_STAT(fname, &st) != 0)
      {
         setStatus(LPS_NO_FILE);
         if (m_stopCondition.wait(10000))
            break;
         continue;
      }

      if (firstRead)
         ctime = st.st_ctime; // prevent incorrect rotation detection on first read

      if ((size == st.st_size) && (mtime == st.st_mtime) && (ctime == st.st_ctime) && !readFromStart)
      {
         if (m_stopCondition.wait(m_fileCheckInterval))
            break;
         continue;
      }

      FileSnapshot *snapshot = CreateFileSnapshot(fname);
      if (snapshot == nullptr)
      {
         setStatus(LPS_VSS_FAILURE);
         if (m_stopCondition.wait(30000))  // retry in 30 seconds
            break;
         continue;
      }

      int fh = _tsopen(snapshot->name, O_RDONLY, _SH_DENYNO);
      if (fh == -1)
      {
         DestroyFileSnapshot(snapshot);
         setStatus(LPS_OPEN_ERROR);
         if (m_stopCondition.wait(10000))  // retry in 10 seconds
            break;
         continue;
      }

      setStatus(LPS_RUNNING);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("File \"%s\" (pattern \"%s\", snapshot \"%s\") successfully opened"), fname, m_fileName, snapshot->name);

      if ((size > static_cast<size_t>(st.st_size)) || (ctime != st.st_ctime))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("File \"%s\" rotation detected (size=%llu/%llu, ctime=%llu/%llu)"),  fname,
            static_cast<UINT64>(size), static_cast<UINT64>(st.st_size), static_cast<UINT64>(ctime), static_cast<UINT64>(st.st_ctime));
         readFromStart = true;   // Assume file rotation
         ctime = st.st_ctime;
      }

      if (m_fileEncoding == LP_FCP_AUTO)
      {
         m_fileEncoding = ScanFileEncoding(fh);
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Detected encoding %s for file \"%s\""), s_encodingName[m_fileEncoding], fname);
      }

      if (!readFromStart && !m_rescan)
      {
         if (firstRead)
         {
            if (startOffset > 0)
            {
               _lseek(fh, startOffset, SEEK_SET);
               startOffset = -1;
            }
            else
            {
               if (m_preallocatedFile)
                  SeekToZero(fh, getCharSize(), m_detectBrokenPrealloc);
               else
                  _lseek(fh, 0, SEEK_END);
            }
            firstRead = false;
         }
         else
         {
            _lseek(fh, lastPos, SEEK_SET);
         }
      }
      readFromStart = false;

      lastPos = processNewRecords(fh, fname);
      _close(fh);
      size = static_cast<size_t>(st.st_size);
      mtime = st.st_mtime;
      m_offset = lastPos;

      DestroyFileSnapshot(snapshot);
      if (m_stopCondition.wait(m_fileCheckInterval))
         break;
   }

   CoUninitialize();
   nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" stopped"), m_fileName);
   return true;
}

#endif   /* _WIN32 */
