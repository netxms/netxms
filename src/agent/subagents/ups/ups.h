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
#include <nms_util.h>
#include <nms_agent.h>

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

#define MAX_UPS_DEVICES       128

#define UPS_PROTOCOL_APC      1
#define UPS_PROTOCOL_USB      2
#define UPS_PROTOCOL_BCMXCP   3
#define UPS_PROTOCOL_MICRODOWELL	4

#define BCMXCP_BUFFER_SIZE    1024
#define BCMXCP_MAP_SIZE       128

#define UPF_NOT_SUPPORTED     ((DWORD)0x00000001)
#define UPF_NULL_VALUE        ((DWORD)0x00000002)


//
// UPS parameter structure
//

typedef struct
{
   DWORD dwFlags;
   TCHAR szValue[MAX_RESULT_LENGTH];
} UPS_PARAMETER;


//
// UPS parameter codes
//

#define UPS_PARAM_MODEL                0
#define UPS_PARAM_FIRMWARE             1
#define UPS_PARAM_MFG_DATE             2
#define UPS_PARAM_SERIAL               3
#define UPS_PARAM_TEMP                 4
#define UPS_PARAM_BATTERY_VOLTAGE      5
#define UPS_PARAM_NOMINAL_BATT_VOLTAGE 6
#define UPS_PARAM_BATTERY_LEVEL        7
#define UPS_PARAM_INPUT_VOLTAGE        8
#define UPS_PARAM_OUTPUT_VOLTAGE       9
#define UPS_PARAM_LINE_FREQ            10
#define UPS_PARAM_LOAD                 11
#define UPS_PARAM_EST_RUNTIME          12
#define UPS_PARAM_ONLINE_STATUS        13

#define UPS_PARAM_COUNT                14


//
// Abstract UPS interface class
//

class UPSInterface
{
private:
   MUTEX m_mutex;
   CONDITION m_condStop;
   THREAD m_thCommThread;
   int m_nIndex;

   static THREAD_RESULT THREAD_CALL CommThreadStarter(void *pArg);
   void CommThread(void);

protected:
   TCHAR *m_pszDevice;
   TCHAR *m_pszName;
   BOOL m_bIsConnected;
   UPS_PARAMETER m_paramList[UPS_PARAM_COUNT];

   void SetConnected(void) { m_bIsConnected = TRUE; }
   void SetNullFlag(int nParam, BOOL bFlag) 
   { 
      if (bFlag)
         m_paramList[nParam].dwFlags |= UPF_NULL_VALUE;
      else
         m_paramList[nParam].dwFlags &= ~UPF_NULL_VALUE;
   }

   virtual BOOL Open(void);
   virtual void Close(void);
   virtual BOOL ValidateConnection(void);

public:
   UPSInterface(TCHAR *pszDevice);
   virtual ~UPSInterface();

   const TCHAR *Device(void) { return m_pszDevice; }
   const TCHAR *Name(void) { return CHECK_NULL(m_pszName); }
   virtual const TCHAR *Type(void) { return _T("GENERIC"); }
   BOOL IsConnected(void) { return m_bIsConnected; }

   void SetName(TCHAR *pszName);
   void SetIndex(int nIndex) { m_nIndex = nIndex; }

   void StartCommunication(void);

   LONG GetParameter(int nParam, TCHAR *pszValue);

   virtual void QueryModel();
   virtual void QueryFirmwareVersion();
   virtual void QueryMfgDate();
   virtual void QuerySerialNumber();
   virtual void QueryTemperature();
   virtual void QueryBatteryVoltage();
   virtual void QueryNominalBatteryVoltage();
   virtual void QueryBatteryLevel();
   virtual void QueryInputVoltage();
   virtual void QueryOutputVoltage();
   virtual void QueryLineFrequency();
   virtual void QueryPowerLoad();
   virtual void QueryEstimatedRuntime();
   virtual void QueryOnlineStatus();
};


//
// UPS with serial interface
//

class SerialInterface : public UPSInterface
{
protected:
   Serial m_serial;
	int m_portSpeed;
	int m_dataBits;
	int m_parity;
	int m_stopBits;

   virtual BOOL Open();
   virtual void Close();

   BOOL ReadLineFromSerial(char *pszBuffer, int nBufLen);

public:
   SerialInterface(TCHAR *pszDevice);
};


//
// APC UPS interface
//

class APCInterface : public SerialInterface
{
protected:
   virtual BOOL Open();
   virtual BOOL ValidateConnection();

