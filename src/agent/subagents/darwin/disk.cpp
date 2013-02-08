/* 
** NetXMS subagent for Darwin/OS X
** Copyright (C) 2012 Alex Kirhenshtein
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
**/

#include <nms_common.h>
#include <nms_agent.h>

#include <sys/param.h>
#include <sys/mount.h>

#include "disk.h"

LONG H_DiskInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	char szArg[512] = {0};
	struct statfs s;

	AgentGetParameterArgA(pszParam, 1, szArg, sizeof(szArg));

	if (szArg[0] != 0 && statfs(szArg, &s) == 0)
	{
		nRet = SYSINFO_RC_SUCCESS;

		QWORD usedBlocks = (QWORD)(s.f_blocks - s.f_bfree);
		QWORD totalBlocks = (QWORD)s.f_blocks;
		QWORD blockSize = (QWORD)s.f_bsize;
		QWORD freeBlocks = (QWORD)s.f_bfree;
		QWORD availableBlocks = (QWORD)s.f_bavail;
		
		switch((long)pArg)
		{
			case DISK_TOTAL:
				ret_uint64(pValue, totalBlocks * blockSize);
				break;
			case DISK_USED:
				ret_uint64(pValue, usedBlocks * blockSize);
				break;
			case DISK_FREE:
				ret_uint64(pValue, freeBlocks * blockSize);
				break;
			case DISK_AVAIL:
				ret_uint64(pValue, availableBlocks * blockSize);
				break;
			case DISK_USED_PERC:
				ret_double(pValue, (usedBlocks * 100) / totalBlocks);
				break;
			case DISK_AVAIL_PERC:
				ret_double(pValue, (availableBlocks * 100) / totalBlocks);
				break;
			case DISK_FREE_PERC:
				ret_double(pValue, (freeBlocks * 100) / totalBlocks);
				break;
			default:
				nRet = SYSINFO_RC_ERROR;
				break;
		}
	}

	return nRet;
}


LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *table)
{
  LONG rc = SYSINFO_RC_SUCCESS;

  struct statfs *sf;

  int count = getmntinfo(&sf, MNT_NOWAIT);
  if (count > 0)
  {
    table->addColumn(_T("MOUNTPOINT"), DCI_DT_STRING);
    table->addColumn(_T("VOLUME"), DCI_DT_STRING);
    table->addColumn(_T("LABEL"), DCI_DT_STRING);
    table->addColumn(_T("FSTYPE"), DCI_DT_STRING);
    table->addColumn(_T("SIZE.TOTAL"), DCI_DT_UINT64);
    table->addColumn(_T("SIZE.FREE"), DCI_DT_UINT64);
    table->addColumn(_T("SIZE.FREE.PCT"), DCI_DT_FLOAT);
    table->addColumn(_T("SIZE.AVAIL"), DCI_DT_UINT64);
    table->addColumn(_T("SIZE.AVAIL.PCT"), DCI_DT_FLOAT);
    table->addColumn(_T("SIZE.USED"), DCI_DT_UINT64);
    table->addColumn(_T("SIZE.USED.PCT"), DCI_DT_FLOAT);
  }
  else
  {
    rc = SYSINFO_RC_ERROR;
  }

  for (int i = 0; i < count; i++)
  {
    if (!(sf[i].f_flags & MNT_DONTBROWSE)) // ignore /dev, autofs, etc.
    {
      int blockSize = sf[i].f_bsize;
      INT64 sizeTotal = sf[i].f_blocks * blockSize;
      INT64 sizeFree = sf[i].f_bfree * blockSize;
      INT64 sizeAvailable = sf[i].f_bavail * blockSize;
      INT64 sizeUsed = sizeTotal - sizeFree;

      table->addRow();

      table->set(0, sf[i].f_mntonname); // mountpoint
      table->set(1, sf[i].f_mntfromname); // volume
      table->set(2, ""); // label
      table->set(3, sf[i].f_fstypename); // fstype
      table->set(4, sizeTotal); // size.total
      table->set(5, sizeFree); // size.free
      table->set(6, 100.0 * sizeFree / sizeTotal); // size.free.pct
      table->set(7, sizeAvailable); // size.avail
      table->set(8, 100.0 * sizeAvailable / sizeTotal); // size.avail.pct
      table->set(9, sizeUsed); // size.used
      table->set(10, 100.0 * sizeUsed / sizeTotal); // size.used.pct
    }
  }

  return rc;
}

LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value)
{
  LONG rc = SYSINFO_RC_SUCCESS;

  struct statfs *sf;

  int count = getmntinfo(&sf, MNT_NOWAIT);
  if (count > 0)
  {
    for (int i = 0; i < count; i++)
    {
      if (!(sf[i].f_flags & MNT_DONTBROWSE)) // ignore /dev, autofs, etc.
      {
        value->add(sf[i].f_mntonname); // mountpoint
        //table->set(1, sf[i].f_mntfromname); // volume
      }
    }
  }
  else
  {
    rc = SYSINFO_RC_ERROR;
  }

  return rc;
}
