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

#ifdef _WIN32

#ifdef __cplusplus
extern "C" {
#endif

#include <hidsdi.h>

#ifdef __cplusplus
}
#endif

#endif


//
// Constants
//

#define MAX_UPS_DEVICES    128

#define UPS_PROTOCOL_APC   1
#define UPS_PROTOCOL_USB   2


//
// Abstract UPS interface class
//

class UPSInterface
{
protected:
   TCHAR *m_pszDevice;
   TCHAR *m_pszName;
   BOOL m_bIsConnected;

   void SetConnected(void) { m_bIsConnected = TRUE; }

public:
   UPSInterface(TCHAR *pszDevice);
   virtual ~UPSInterface();

   const TCHAR *Device(void) { return m_pszDevice; }
   const TCHAR *Name(void) { return CHECK_NULL(m_pszName); }
   virtual TCHAR *Type(void) { return _T("GENERIC"); }
   BOOL IsConnected(void) { return m_bIsConnected; }

   void SetName(TCHAR *pszName);

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
// UPS with serial interface
//

class SerialInterface : public UPSInterface
{
protected:
   Serial m_serial;

   BOOL ReadLineFromSerial(char *pszBuffer, int nBufLen);

public:
   SerialInterface(TCHAR *pszDevice) : UPSInterface(pszDevice) { }

   virtual BOOL Open(void);
   virtual void Close(void);
};


//
// APC UPS interface
//

class APCInterface : public SerialInterface
{
public:
   APCInterface(TCHAR *pszDevice) : SerialInterface(pszDevice) { }

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


//
// UPS with USB interface
//

#ifdef _WIN32

class USBInterface : public UPSInterface
{
protected:
   HANDLE m_hDev;
   HIDP_CAPS m_deviceCaps;
   PHIDP_PREPARSED_DATA m_pPreparsedData;
   HIDP_VALUE_CAPS *m_pFeatureValueCaps;

   LONG ReadIndexedString(USAGE nPage, USAGE uUsage, TCHAR *pszBuffer, int nBufLen);
   LONG ReadInt(USAGE nPage, USAGE nUsage, LONG *pnValue);

public:
   USBInterface(TCHAR *pszDevice);
   virtual ~USBInterface();

   virtual TCHAR *Type(void) { return _T("USB"); }

   virtual BOOL Open(void);
   virtual void Close(void);

   virtual LONG GetModel(TCHAR *pszBuffer);
   virtual LONG GetSerialNumber(TCHAR *pszBuffer);
   virtual LONG GetBatteryVoltage(double *pdVoltage);
   virtual LONG GetNominalBatteryVoltage(double *pdVoltage);
   virtual LONG GetBatteryLevel(LONG *pnLevel);
   virtual LONG GetPowerLoad(LONG *pnLoad);
   virtual LONG GetEstimatedRuntime(LONG *pnMinutes);
};

#endif


#endif
