/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2014 Victor Kirhenshtein
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

/**
 * Constants
 */
#define MAX_UPS_DEVICES          128

#define UPS_PROTOCOL_APC         1
#define UPS_PROTOCOL_BCMXCP      2
#define UPS_PROTOCOL_METASYS	   3
#define UPS_PROTOCOL_MICRODOWELL	4
#define UPS_PROTOCOL_USB         5
#define UPS_PROTOCOL_MEGATEC     6

#define BCMXCP_BUFFER_SIZE       1024
#define BCMXCP_MAP_SIZE          128

#define METASYS_BUFFER_SIZE      256

#define UPF_NOT_SUPPORTED        ((DWORD)0x00000001)
#define UPF_NULL_VALUE           ((DWORD)0x00000002)

/**
 * UPS parameter structure
 */
typedef struct
{
   DWORD dwFlags;
   char szValue[MAX_RESULT_LENGTH];
} UPS_PARAMETER;

/**
 * UPS parameter codes
 */
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

/**
 * Abstract UPS interface class
 */
class UPSInterface
{
private:
   MUTEX m_mutex;
   CONDITION m_condStop;
   THREAD m_thCommThread;
   int m_nIndex;

   static THREAD_RESULT THREAD_CALL commThreadStarter(void *pArg);
   void commThread();

protected:
   TCHAR *m_device;
   TCHAR *m_pszName;
   BOOL m_bIsConnected;
   UPS_PARAMETER m_paramList[UPS_PARAM_COUNT];

   void setConnected() { m_bIsConnected = TRUE; }
   void setNullFlag(int nParam, BOOL bFlag) 
   { 
      if (bFlag)
         m_paramList[nParam].dwFlags |= UPF_NULL_VALUE;
      else
         m_paramList[nParam].dwFlags &= ~UPF_NULL_VALUE;
   }

   virtual BOOL open();
   virtual void close();
   virtual BOOL validateConnection();

   virtual void queryModel();
   virtual void queryFirmwareVersion();
   virtual void queryMfgDate();
   virtual void querySerialNumber();
   virtual void queryTemperature();
   virtual void queryBatteryVoltage();
   virtual void queryNominalBatteryVoltage();
   virtual void queryBatteryLevel();
   virtual void queryInputVoltage();
   virtual void queryOutputVoltage();
   virtual void queryLineFrequency();
   virtual void queryPowerLoad();
   virtual void queryEstimatedRuntime();
   virtual void queryOnlineStatus();

public:
   UPSInterface(const TCHAR *device);
   virtual ~UPSInterface();

   const TCHAR *getDevice() { return m_device; }
   const TCHAR *getName() { return CHECK_NULL(m_pszName); }
   virtual const TCHAR *getType() { return _T("GENERIC"); }
   BOOL isConnected() { return m_bIsConnected; }

   void setName(const char *pszName);
#ifdef UNICODE
   void setName(const WCHAR *pszName);
#endif
   void setIndex(int nIndex) { m_nIndex = nIndex; }

   void startCommunication();

   LONG getParameter(int nParam, TCHAR *pszValue);

   virtual void queryStaticData();
   virtual void queryDynamicData();
};

/**
 * UPS with serial interface
 */
class SerialInterface : public UPSInterface
{
protected:
   Serial m_serial;
	int m_portSpeed;
	int m_dataBits;
	int m_parity;
	int m_stopBits;

   virtual BOOL open();
   virtual void close();

   bool readLineFromSerial(char *buffer, size_t bufLen, char eol = '\n');

public:
   SerialInterface(const TCHAR *device);
};

/**
 * APC UPS interface
 */
class APCInterface : public SerialInterface
{
protected:
   virtual BOOL open();
   virtual BOOL validateConnection();

   void queryParameter(const char *pszRq, UPS_PARAMETER *p, int nType, int chSep);

   virtual void queryModel();
   virtual void queryFirmwareVersion();
   virtual void queryMfgDate();
   virtual void querySerialNumber();
   virtual void queryTemperature();
   virtual void queryBatteryVoltage();
   virtual void queryNominalBatteryVoltage();
   virtual void queryBatteryLevel();
   virtual void queryInputVoltage();
   virtual void queryOutputVoltage();
   virtual void queryLineFrequency();
   virtual void queryPowerLoad();
   virtual void queryEstimatedRuntime();
   virtual void queryOnlineStatus();

public:
   APCInterface(const TCHAR *device);

   virtual const TCHAR *getType() { return _T("APC"); }
};

/**
 * APC UPS interface
 */
class MegatecInterface : public SerialInterface
{
private:
   double m_packs;

   void calculatePacks(double nominalVoltage, double actualVoltage);

protected:
   virtual BOOL open();
   virtual BOOL validateConnection();

public:
   MegatecInterface(const TCHAR *device);

