/*
** NetXMS UPS management subagent
** Copyright (C) 2006 Victor Kirhenshtein
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
** File: ups.h
**
**/

#ifndef _ups_h_
#define _ups_h_

#include <nms_common.h>
#include <nms_threads.h>
#include <nms_agent.h>
#include <nms_util.h>


//
// Constants
//

#define MAX_UPS_DEVICES    128

#define UPS_PROTOCOL_APC   1


//
// Abstract UPS interface class
//

class UPSInterface
{
protected:
   TCHAR *m_pszDevice;
   Serial m_serial;

   BOOL ReadLineFromSerial(char *pszBuffer, int nBufLen);

public:
   UPSInterface(TCHAR *pszDevice);
   virtual ~UPSInterface();

   const TCHAR *Device(void) { return m_pszDevice; }
   virtual TCHAR *Type(void) { return _T("GENERIC"); }

   virtual BOOL Open(void);
   virtual void Close(void);

   virtual LONG GetModel(TCHAR *pszBuffer);
   virtual LONG GetFirmwareVersion(TCHAR *pszBuffer);
   virtual LONG GetMfgDate(TCHAR *pszBuffer);
   virtual LONG GetSerialNumber(TCHAR *pszBuffer);
   virtual LONG GetTemperature(LONG *pnTemp);
   virtual LONG GetBatteryVoltage(double *pdVoltage);
   virtual LONG GetNominalBatteryVoltage(double *pdVoltage);
   virtual LONG GetBatteryLevel(LONG *pnLevel);
   virtual LONG GetInputVoltage(double *pdVoltage);
   virtual LONG GetOutputVoltage(double *pdVoltage);
   virtual LONG GetLineFrequency(LONG *pnFrequency);
   virtual LONG GetPowerLoad(LONG *pnLoad);
   virtual LONG GetEstimatedRuntime(LONG *pnMinutes);
};


//
// APC UPS interface
//

class APCInterface : public UPSInterface
{
public:
   APCInterface(TCHAR *pszDevice) : UPSInterface(pszDevice) { }

   virtual TCHAR *Type(void) { return _T("APC"); }

   virtual BOOL Open(void);

   virtual LONG GetModel(TCHAR *pszBuffer);
   virtual LONG GetFirmwareVersion(TCHAR *pszBuffer);
   virtual LONG GetMfgDate(TCHAR *pszBuffer);
   virtual LONG GetSerialNumber(TCHAR *pszBuffer);
   virtual LONG GetTemperature(LONG *pnTemp);
   virtual LONG GetBatteryVoltage(double *pdVoltage);
   virtual LONG GetNominalBatteryVoltage(double *pdVoltage);
   virtual LONG GetBatteryLevel(LONG *pnLevel);
   virtual LONG GetInputVoltage(double *pdVoltage);
   virtual LONG GetOutputVoltage(double *pdVoltage);
   virtual LONG GetLineFrequency(LONG *pnFrequency);
   virtual LONG GetPowerLoad(LONG *pnLoad);
   virtual LONG GetEstimatedRuntime(LONG *pnMinutes);
};


#endif
