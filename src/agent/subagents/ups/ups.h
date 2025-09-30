/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2021 Victor Kirhenshtein
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
#define UPS_PROTOCOL_MEC0003     7

#define BCMXCP_BUFFER_SIZE       1024
#define BCMXCP_MAP_SIZE          128

#define METASYS_BUFFER_SIZE      256

#define UPF_NOT_SUPPORTED        ((DWORD)0x00000001)
#define UPF_NULL_VALUE           ((DWORD)0x00000002)

#define UPS_DEBUG_TAG            _T("ups")

/**
 * UPS parameter structure
 */
struct UPS_PARAMETER
{
   uint32_t flags;
   char value[MAX_RESULT_LENGTH];
};

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
   Mutex m_mutex;
   Condition m_condStop;
   THREAD m_commThread;
   int m_id;

   void commThread();

protected:
   TCHAR *m_device;
   TCHAR *m_name;
   bool m_isConnected;
   UPS_PARAMETER m_paramList[UPS_PARAM_COUNT];

   void setConnected() { m_isConnected = true; }
   void setNullFlag(int parameterIndex, bool value)
   { 
      if (value)
         m_paramList[parameterIndex].flags |= UPF_NULL_VALUE;
      else
         m_paramList[parameterIndex].flags &= ~UPF_NULL_VALUE;
   }

   virtual bool open();
   virtual void close();
   virtual bool validateConnection();

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

   int getId() const { return m_id; }
   const TCHAR *getDevice() const { return m_device; }
   const TCHAR *getName() const { return CHECK_NULL(m_name); }
   virtual const TCHAR *getType() const { return _T("GENERIC"); }
   bool isConnected() const { return m_isConnected; }

   void setName(const char *name);
#ifdef UNICODE
   void setName(const WCHAR *name);
#endif
   void setId(int id) { m_id = id; }

   void startCommunication();

   LONG getParameter(int parameterIndex, TCHAR *value);

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

   virtual bool open() override;
   virtual void close() override;

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
   virtual bool open() override;
   virtual bool validateConnection() override;

   void queryParameter(const char *pszRq, UPS_PARAMETER *p, int nType, int chSep);

   virtual void queryModel() override;
   virtual void queryFirmwareVersion() override;
   virtual void queryMfgDate() override;
   virtual void querySerialNumber() override;
   virtual void queryTemperature() override;
   virtual void queryBatteryVoltage() override;
   virtual void queryNominalBatteryVoltage() override;
   virtual void queryBatteryLevel() override;
   virtual void queryInputVoltage() override;
   virtual void queryOutputVoltage() override;
   virtual void queryLineFrequency() override;
   virtual void queryPowerLoad() override;
   virtual void queryEstimatedRuntime() override;
   virtual void queryOnlineStatus() override;

public:
   APCInterface(const TCHAR *device);

   virtual const TCHAR *getType() const override { return _T("APC"); }
};

/**
 * Megatec UPS interface (serial)
 */
class MegatecInterface : public SerialInterface
{
private:
   double m_packs;

   void calculatePacks(double nominalVoltage, double actualVoltage);

protected:
   virtual bool open() override;
   virtual bool validateConnection() override;

public:
   MegatecInterface(const TCHAR *device);

   virtual const TCHAR *getType() const override { return _T("MEGATEC"); }

   virtual void queryStaticData() override;
   virtual void queryDynamicData() override;
};

#ifdef _WIN32

/**
 * MEC0003 UPS interface (USB via HID string descriptors)
 */
class MEC0003Interface : public UPSInterface
{
private:
   HANDLE m_hDev;
   TCHAR *m_instanceId;
   double m_packs;

   void calculatePacks(double nominalVoltage, double actualVoltage);
   bool queryStringDescriptor(UCHAR index, char *buffer, size_t bufferSize);

protected:
   virtual bool open() override;
   virtual void close() override;
   virtual bool validateConnection() override;

public:
   MEC0003Interface(const TCHAR *device);
   virtual ~MEC0003Interface();

   virtual const TCHAR *getType() const override { return _T("MEC0003"); }

   virtual void queryStaticData() override;
   virtual void queryDynamicData() override;

   static void enumerateDevices();
};

#endif

/**
 * BCMXCP meter map entry
 */
struct BCMXCP_METER_MAP_ENTRY
{
   int format;
   int offset;
};