   virtual const TCHAR *getType() { return _T("MEGATEC"); }

   virtual void queryStaticData();
   virtual void queryDynamicData();
};

/**
 * BCMXCP meter map entry
 */
struct BCMXCP_METER_MAP_ENTRY
{
   int nFormat;
   int nOffset;
};

/**
 * BCMXCP-compatible UPS interface
 */
class BCMXCPInterface : public SerialInterface
{
protected:
   BYTE m_data[BCMXCP_BUFFER_SIZE];
   BCMXCP_METER_MAP_ENTRY m_map[BCMXCP_MAP_SIZE];

   virtual BOOL open();
   virtual BOOL validateConnection();

   BOOL sendReadCommand(BYTE nCommand);
   int recvData(int nCommand);
   void readParameter(int nIndex, int nFormat, UPS_PARAMETER *pParam);

   virtual void queryTemperature();
   virtual void queryLineFrequency();
   virtual void queryBatteryLevel();
   virtual void queryInputVoltage();
   virtual void queryOutputVoltage();
   virtual void queryBatteryVoltage();
   virtual void queryEstimatedRuntime();
   virtual void queryModel();
   virtual void queryFirmwareVersion();
   virtual void querySerialNumber();
   virtual void queryOnlineStatus();
   virtual void queryPowerLoad();

public:
   BCMXCPInterface(const TCHAR *device);

   virtual const TCHAR *getType() { return _T("BCMXCP"); }
};

/**
 * BCMXCP-compatible UPS interface
 */
class MetaSysInterface : public SerialInterface
{
protected:
   BYTE m_data[METASYS_BUFFER_SIZE];
   int m_nominalPower;

   virtual BOOL open();
   virtual BOOL validateConnection();

   BOOL sendReadCommand(BYTE nCommand);
   int recvData(int nCommand);
   void readParameter(int command, int offset, int format, UPS_PARAMETER *param);
   void parseModelId();

   virtual void queryTemperature();
   virtual void queryInputVoltage();
   virtual void queryOutputVoltage();
   virtual void queryBatteryVoltage();
   virtual void queryModel();
   virtual void queryFirmwareVersion();
   virtual void querySerialNumber();
   virtual void queryOnlineStatus();
   virtual void queryPowerLoad();

public:
   MetaSysInterface(const TCHAR *device);

   virtual const TCHAR *getType() { return _T("METASYS"); }
};

/**
 * Microdowell UPS interface
 */
class MicrodowellInterface : public SerialInterface
{
protected:
	virtual BOOL open();
	virtual BOOL validateConnection();

	BOOL sendCmd(const char *cmd, int cmdLen, char *ret, int *retLen);

	int ge2kva;

   virtual void queryModel();
   virtual void queryFirmwareVersion();
   virtual void queryMfgDate();
   virtual void querySerialNumber();
   virtual void queryTemperature();
   virtual void queryBatteryVoltage();
   virtual void queryNominalBatteryVoltage();
   virtual void queryBatteryLevel();
   virtual void queryInputVoltage();
   virtual void queryOutputVoltage();
   virtual void queryLineFrequency();
   virtual void queryPowerLoad();
   virtual void queryEstimatedRuntime();
   virtual void queryOnlineStatus();

public:
   MicrodowellInterface(const TCHAR *device);

   virtual const TCHAR *getType() { return _T("MICRODOWELL"); }
};

#ifdef _WIN32

/**
 * UPS with USB interface
 */
class USBInterface : public UPSInterface
{
protected:
   HANDLE m_hDev;
   HIDP_CAPS m_deviceCaps;
   PHIDP_PREPARSED_DATA m_pPreparsedData;
   HIDP_VALUE_CAPS *m_pFeatureValueCaps;

   virtual BOOL open();
   virtual void close();
   virtual BOOL validateConnection();

   LONG readIndexedString(USAGE nPage, USAGE uUsage, char *pszBuffer, int nBufLen);
   LONG readInt(USAGE nPage, USAGE nUsage, LONG *pnValue);

   void readStringParam(USAGE nPage, USAGE nUsage, UPS_PARAMETER *pParam);
   void readIntParam(USAGE nPage, USAGE nUsage, UPS_PARAMETER *pParam, int nDiv, BOOL bDouble);

   virtual void queryModel();
   virtual void querySerialNumber();
   virtual void queryBatteryVoltage();
   virtual void queryNominalBatteryVoltage();
   virtual void queryBatteryLevel();
   virtual void queryPowerLoad();
   virtual void queryEstimatedRuntime();
   virtual void queryOnlineStatus();

public:
   USBInterface(const TCHAR *device);
   virtual ~USBInterface();

   virtual const TCHAR *getType() { return _T("USB"); }
};

#endif


#endif
