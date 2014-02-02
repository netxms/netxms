/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2014 Alex Kirhenshtein
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

#include "ups.h"

#define VAL(x) ((unsigned char)buff[x+1] + (unsigned char)buff[x]*256)

/**
 * Constructor
 */
MicrodowellInterface::MicrodowellInterface(TCHAR *pszDevice) : SerialInterface(pszDevice)
{
	if (m_portSpeed == 0)
		m_portSpeed = 19200;
}

/**
 * Prepare and send data packet
 */
BOOL MicrodowellInterface::sendCmd(const char *cmd, int cmdLen, char *ret, int *retLen)
{
	char buff[512];
	int i, crc;
	int c;

	*retLen = 0;
	cmdLen &= 0xFF;
	buff[0] = '[';
	buff[1] = cmdLen;
	memcpy(buff + 2, cmd, cmdLen);
	for (i = 1, crc = 0; cmdLen >= 0; i++, cmdLen--)
	{
		crc ^= buff[i];
	}
	buff[i++] = crc;

   m_serial.write(buff, i);

	for (c = 0; c != '['; )
	{
		if (m_serial.read((char *)&c, 1) != 1)
		{
			return FALSE;
		}
	}

	if (m_serial.read(buff, i + 1) < i + 1)
	{
		return FALSE;
	}

	for (crc = i, c = 0; c < i; c++)
	{
		crc ^= buff[c];
	}

	if (crc != buff[c])
	{
		return FALSE;
	}

	memcpy(ret, buff, i);
	*retLen = i;

	return TRUE;
}

/**
 * Open device
 */
BOOL MicrodowellInterface::open()
{
   BOOL bRet = FALSE;

   if (!SerialInterface::open())
      return FALSE;

   m_serial.setTimeout(1000);
   m_serial.set(m_portSpeed, m_dataBits, m_parity, m_stopBits);

	char buff[512];
	int len;
	if (sendCmd("\x50\x80\x08", 3, buff, &len))
	{
		buff[11] = 0;
		if (buff[3] != 'E' || buff[4] != 'N' || buff[5] != 'T')
		{
			AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("Unknown Microdowell UPS model on port %s (%hs)"), m_pszDevice, buff);
		}
		else
		{
			bRet = TRUE;
			setConnected();
		}

		ge2kva = buff[4] > '2' || ( buff[4] == '2' && buff[5] > '0' );
	}

   return bRet;
}

/**
 * Validate connection
 */
BOOL MicrodowellInterface::validateConnection()
{
   BOOL bRet = TRUE;
	char buff[512];
	int len;

	if (!sendCmd("\x00", 1, buff, &len))
	{
		bRet = FALSE;
	}

   return bRet;
}

/**
 * Get UPS model
 */
void MicrodowellInterface::queryModel()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_MODEL];

	char buff[512];
	int len;
	if (sendCmd("\x50\x80\x08", 3, buff, &len))
	{
		buff[11] = 0;
		strcpy(p->szValue, buff + 1);
      p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Get UPS firmware version
//

void MicrodowellInterface::queryFirmwareVersion()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_FIRMWARE];

	p->dwFlags |= UPF_NOT_SUPPORTED;
}


//
// Get manufacturing date
//

void MicrodowellInterface::queryMfgDate()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_MFG_DATE];

	char buff[512];
	int len;
	if (sendCmd("\x50\xA0\x08", 3, buff, &len))
	{
		buff[11] = 0;
		snprintf(p->szValue, MAX_RESULT_LENGTH, "%d/%d/%d", buff[4], buff[5], buff[3]);
      p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Get serial number
//

void MicrodowellInterface::querySerialNumber()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_SERIAL];
	char buff[512];
	int len;
	if (sendCmd("\x50\x98\x08", 3, buff, &len))
	{
		buff[11] = 0;
		strcpy(p->szValue, buff + 1);
		p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Get temperature inside UPS
//

void MicrodowellInterface::queryTemperature()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_TEMP];

	char buff[512];
	int len;
	if (sendCmd("\x01", 1, buff, &len))
	{
		snprintf(p->szValue, MAX_RESULT_LENGTH, "%.1f", ((float)VAL(11)  - 202.97) / 1.424051);
		p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Get battery voltage
//

void MicrodowellInterface::queryBatteryVoltage()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_BATTERY_VOLTAGE];

	char buff[512];
	int len;
	if (sendCmd("\x01", 1, buff, &len))
	{
		snprintf(p->szValue, MAX_RESULT_LENGTH, "%.1f", (float)VAL(3) / 36.4);
		p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Get nominal battery voltage
//

void MicrodowellInterface::queryNominalBatteryVoltage()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE];
	p->dwFlags |= UPF_NOT_SUPPORTED;
}


