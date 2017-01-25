/*
** NetXMS - Network Management System
** Copyright (C) 2017 Raden Solutions
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
** File: dfile_info.cpp
**
**/

#include "nxagentd.h"

#ifdef _WIN32
#define write	_write
#define close	_close
#endif

/**
 * Constructor for DownloadFileInfo class only stores given data
 */
DownloadFileInfo::DownloadFileInfo(const TCHAR *name, time_t lastModTime)
{
   m_fileName = _tcsdup(name);
   m_lastModTime = lastModTime;
}

/**
 * Destructor
 */
DownloadFileInfo::~DownloadFileInfo()
{
   if(m_file != -1)
      close(false);

   delete m_fileName;
}

/**
 * Opens file and returns if it was successfully
 */
bool DownloadFileInfo::open()
{
   m_file = ::_topen(m_fileName, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
   return m_file != -1;
}

/**
 * Function that writes incoming data to file
 */
bool DownloadFileInfo::write(const BYTE *data, int dataSize)
{
   return ::write(m_file, data, dataSize) == dataSize;
}

/**
 * Closes file and changes it's date if required
 */
void DownloadFileInfo::close(bool success)
{
   ::close(m_file);
   m_file = -1;

   if(m_lastModTime != 0)
      SetLastModificationTime(m_fileName, m_lastModTime);

   // Remove received file in case of failure
   if (!success)
      _tunlink(m_fileName);
}