   void QueryParameter(const char *pszRq, UPS_PARAMETER *p, int nType, int chSep);

public:
   APCInterface(TCHAR *pszDevice);

   virtual const TCHAR *Type() { return _T("APC"); }

   virtual void QueryModel();
   virtual void QueryFirmwareVersion();
   virtual void QueryMfgDate();
   virtual void QuerySerialNumber();
   virtual void QueryTemperature();
   virtual void QueryBatteryVoltage();
   virtual void QueryNominalBatteryVoltage();
   virtual void QueryBatteryLevel();
   virtual void QueryInputVoltage();
   virtual void QueryOutputVoltage();
   virtual void QueryLineFrequency();
   virtual void QueryPowerLoad();
   virtual void QueryEstimatedRuntime();
   virtual void QueryOnlineStatus();
};


//
// BCMXCP-compatible UPS interface
//

struct BCMXCP_METER_MAP_ENTRY
{
   int nFormat;
   int nOffset;
};

class BCMXCPInterface : public SerialInterface
{
protected:
   BYTE m_data[BCMXCP_BUFFER_SIZE];
   BCMXCP_METER_MAP_ENTRY m_map[BCMXCP_MAP_SIZE];

   virtual BOOL Open();
   virtual BOOL ValidateConnection();

   BOOL SendReadCommand(BYTE nCommand);
   int RecvData(int nCommand);
   void ReadParameter(int nIndex, int nFormat, UPS_PARAMETER *pParam);

public:
   BCMXCPInterface(TCHAR *pszDevice);

   virtual const TCHAR *Type() { return _T("BCMXCP"); }

   virtual void QueryTemperature(void);
   virtual void QueryLineFrequency(void);
   virtual void QueryBatteryLevel(void);
   virtual void QueryInputVoltage(void);
   virtual void QueryOutputVoltage(void);
   virtual void QueryBatteryVoltage(void);
   virtual void QueryEstimatedRuntime(void);
   virtual void QueryModel(void);
   virtual void QueryFirmwareVersion(void);
   virtual void QuerySerialNumber(void);
   virtual void QueryOnlineStatus(void);
   virtual void QueryPowerLoad(void);
};

//
// Microdowell UPS interface
//

class MicrodowellInterface : public SerialInterface
{
protected:
	virtual BOOL Open(void);
	virtual BOOL ValidateConnection(void);

	BOOL SendCmd(const char *cmd, int cmdLen, char *ret, int *retLen);

	int ge2kva;

public:
   MicrodowellInterface(TCHAR *pszDevice);

   virtual const TCHAR *Type(void) { return _T("MICRODOWELL"); }

   virtual void QueryModel(void);
   virtual void QueryFirmwareVersion(void);
   virtual void QueryMfgDate(void);
   virtual void QuerySerialNumber(void);
   virtual void QueryTemperature(void);
   virtual void QueryBatteryVoltage(void);
   virtual void QueryNominalBatteryVoltage(void);
   virtual void QueryBatteryLevel(void);
   virtual void QueryInputVoltage(void);
   virtual void QueryOutputVoltage(void);
   virtual void QueryLineFrequency(void);
   virtual void QueryPowerLoad(void);
   virtual void QueryEstimatedRuntime(void);
   virtual void QueryOnlineStatus(void);
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

   virtual BOOL Open(void);
   virtual void Close(void);
   virtual BOOL ValidateConnection(void);

   LONG ReadIndexedString(USAGE nPage, USAGE uUsage, TCHAR *pszBuffer, int nBufLen);
   LONG ReadInt(USAGE nPage, USAGE nUsage, LONG *pnValue);

   void ReadStringParam(USAGE nPage, USAGE nUsage, UPS_PARAMETER *pParam);
   void ReadIntParam(USAGE nPage, USAGE nUsage, UPS_PARAMETER *pParam, int nDiv, BOOL bDouble);

public:
   USBInterface(TCHAR *pszDevice);
   virtual ~USBInterface();

   virtual const TCHAR *Type(void) { return _T("USB"); }

   virtual void QueryModel(void);
   virtual void QuerySerialNumber(void);
   virtual void QueryBatteryVoltage(void);
   virtual void QueryNominalBatteryVoltage(void);
   virtual void QueryBatteryLevel(void);
   virtual void QueryPowerLoad(void);
   virtual void QueryEstimatedRuntime(void);
   virtual void QueryOnlineStatus(void);
};

#endif


#endif