/**
 * BCMXCP-compatible UPS interface
 */
class BCMXCPInterface : public SerialInterface
{
protected:
   BYTE m_data[BCMXCP_BUFFER_SIZE];
   BCMXCP_METER_MAP_ENTRY m_map[BCMXCP_MAP_SIZE];

   virtual bool open() override;
   virtual bool validateConnection() override;

   bool sendReadCommand(BYTE command);
   int recvData(int command);
   void readParameter(int index, int format, UPS_PARAMETER *param);

   virtual void queryTemperature() override;
   virtual void queryLineFrequency() override;
   virtual void queryBatteryLevel() override;
   virtual void queryInputVoltage() override;
   virtual void queryOutputVoltage() override;
   virtual void queryBatteryVoltage() override;
   virtual void queryEstimatedRuntime() override;
   virtual void queryModel() override;
   virtual void queryFirmwareVersion() override;
   virtual void querySerialNumber() override;
   virtual void queryOnlineStatus() override;
   virtual void queryPowerLoad() override;

public:
   BCMXCPInterface(const TCHAR *device);

   virtual const TCHAR *getType() const override { return _T("BCMXCP"); }
};

/**
 * BCMXCP-compatible UPS interface
 */
class MetaSysInterface : public SerialInterface
{
protected:
   BYTE m_data[METASYS_BUFFER_SIZE];
   int m_nominalPower;

   virtual bool open() override;
   virtual bool validateConnection() override;

   bool sendReadCommand(BYTE command);
   int recvData(int command);
   void readParameter(int command, int offset, int format, UPS_PARAMETER *param);
   void parseModelId();

   virtual void queryTemperature() override;
   virtual void queryInputVoltage() override;
   virtual void queryOutputVoltage() override;
   virtual void queryBatteryVoltage() override;
   virtual void queryModel() override;
   virtual void queryFirmwareVersion() override;
   virtual void querySerialNumber() override;
   virtual void queryOnlineStatus() override;
   virtual void queryPowerLoad() override;

public:
   MetaSysInterface(const TCHAR *device);

   virtual const TCHAR *getType() const override { return _T("METASYS"); }
};

/**
 * Microdowell UPS interface
 */
class MicrodowellInterface : public SerialInterface
{
protected:
   bool m_ge2kva;

	virtual bool open() override;
	virtual bool validateConnection() override;

	bool sendCmd(const char *cmd, int cmdLen, char *ret, int *retLen);

   virtual void queryModel() override;
   virtual void queryFirmwareVersion() override;
   virtual void queryMfgDate() override;
   virtual void querySerialNumber() override;
   virtual void queryTemperature() override;
   virtual void queryBatteryVoltage() override;
   virtual void queryNominalBatteryVoltage() override;
   virtual void queryBatteryLevel() override;
   virtual void queryInputVoltage() override;
   virtual void queryOutputVoltage() override;
   virtual void queryLineFrequency() override;
   virtual void queryPowerLoad() override;
   virtual void queryEstimatedRuntime() override;
   virtual void queryOnlineStatus() override;

public:
   MicrodowellInterface(const TCHAR *device);

   virtual const TCHAR *getType() const override { return _T("MICRODOWELL"); }
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

   virtual bool open() override;
   virtual void close() override;
   virtual bool validateConnection() override;

   LONG readIndexedString(USAGE nPage, USAGE uUsage, char *pszBuffer, int nBufLen);
   LONG readInt(USAGE nPage, USAGE nUsage, LONG *pnValue);

   void readStringParam(USAGE nPage, USAGE nUsage, UPS_PARAMETER *pParam);
   void readIntParam(USAGE nPage, USAGE nUsage, UPS_PARAMETER *pParam, int nDiv, BOOL bDouble);

   virtual void queryModel() override;
   virtual void querySerialNumber() override;
   virtual void queryBatteryVoltage() override;
   virtual void queryNominalBatteryVoltage() override;
   virtual void queryBatteryLevel() override;
   virtual void queryPowerLoad() override;
   virtual void queryEstimatedRuntime() override;
   virtual void queryOnlineStatus() override;

public:
   USBInterface(const TCHAR *device);
   virtual ~USBInterface();

   virtual const TCHAR *getType() const override { return _T("USB"); }
};

#endif


#endif