//
// Get battery level (in percents)
//

void MicrodowellInterface::queryBatteryLevel()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_BATTERY_LEVEL];

	char buff[512];
	int len;
	if (sendCmd("\x03", 1, buff, &len))
	{
		snprintf(p->szValue, MAX_RESULT_LENGTH, "%d", (int)buff[1]);
		p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Get input line voltage
//

void MicrodowellInterface::queryInputVoltage()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_INPUT_VOLTAGE];

	char buff[512];
	int len;
	if (sendCmd("\x01", 1, buff, &len))
	{
		snprintf(p->szValue, MAX_RESULT_LENGTH, "%.1f", (float)VAL(3) / 36.4);
		p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Get output voltage
//

void MicrodowellInterface::queryOutputVoltage()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_OUTPUT_VOLTAGE];

	char buff[512];
	int len;
	if (sendCmd("\x01", 1, buff, &len))
	{
		snprintf(p->szValue, MAX_RESULT_LENGTH, "%.1f", (float)VAL(7) / (ge2kva ? 63.8 : 36.4));
		p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Get line frequency (Hz)
//

void MicrodowellInterface::queryLineFrequency()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_LINE_FREQ];

	char buff[512];
	int len;
	if (sendCmd("\x03", 1, buff, &len) && VAL(8))
	{
		snprintf(p->szValue, MAX_RESULT_LENGTH, "%d", (int)(50000.0 / VAL(8)));
		p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Get UPS power load (in percents)
//

void MicrodowellInterface::queryPowerLoad()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_LOAD];

	char buff[512];
	int len;
	if (sendCmd("\x03", 1, buff, &len))
	{
		snprintf(p->szValue, MAX_RESULT_LENGTH, "%d", (int)buff[7]);
		p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Get estimated on-battery runtime
//

void MicrodowellInterface::queryEstimatedRuntime()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_EST_RUNTIME];

	char buff[512];
	int len;
	if (sendCmd("\x03", 1, buff, &len) && VAL(2) != 65535)
	{
		snprintf(p->szValue, MAX_RESULT_LENGTH, "%d", VAL(2));
		p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}


//
// Check if UPS is online or on battery power
//

void MicrodowellInterface::queryOnlineStatus()
{
	UPS_PARAMETER *p = &m_paramList[UPS_PARAM_ONLINE_STATUS];

	char buff[512];
	int len;
	if (sendCmd("\x00", 1, buff, &len))
	{
		p->szValue[1] = 0;
		p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
		if (buff[1] & 0x10) // online
		{
			p->szValue[0] = '0';
		}
		else if (buff[1] & 0x01) // on battery
		{
			p->szValue[0] = '1';
		}
		else if (buff[1] & 0x02) // low battery
		{
			p->szValue[0] = '2';
		}
		else // unknown
		{
			p->dwFlags |= UPF_NULL_VALUE;
		}
	}
	else
	{
		p->dwFlags |= UPF_NULL_VALUE;
	}
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.3  2007/04/17 19:04:54  alk
microdowell fixed(?). should test on a real hardware

Revision 1.2  2006/12/09 21:02:11  victor
New configure works on Gentoo Linux

Revision 1.1  2006/12/05 13:20:15  alk
Microdowell UPS support added


*/
