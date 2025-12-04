/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: session.cpp
**
**/

#include "ntcb.h"

/**
 * Parser for field 4
 */
static void FP_4(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.Controller.TestMode"), value.testBit8(0));
   pushValues->set(_T("NTCB.Controller.AlarmNotification"), value.testBit8(1));
   pushValues->set(_T("NTCB.Controller.Alarm"), value.testBit8(2));
   pushValues->set(_T("NTCB.Controller.Mode"), static_cast<int32_t>((value.u8 >> 3) & 0x03));
   pushValues->set(_T("NTCB.Controller.Evacuation"), value.testBit8(5));
   pushValues->set(_T("NTCB.Controller.PowerSavingMode"), value.testBit8(6));
   pushValues->set(_T("NTCB.Accelerometer.Calibration"), value.testBit8(7));
}

/**
 * Parser for field 5
 */
static void FP_5(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.GSM.Status"), value.testBit8(0));
   pushValues->set(_T("NTCB.USB.Status"), value.testBit8(1));
   pushValues->set(_T("NTCB.NavReceiver"), value.testBit8(2));
   pushValues->set(_T("NTCB.Controller.ClockSync"), value.testBit8(3));
   pushValues->set(_T("NTCB.GSM.ActiveSIM"), value.testBit8(4) + 1);
   pushValues->set(_T("NTCB.GSM.Registration"), value.testBit8(5));
   pushValues->set(_T("NTCB.GSM.Roaming"), value.testBit8(6));
   pushValues->set(_T("NTCB.EngineStatus"), value.testBit8(7));
}

/**
 * Parser for field 6
 */
static void FP_6(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.GSM.Interference"), static_cast<int32_t>(value.u8 & 0x03));
   pushValues->set(_T("NTCB.GNSS.Interference"), value.testBit8(2));
   pushValues->set(_T("NTCB.GNSS.ApproximationMode"), value.testBit8(3));
   pushValues->set(_T("NTCB.Accelerometer.Status"), value.testBit8(4));
   pushValues->set(_T("NTCB.Bluetooth.Status"), value.testBit8(5));
   pushValues->set(_T("NTCB.WiFi.Status"), value.testBit8(6));
}

/**
 * Parser for field 7
 * 0: -113 Дб/м или меньше
 * 1: -111 Дб/м
 * 2..30: -109..-53 Дб/м
 * 31: -51 Дб/м или больше
 * 99: нет сигнала сотовой сети.
 */
static void FP_7(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.GSM.SignalLevel"), (value.u8 <= 31) ? static_cast<int32_t>(value.u8) * 2 - 113 : 0);
}

/**
 * Parser for field 8
 */
static void FP_8(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.GPS.Status"), value.testBit8(0));
   pushValues->set(_T("NTCB.GPS.ValidCoordinates"), value.testBit8(1));
   pushValues->set(_T("NTCB.GPS.Satellites"), static_cast<uint32_t>(value.u8 >> 2));
}

/**
 * Parser for field 29
 */
static void FP_29(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.DigitalInput1.State"), value.testBit8(0));
   pushValues->set(_T("NTCB.DigitalInput2.State"), value.testBit8(1));
   pushValues->set(_T("NTCB.DigitalInput3.State"), value.testBit8(2));
   pushValues->set(_T("NTCB.DigitalInput4.State"), value.testBit8(3));
   pushValues->set(_T("NTCB.DigitalInput5.State"), value.testBit8(4));
   pushValues->set(_T("NTCB.DigitalInput6.State"), value.testBit8(5));
   pushValues->set(_T("NTCB.DigitalInput7.State"), value.testBit8(6));
   pushValues->set(_T("NTCB.DigitalInput8.State"), value.testBit8(7));
}

/**
 * Parser for field 30
 */
static void FP_30(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.DigitalInput9.State"), value.testBit8(0));
   pushValues->set(_T("NTCB.DigitalInput10.State"), value.testBit8(1));
   pushValues->set(_T("NTCB.DigitalInput11.State"), value.testBit8(2));
   pushValues->set(_T("NTCB.DigitalInput12.State"), value.testBit8(3));
   pushValues->set(_T("NTCB.DigitalInput13.State"), value.testBit8(4));
   pushValues->set(_T("NTCB.DigitalInput14.State"), value.testBit8(5));
   pushValues->set(_T("NTCB.DigitalInput15.State"), value.testBit8(6));
   pushValues->set(_T("NTCB.DigitalInput16.State"), value.testBit8(7));
}

/**
 * Parser for field 31
 */
static void FP_31(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.DigitalOutput1.State"), value.testBit8(0));
   pushValues->set(_T("NTCB.DigitalOutput2.State"), value.testBit8(1));
   pushValues->set(_T("NTCB.DigitalOutput3.State"), value.testBit8(2));
   pushValues->set(_T("NTCB.DigitalOutput4.State"), value.testBit8(3));
   pushValues->set(_T("NTCB.DigitalOutput5.State"), value.testBit8(4));
   pushValues->set(_T("NTCB.DigitalOutput6.State"), value.testBit8(5));
   pushValues->set(_T("NTCB.DigitalOutput7.State"), value.testBit8(6));
   pushValues->set(_T("NTCB.DigitalOutput8.State"), value.testBit8(7));
}

/**
 * Parser for field 32
 */
static void FP_32(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.DigitalOutput9.State"), value.testBit8(0));
   pushValues->set(_T("NTCB.DigitalOutput10.State"), value.testBit8(1));
   pushValues->set(_T("NTCB.DigitalOutput11.State"), value.testBit8(2));
   pushValues->set(_T("NTCB.DigitalOutput12.State"), value.testBit8(3));
   pushValues->set(_T("NTCB.DigitalOutput13.State"), value.testBit8(4));
   pushValues->set(_T("NTCB.DigitalOutput14.State"), value.testBit8(5));
   pushValues->set(_T("NTCB.DigitalOutput15.State"), value.testBit8(6));
   pushValues->set(_T("NTCB.DigitalOutput16.State"), value.testBit8(7));
}

/**
 * Parser for fields 38-44
 */
static void FP_38_44(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   if (value.u16 >= 65500)
      return;  // Sensor error
   TCHAR buffer[32], name[64];
   _sntprintf(buffer, 32, _T("%d.%d"), static_cast<int32_t>(value.u16) / 10, static_cast<int32_t>(value.u16) % 10);
   _sntprintf(name, 64, _T("NTCB.FuelSensor%s.Amount"), static_cast<const TCHAR*>(options));
   pushValues->set(name, buffer);
}

/**
 * Parser for field 53
 * Если разряд 15 равен единице 0-100 в % (точность до 1%)
 * Если разряд 15 равен нулю 0-32766 в 0,1 литра (0-3276,6 литра)
 *    32767 (0x7FFF) – параметр не считывается
 * Actual data from emulator is reversed - current code uses what is received from emulator
 */
static void FP_53(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   if ((value.u16 & 0x7FFF) == 0x7FFF)
      return;
   if (value.u16 & 0x8000)
   {
      TCHAR buffer[32];
      _sntprintf(buffer, 32, _T("%d.%d"), static_cast<int32_t>(value.u16 & 0x7FFF) / 10, static_cast<int32_t>(value.u16 & 0x7FFF) % 10);
      pushValues->set(_T("NTCB.CAN.FuelAmount"), buffer);
   }
   else
   {
      pushValues->set(_T("NTCB.CAN.FuelLevel"), value.u16);
   }
}

/**
 * Parser for field 66
 * Если разряд 15 равен единице 0-100 в % (точность до 1%)
 * Если разряд 15 равен нулю 0-32766 в 0,1 литра (0-3276,6 литра)
 *    32767 (0x7FFF) – параметр не считывается
 * Actual data from emulator is reversed - current code uses what is received from emulator
 */
static void FP_66(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   if ((value.u16 & 0x7FFF) == 0x7FFF)
      return;
   if (value.u16 & 0x8000)
   {
      TCHAR buffer[32];
      _sntprintf(buffer, 32, _T("%d.%d"), static_cast<int32_t>(value.u16 & 0x7FFF) / 10, static_cast<int32_t>(value.u16 & 0x7FFF) % 10);
      pushValues->set(_T("NTCB.CAN.DEFAmount"), buffer);
   }
   else
   {
      pushValues->set(_T("NTCB.CAN.DEFLevel"), value.u16);
   }
}

/**
 * Parser for field 68
 */
static void FP_68(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   if (value.i16 < 0)
      return;  // Read error
   pushValues->set(_T("NTCB.CAN.ServiceDistance"), static_cast<int32_t>(value.i16) * 5);
}

/**
 * Parser for field 70
 */
static void FP_70(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.Glonass.Satellites"), static_cast<int32_t>(value.raw[0]));
   pushValues->set(_T("NTCB.GPS.Satellites"), static_cast<int32_t>(value.raw[1]));
   pushValues->set(_T("NTCB.Galileo.Satellites"), static_cast<int32_t>(value.raw[2]));
   pushValues->set(_T("NTCB.Compass.Satellites"), static_cast<int32_t>(value.raw[3]));
   pushValues->set(_T("NTCB.Beidou.Satellites"), static_cast<int32_t>(value.raw[4]));
   pushValues->set(_T("NTCB.DORIS.Satellites"), static_cast<int32_t>(value.raw[5]));
   pushValues->set(_T("NTCB.IRNSS.Satellites"), static_cast<int32_t>(value.raw[6]));
   pushValues->set(_T("NTCB.QZSS.Satellites"), static_cast<int32_t>(value.raw[7]));
}

/**
 * LBS information for field 77 parsing
 */
#pragma pack(1)
struct LBS_INFO
{
   uint32_t cellId;
   uint16_t lac;
   uint16_t mcc;
   uint16_t mnc;
   int8_t signalLevel;
};
#pragma pack()

/**
 * Parser for field 77
 */
static void FP_77(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   auto lbs = reinterpret_cast<const LBS_INFO*>(value.raw);
   pushValues->set(_T("NTCB.LBS.CellID"), lbs->cellId);
   pushValues->set(_T("NTCB.LBS.LocalZoneID"), lbs->lac);
   pushValues->set(_T("NTCB.LBS.CountryCode"), lbs->mcc);
   pushValues->set(_T("NTCB.LBS.NetworkCode"), lbs->mnc);
   pushValues->set(_T("NTCB.LBS.SignalLevel"), lbs->signalLevel);
}

/**
 * Parser for fields 84-93
 */
static void FP_84_93(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   if (value.u16 >= 65500)
      return;  // Sensor error

   TCHAR buffer[32], name[64];
   _sntprintf(buffer, 32, _T("%d.%d"), static_cast<int32_t>(value.u16) / 10, static_cast<int32_t>(value.u16) % 10);
   _sntprintf(name, 64, _T("NTCB.FuelSensor%s.Amount"), static_cast<const TCHAR*>(options));
   pushValues->set(name, buffer);

   _sntprintf(name, 64, _T("NTCB.FuelSensor%s.Temperature"), static_cast<const TCHAR*>(options));
   pushValues->set(name, static_cast<int32_t>(static_cast<int8_t>(value.raw[2])));
}

/**
 * Parser for fields 94-97
 * Each field contains 2 to 16 3-byte structures, each structure describing one sensor
 * Fields in each structure are: wheel number, pressure in 0.1 bar units, temperature or -128
 */
static void FP_94_97(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   int sensorCount = CAST_FROM_POINTER(options, int);
   for(int i = 0; i < sensorCount; i++)
   {
      uint32_t wheel = value.raw[i * 3];
      if (wheel == 0)
         continue;   // Sensor not installed

      TCHAR name[64], data[64];
      _sntprintf(name, 64, _T("NTCB.Wheel%u.Pressure"), wheel);
      uint32_t pressure = value.raw[i * 3 + 1];
      _sntprintf(data, 64, _T("%u.%u"), pressure / 10, pressure % 10);
      pushValues->set(name, data);

      int32_t temperature = static_cast<int8_t>(value.raw[i * 3 + 2]);
      if (temperature != -128)
      {
         _sntprintf(name, 64, _T("NTCB.Wheel%u.Temperature"), wheel);
         pushValues->set(name, temperature);
      }
   }
}

/**
 * Parser for field 123
 */
static void FP_123(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.Controller.TamperingSensor"), value.testBit8(0));
}

/**
 * Parser for field 126
 */
static void FP_126(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.DigitalInput17.State"), value.testBit8(0));
   pushValues->set(_T("NTCB.DigitalInput18.State"), value.testBit8(1));
   pushValues->set(_T("NTCB.DigitalInput19.State"), value.testBit8(2));
   pushValues->set(_T("NTCB.DigitalInput20.State"), value.testBit8(3));
   pushValues->set(_T("NTCB.DigitalInput21.State"), value.testBit8(4));
   pushValues->set(_T("NTCB.DigitalInput22.State"), value.testBit8(5));
   pushValues->set(_T("NTCB.DigitalInput23.State"), value.testBit8(6));
   pushValues->set(_T("NTCB.DigitalInput24.State"), value.testBit8(7));
}

/**
 * Parser for field 140
 */
static void FP_140(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   TCHAR data[32];
   _sntprintf(data, 32, _T("%0.02f"), static_cast<double>(value.u8) / 4);
   pushValues->set(_T("NTCB.InternalTiltSensor.Angle"), data);
}

/**
 * Parser for field 141
 */
static void FP_141(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.InternalTiltSensor.Pitch"), static_cast<int32_t>(value.i8));

   TCHAR data[32];
   _sntprintf(data, 32, _T("%0.1f"), static_cast<double>(*reinterpret_cast<const int8_t*>(&value.raw[1])) * 1.5);
   pushValues->set(_T("NTCB.InternalTiltSensor.Roll"), data);
}

/**
 * Parser for field 142
 */
static void FP_142(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.ExternalTiltSensor.X"), static_cast<int32_t>(value.raw[0]));
   pushValues->set(_T("NTCB.ExternalTiltSensor.Y"), static_cast<int32_t>(value.raw[1]));
   pushValues->set(_T("NTCB.ExternalTiltSensor.Z"), static_cast<int32_t>(value.raw[2]));
}

/**
 * Parser for fields 163-166
 * От −273,15 до +1638,35 с шагом 0,05 °C, 0x8000 – нет данных
 */
static void FP_163_166(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   if (value.u16 == 0x8000)
      return;  // No data

   TCHAR data[32];
   _sntprintf(data, 32, _T("%0.2f"), static_cast<double>(value.i16) / 20.0);
   pushValues->set(static_cast<const TCHAR*>(options), data);
}

/**
 * Parser for fields 167-170
 * От 0 до 100 с шагом 0,5 %, 0xFF – нет данных
 */
static void FP_167_170(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   if (value.u8 == 0xFF)
      return;  // No data

   TCHAR data[32];
   _sntprintf(data, 32, _T("%0.1f"), static_cast<double>(value.u8) / 2);
   pushValues->set(static_cast<const TCHAR*>(options), data);
}

/**
 * Parser for field 171
 */
static void FP_171(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.FuelConsumption.SupplyMode"), static_cast<int32_t>(value.u16 & 0x000F));
   pushValues->set(_T("NTCB.FuelConsumption.ReturnMode"), static_cast<int32_t>((value.u16 >> 4) & 0x000F));
   pushValues->set(_T("NTCB.FuelConsumption.EngineMode"), static_cast<int32_t>((value.u16 >> 8) & 0x000F));
   pushValues->set(_T("NTCB.FuelConsumption.SensorPowerStatus"), static_cast<int32_t>((value.u16 >> 12) & 0x0003));
}

/**
 * Parser for field 201
 */
static void FP_201(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.CAN.SecurityState.Ignition"), value.testBit16(0));
   pushValues->set(_T("NTCB.CAN.SecurityState.AlarmActivated"), value.testBit16(1));
   pushValues->set(_T("NTCB.CAN.SecurityState.LockedFromRemote"), value.testBit16(2));
   pushValues->set(_T("NTCB.CAN.SecurityState.KeyPresent"), value.testBit16(3));
   pushValues->set(_T("NTCB.CAN.SecurityState.Ignition2"), value.testBit16(4));
   pushValues->set(_T("NTCB.CAN.SecurityState.FrontPassengerDoorOpen"), value.testBit16(5));
   pushValues->set(_T("NTCB.CAN.SecurityState.RearPassengerDoorsOpen"), value.testBit16(6));
   pushValues->set(_T("NTCB.CAN.SecurityState.DriverDoorOpen"), value.testBit16(8));
   pushValues->set(_T("NTCB.CAN.SecurityState.PassengerDoorsOpen"), value.testBit16(9));
   pushValues->set(_T("NTCB.CAN.SecurityState.HoodOpen"), value.testBit16(10));
   pushValues->set(_T("NTCB.CAN.SecurityState.TrunkOpen"), value.testBit16(11));
   pushValues->set(_T("NTCB.CAN.SecurityState.ParkingBrakes"), value.testBit16(12));
   pushValues->set(_T("NTCB.CAN.SecurityState.Brakes"), value.testBit16(13));
   pushValues->set(_T("NTCB.CAN.SecurityState.EngineRunning"), value.testBit16(14));
   pushValues->set(_T("NTCB.CAN.SecurityState.HeaterRunning"), value.testBit16(15));
}

/**
 * Parser for field 203
 */
static void FP_203(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.CAN.SafetyState.STOP"), value.testBit32(0));
   pushValues->set(_T("NTCB.CAN.SafetyState.OilLevel"), value.testBit32(1));
   pushValues->set(_T("NTCB.CAN.SafetyState.CoolantLevel"), value.testBit32(2));
   pushValues->set(_T("NTCB.CAN.SafetyState.ParkingBrakes"), value.testBit32(3));
   pushValues->set(_T("NTCB.CAN.SafetyState.BatteryCharge"), value.testBit32(4));
   pushValues->set(_T("NTCB.CAN.SafetyState.Airbags"), value.testBit32(5));
   pushValues->set(_T("NTCB.CAN.SafetyState.CheckEngine"), value.testBit32(8));
   pushValues->set(_T("NTCB.CAN.SafetyState.Lights"), value.testBit32(9));
   pushValues->set(_T("NTCB.CAN.SafetyState.TirePressure"), value.testBit32(10));
   pushValues->set(_T("NTCB.CAN.SafetyState.BrakePads"), value.testBit32(11));
   pushValues->set(_T("NTCB.CAN.SafetyState.Warning"), value.testBit32(12));
   pushValues->set(_T("NTCB.CAN.SafetyState.ABS"), value.testBit32(13));
   pushValues->set(_T("NTCB.CAN.SafetyState.FuelLevel"), value.testBit32(14));
   pushValues->set(_T("NTCB.CAN.SafetyState.ServiceIndicator"), value.testBit32(15));
   pushValues->set(_T("NTCB.CAN.SafetyState.ESP"), value.testBit32(16));
   pushValues->set(_T("NTCB.CAN.SafetyState.SparkPlugs"), value.testBit32(17));
   pushValues->set(_T("NTCB.CAN.SafetyState.FAP"), value.testBit32(18));
   pushValues->set(_T("NTCB.CAN.SafetyState.EPC"), value.testBit32(19));
   pushValues->set(_T("NTCB.CAN.SafetyState.PositionLamps"), value.testBit32(20));
   pushValues->set(_T("NTCB.CAN.SafetyState.LowBeam"), value.testBit32(21));
   pushValues->set(_T("NTCB.CAN.SafetyState.HighBeam"), value.testBit32(22));
   pushValues->set(_T("NTCB.CAN.SafetyState.ReadyToDrive"), value.testBit32(24));
   pushValues->set(_T("NTCB.CAN.SafetyState.CruiseControl"), value.testBit32(25));
   pushValues->set(_T("NTCB.CAN.SafetyState.AutomaticRetarder"), value.testBit32(26));
   pushValues->set(_T("NTCB.CAN.SafetyState.ManualRetarder"), value.testBit32(27));
   pushValues->set(_T("NTCB.CAN.SafetyState.AirConditioner"), value.testBit32(28));
   pushValues->set(_T("NTCB.CAN.SafetyState.DriverSeatBelt"), value.testBit32(30));
   pushValues->set(_T("NTCB.CAN.SafetyState.PassengerSeatBelt"), value.testBit32(31));
}

/**
 * Parser for field 204
 */
static void FP_204(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.CAN.Diagnostics.SteadyFaultIndicator"), value.testBit8(0));
   pushValues->set(_T("NTCB.CAN.Diagnostics.SteadyStopIndicator"), value.testBit8(1));
   pushValues->set(_T("NTCB.CAN.Diagnostics.SteadyWarningIndicator"), value.testBit8(2));
   pushValues->set(_T("NTCB.CAN.Diagnostics.SteadyProtectionIndicator"), value.testBit8(3));
   pushValues->set(_T("NTCB.CAN.Diagnostics.BlinkingFaultIndicator"), value.testBit8(4));
   pushValues->set(_T("NTCB.CAN.Diagnostics.BlinkingStopIndicator"), value.testBit8(5));
   pushValues->set(_T("NTCB.CAN.Diagnostics.BlinkingWarningIndicator"), value.testBit8(6));
   pushValues->set(_T("NTCB.CAN.Diagnostics.BlinkingProtectionIndicator"), value.testBit8(7));

   uint32_t diagCode;
   memcpy(&diagCode, &value.raw[1], 4);
   pushValues->set(_T("NTCB.CAN.Diagnostics.TroubleCode"), diagCode);
}

/**
 * Parser for fields that contains amount in centi- units
 */
static void FP_centi(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   if (value.u32 == 0xFFFFFFFF)
      return;  // No data

   TCHAR data[32];
   _sntprintf(data, 32, _T("%u.%02u"), value.u32 / 100, value.u32 % 100);
   pushValues->set(static_cast<const TCHAR*>(options), data);
}

/**
 * Parser for fields that contains amount in deci- units
 */
static void FP_deci(TelemetryDataType dataType, const TelemetryValue& value, const void *options, StringMap *pushValues)
{
   if (value.u16 & 0x8000)
      return;  // No data

   TCHAR data[32];
   _sntprintf(data, 32, _T("%d.%d"), static_cast<int32_t>(value.i16) / 10, static_cast<int32_t>(value.i16) % 10);
   pushValues->set(static_cast<const TCHAR*>(options), data);
}

/**
 * FLEX telemetry fields
 */
static struct TelemetryField s_telemetryFields[255] =
{
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 1: Сквозной номер записи в энергонезависимой памяти
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 2: Код события, соответствующий данной записи
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 3: Время события
   { 1, TelemetryDataType::U8, nullptr, FP_4, nullptr },    // 4: Статус устройства
   { 1, TelemetryDataType::U8, nullptr, FP_5, nullptr },    // 5: Статус функциональных модулей 1
   { 1, TelemetryDataType::U8, nullptr, FP_6, nullptr },    // 6: Статус функциональных модулей 2
   { 1, TelemetryDataType::U8, nullptr, FP_7, nullptr },    // 7: Уровень GSM
   { 1, TelemetryDataType::U8, nullptr, FP_8, nullptr },    // 8: Состояние навигационного датчика GPS/ГЛОНАСС
   { 4, TelemetryDataType::U32, _T("NTCB.GPS.LastValidLocationTimestamp"), nullptr, nullptr },   // 9: Время последних валидных координат (до произошедшего события)
   { 4, TelemetryDataType::I32, nullptr, nullptr, nullptr },   // 10: Последняя валидная широта
   { 4, TelemetryDataType::I32, nullptr, nullptr, nullptr },   // 11: Последняя валидная долгота
   { 4, TelemetryDataType::I32, nullptr, nullptr, nullptr },   // 12: Последняя валидная высота
   { 4, TelemetryDataType::F32, _T("NTCB.Speed"), nullptr, nullptr },   // 13: Скорость
   { 2, TelemetryDataType::U16, _T("NTCB.Direction"), nullptr, nullptr },   // 14: Курс
   { 4, TelemetryDataType::F32, _T("NTCB.CurrentMileage"), nullptr, nullptr },   // 15: Текущий пробег
   { 4, TelemetryDataType::F32, nullptr, nullptr, nullptr },   // 16: Последний отрезок пути
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 17: Общее количество секунд на последнем отрезке пути
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 18: Количество секунд на последнем отрезке пути, по которым вычислялся пробег
   { 2, TelemetryDataType::U16, _T("NTCB.PrimaryPowerSupply.Voltage"), nullptr, nullptr },   // 19: Напряжение на основном источнике питания
   { 2, TelemetryDataType::U16, _T("NTCB.SecondaryPowerSupply.Voltage"), nullptr, nullptr },   // 20: Напряжение на резервном источнике питания
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput1.Voltage"), nullptr, nullptr },   // 21: Напряжение на аналоговом входе 1 (Ain1)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput2.Voltage"), nullptr, nullptr },   // 22: Напряжение на аналоговом входе 2 (Ain2)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput3.Voltage"), nullptr, nullptr },   // 23: Напряжение на аналоговом входе 3 (Ain3)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput4.Voltage"), nullptr, nullptr },   // 24: Напряжение на аналоговом входе 4 (Ain4)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput5.Voltage"), nullptr, nullptr },   // 25: Напряжение на аналоговом входе 5 (Ain5)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput6.Voltage"), nullptr, nullptr },   // 26: Напряжение на аналоговом входе 6 (Ain6)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput7.Voltage"), nullptr, nullptr },   // 27: Напряжение на аналоговом входе 7 (Ain7)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput8.Voltage"), nullptr, nullptr },   // 28: Напряжение на аналоговом входе 8 (Ain8)
   { 1, TelemetryDataType::U8, nullptr, FP_29, nullptr },    // 29: Текущие показания дискретных датчиков 1
   { 1, TelemetryDataType::U8, nullptr, FP_30, nullptr },    // 30: Текущие показания дискретных датчиков 2
   { 1, TelemetryDataType::U8, nullptr, FP_31, nullptr },    // 31: Текущее состояние выходов 1
   { 1, TelemetryDataType::U8, nullptr, FP_32, nullptr },    // 32: Текущее состояние выходов 2
   { 4, TelemetryDataType::U32, _T("NTCB.ImpulseCount1"), nullptr, nullptr },   // 33: Показания счетчика импульсов 1
   { 4, TelemetryDataType::U32, _T("NTCB.ImpulseCount2"), nullptr, nullptr },   // 34: Показания счетчика импульсов 2
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogFuelLevelSensor1.Frequency"), nullptr, nullptr },   // 35: Частота на аналогово-частотном датчике уровня топлива 1
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogFuelLevelSensor2.Frequency"), nullptr, nullptr },   // 36: Частота на аналогово-частотном датчике уровня топлива 2
   { 4, TelemetryDataType::U32, _T("NTCB.EngineHours"), nullptr, nullptr },   // 37: Моточасы, подсчитанные во время срабатывания датчика работы генератора
   { 2, TelemetryDataType::U16, nullptr, FP_38_44, _T("1") },   // 38: Уровень топлива, измеренный датчиком уровня топлива 1 RS-485
   { 2, TelemetryDataType::U16, nullptr, FP_38_44, _T("2") },   // 39: Уровень топлива, измеренный датчиком уровня топлива 2 RS-485
   { 2, TelemetryDataType::U16, nullptr, FP_38_44, _T("3") },   // 40: Уровень топлива, измеренный датчиком уровня топлива 3 RS-485
   { 2, TelemetryDataType::U16, nullptr, FP_38_44, _T("4") },   // 41: Уровень топлива, измеренный датчиком уровня топлива 4 RS-485
   { 2, TelemetryDataType::U16, nullptr, FP_38_44, _T("5") },   // 42: Уровень топлива, измеренный датчиком уровня топлива 5 RS-485
   { 2, TelemetryDataType::U16, nullptr, FP_38_44, _T("6") },   // 43: Уровень топлива, измеренный датчиком уровня топлива 6 RS-485
   { 2, TelemetryDataType::U16, nullptr, FP_38_44, _T("RS232") },   // 44: Уровень топлива, измеренный датчиком уровня топлива RS-232
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature1"), nullptr, nullptr },    // 45: Температура с цифрового датчика 1 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature2"), nullptr, nullptr },    // 46: Температура с цифрового датчика 2 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature3"), nullptr, nullptr },    // 47: Температура с цифрового датчика 3 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature4"), nullptr, nullptr },    // 48: Температура с цифрового датчика 4 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature5"), nullptr, nullptr },    // 49: Температура с цифрового датчика 5 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature6"), nullptr, nullptr },    // 50: Температура с цифрового датчика 6 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature7"), nullptr, nullptr },    // 51: Температура с цифрового датчика 7 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature8"), nullptr, nullptr },    // 52: Температура с цифрового датчика 8 (в градусах Цельсия)
   { 2, TelemetryDataType::U16, nullptr, FP_53, nullptr },   // 53: CAN Уровень топлива в баке
   { 4, TelemetryDataType::F32, _T("NTCB.CAN.FuelConsumption"), nullptr, nullptr },   // 54: CAN Полный расход топлива
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.EngineRPM"), nullptr, nullptr },   // 55: CAN Обороты двигателя
   { 1, TelemetryDataType::I8, _T("NTCB.CAN.CoolantTemperature"), nullptr, nullptr },    // 56: CAN Температура охлаждающей жидкости (двигателя)
   { 4, TelemetryDataType::F32, _T("NTCB.CAN.Mileage"), nullptr, nullptr },   // 57: CAN Полный пробег ТС
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.Axis1Load"), nullptr, nullptr },   // 58: CAN Нагрузка на ось 1
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.Axis2Load"), nullptr, nullptr },   // 59: CAN Нагрузка на ось 2
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.Axis3Load"), nullptr, nullptr },   // 60: CAN Нагрузка на ось 3
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.Axis4Load"), nullptr, nullptr },   // 61: CAN Нагрузка на ось 4
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.Axis5Load"), nullptr, nullptr },   // 62: CAN Нагрузка на ось 5
   { 1, TelemetryDataType::U8, _T("NTCB.CAN.AcceleratorPedalPosition"), nullptr, nullptr },    // 63: CAN Положение педали газа
   { 1, TelemetryDataType::U8, _T("NTCB.CAN.BrakePedalPosition"), nullptr, nullptr },    // 64: CAN Положение педали тормоза
   { 1, TelemetryDataType::U8, _T("NTCB.CAN.EngineLoad"), nullptr, nullptr },    // 65: CAN Нагрузка на двигатель
   { 2, TelemetryDataType::U16, nullptr, FP_66, nullptr },   // 66: CAN Уровень жидкости в дизельном фильтре выхлопных газов
   { 4, TelemetryDataType::U32, _T("NTCB.CAN.EngineRunTime"), nullptr, nullptr },   // 67: CAN Полное время работы двигателя
   { 2, TelemetryDataType::I16, nullptr, FP_68, nullptr },   // 68: CAN Расстояние до ТО
   { 1, TelemetryDataType::U8, _T("NTCB.CAN.Speed"), nullptr, nullptr },    // 69: CAN Скорость ТС
   { 8, TelemetryDataType::STRUCT, nullptr, FP_70, nullptr },    // 70: Информация о навигации
   { 2, TelemetryDataType::STRUCT, nullptr, nullptr, nullptr },  // 71: HDOP штатного приёмника PDOP штатного приёмника
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 72: Состояние дополнительного высокоточного навигационного приёмника
   { 16, TelemetryDataType::STRUCT, nullptr, nullptr, nullptr },  // 73: Широта координаты от высокоточного приёмника Долгота координаты от высокоточного приёмника
   { 4, TelemetryDataType::I32, nullptr, nullptr, nullptr },   // 74: Высота от высокоточного приёмника
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },      // 75: Курс от высокоточного приёмника
   { 4, TelemetryDataType::F32, nullptr, nullptr, nullptr },      // 76: Скорость от высокоточного приёмника
   { 37, TelemetryDataType::STRUCT, nullptr, FP_77, nullptr },  // 77: Информация о текущей и соседних базовых станциях (LBS)
   { 1, TelemetryDataType::I8, _T("NTCB.FuelSensor1.Temperature"), nullptr, nullptr },       // 78: Температура, измеренная датчиком уровня топлива 1 RS485
   { 1, TelemetryDataType::I8, _T("NTCB.FuelSensor2.Temperature"), nullptr, nullptr },       // 79: Температура, измеренная датчиком уровня топлива 2 RS485
   { 1, TelemetryDataType::I8, _T("NTCB.FuelSensor3.Temperature"), nullptr, nullptr },       // 80: Температура, измеренная датчиком уровня топлива 3 RS485
   { 1, TelemetryDataType::I8, _T("NTCB.FuelSensor4.Temperature"), nullptr, nullptr },       // 81: Температура, измеренная датчиком уровня топлива 4 RS485
   { 1, TelemetryDataType::I8, _T("NTCB.FuelSensor5.Temperature"), nullptr, nullptr },       // 82: Температура, измеренная датчиком уровня топлива 5 RS485
   { 1, TelemetryDataType::I8, _T("NTCB.FuelSensor6.Temperature"), nullptr, nullptr },       // 83: Температура, измеренная датчиком уровня топлива 6 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, FP_84_93, _T("7") },   // 84: Уровень топлива измеренный датчиком уровня топлива 7 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, FP_84_93, _T("8") },   // 85: Уровень топлива измеренный датчиком уровня топлива 8 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, FP_84_93, _T("9") },   // 86: Уровень топлива измеренный датчиком уровня топлива 9 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, FP_84_93, _T("10") },   // 87: Уровень топлива измеренный датчиком уровня топлива 10 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, FP_84_93, _T("11") },   // 88: Уровень топлива измеренный датчиком уровня топлива 11 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, FP_84_93, _T("12") },   // 89: Уровень топлива измеренный датчиком уровня топлива 12 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, FP_84_93, _T("13") },   // 90: Уровень топлива измеренный датчиком уровня топлива 13 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, FP_84_93, _T("14") },   // 91: Уровень топлива измеренный датчиком уровня топлива 14 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, FP_84_93, _T("15") },   // 92: Уровень топлива измеренный датчиком уровня топлива 15 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, FP_84_93, _T("16") },   // 93: Уровень топлива измеренный датчиком уровня топлива 16 RS485
   { 6, TelemetryDataType::STRUCT, nullptr, FP_94_97, CAST_TO_POINTER(2, const void*) },   // 94: Информация о 1 и 2 датчике давления в шинах
   { 12, TelemetryDataType::STRUCT, nullptr, FP_94_97, CAST_TO_POINTER(4, const void*) },  // 95: Информация о 3-6 датчике давления в шинах
   { 24, TelemetryDataType::STRUCT, nullptr, FP_94_97, CAST_TO_POINTER(8, const void*) },  // 96: Информация о 7-14 датчике давления в шинах
   { 48, TelemetryDataType::STRUCT, nullptr, FP_94_97, CAST_TO_POINTER(16, const void*) }, // 97: Информация о 15-30 датчике давления в шинах
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },       // 98: Данные от тахографа: Активность водителей и состояние слотов карт.
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },       // 99: Режим работы тахографа/ карта
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },      // 100: Флаги состояния от тахографа
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },       // 101: Скорость по тахографу
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },      // 102: Одометр по тахографу
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },      // 103: Время по тахографу
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },       // 104: Текущее состояние водителя принятое от дисплейного модуля
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },      // 105: Индекс последнего полученного/прочитанного сообщения на дисплейном модуле.
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },      // 106: Приращение к времени относительно предыдущей записи
   { 6, TelemetryDataType::I16, nullptr, nullptr, nullptr },      // 107: Линейное ускорение по оси X Y Z
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },     // 108: EcoDriving. Длительность превышения порога
   { 6, TelemetryDataType::I16, nullptr, nullptr, nullptr },     // 109: EcoDriving. Максимальное значение положительного, отрицательного и бокового ускорения за период
   { 2, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 110: Данные счетчиков пассажиропотока  1 и 2
   { 2, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 111: Данные счетчиков пассажиропотока  3 и 4
   { 2, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 112: Данные счетчиков пассажиропотока  5 и 6
   { 2, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 113: Данные счетчиков пассажиропотока  7 и 8
   { 2, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 114: Данные счетчиков пассажиропотока  9 и 10
   { 2, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 115: Данные счетчиков пассажиропотока  11 и 12
   { 2, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 116: Данные счетчиков пассажиропотока  13 и 14
   { 2, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 117: Данные счетчиков пассажиропотока  15 и 16
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 118: Статус автоинформатора
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },     // 119: ID последней геозоны
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },     // 120: ID последней остановки
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },     // 121: ID текущего маршрута
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 122: Статус камеры
   { 1, TelemetryDataType::U8, nullptr, FP_123, nullptr },      // 123: Статус устройства 2
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 124: Статус функциональных модулей 3
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 125: Статус состояния связи
   { 1, TelemetryDataType::U8, nullptr, FP_126, nullptr },      // 126: Текущие показания дискретных датчиков 3
   { 4, TelemetryDataType::U32, _T("NTCB.ImpulseCount3"), nullptr, nullptr },     // 127: Показания счетчика импульсов 3
   { 4, TelemetryDataType::U32, _T("NTCB.ImpulseCount4"), nullptr, nullptr },     // 128: Показания счетчика импульсов 4
   { 4, TelemetryDataType::U32, _T("NTCB.ImpulseCount5"), nullptr, nullptr },     // 129: Показания счетчика импульсов 5
   { 4, TelemetryDataType::U32, _T("NTCB.ImpulseCount6"), nullptr, nullptr },     // 130: Показания счетчика импульсов 6
   { 4, TelemetryDataType::U32, _T("NTCB.ImpulseCount7"), nullptr, nullptr },     // 131: Показания счетчика импульсов 7
   { 4, TelemetryDataType::U32, _T("NTCB.ImpulseCount8"), nullptr, nullptr },     // 132: Показания счетчика импульсов 8
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogFuelLevelSensor3.Frequency"), nullptr, nullptr },     // 133: Частота на аналогово-частотном датчике 3
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogFuelLevelSensor4.Frequency"), nullptr, nullptr },     // 134: Частота на аналогово-частотном датчике 4
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogFuelLevelSensor5.Frequency"), nullptr, nullptr },     // 135: Частота на аналогово-частотном датчике 5
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogFuelLevelSensor6.Frequency"), nullptr, nullptr },     // 136: Частота на аналогово-частотном датчике 6
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogFuelLevelSensor7.Frequency"), nullptr, nullptr },     // 137: Частота на аналогово-частотном датчике 7
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogFuelLevelSensor8.Frequency"), nullptr, nullptr },     // 138: Частота на аналогово-частотном датчике 8
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 139: Состояние виртуальных датчиков акселерометра
   { 1, TelemetryDataType::U8, nullptr, FP_140, nullptr },      // 140: Внутренний датчик угла наклона.Угол наклона относительно местной вертикали
   { 2, TelemetryDataType::I8, nullptr, FP_141, nullptr },      // 141: Внутренний датчик наклона. Углы наклона относительно отвесной линии
   { 3, TelemetryDataType::U8, nullptr, FP_142, nullptr },      // 142: Внешний датчик угла наклона. Отклонения по осям
   { 2, TelemetryDataType::I16, nullptr, nullptr, nullptr },     // 143: EcoDriving. Максимальное значение вертикального ускорения за период
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 144: EcoDriving. Максимальное значение скорости за период
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 145: EcoDriving. Состояние порогов скорости
   { 3, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 146: EcoDriving. Состояние порогов ускорения
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor1.Frequency"), nullptr, nullptr },  // 147: Частота на выходе ДУТ 485 No1
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor2.Frequency"), nullptr, nullptr },  // 148: Частота на выходе ДУТ 485 No2
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor3.Frequency"), nullptr, nullptr },  // 149: Частота на выходе ДУТ 485 No3
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor4.Frequency"), nullptr, nullptr },  // 150: Частота на выходе ДУТ 485 No4
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor5.Frequency"), nullptr, nullptr },  // 151: Частота на выходе ДУТ 485 No5
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor6.Frequency"), nullptr, nullptr },  // 152: Частота на выходе ДУТ 485 No6
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor7.Frequency"), nullptr, nullptr },  // 153: Частота на выходе ДУТ 485 No7
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor8.Frequency"), nullptr, nullptr },  // 154: Частота на выходе ДУТ 485 No8
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor9.Frequency"), nullptr, nullptr },  // 155: Частота на выходе ДУТ 485 No9
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor10.Frequency"), nullptr, nullptr }, // 156: Частота на выходе ДУТ 485 No10
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor11.Frequency"), nullptr, nullptr }, // 157: Частота на выходе ДУТ 485 No11
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor12.Frequency"), nullptr, nullptr }, // 158: Частота на выходе ДУТ 485 No12
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor13.Frequency"), nullptr, nullptr }, // 159: Частота на выходе ДУТ 485 No13
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor14.Frequency"), nullptr, nullptr }, // 160: Частота на выходе ДУТ 485 No14
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor15.Frequency"), nullptr, nullptr }, // 161: Частота на выходе ДУТ 485 No15
   { 2, TelemetryDataType::U16, _T("NTCB.FuelLevelSensor16.Frequency"), nullptr, nullptr }, // 162: Частота на выходе ДУТ 485 No16
   { 2, TelemetryDataType::I16, nullptr, FP_163_166, _T("NTCB.HighPrecisionTemperature1") }, // 163: Высокоточный датчик температуры 1
   { 2, TelemetryDataType::I16, nullptr, FP_163_166, _T("NTCB.HighPrecisionTemperature2") }, // 164: Высокоточный датчик температуры 2
   { 2, TelemetryDataType::I16, nullptr, FP_163_166, _T("NTCB.HighPrecisionTemperature3") }, // 165: Высокоточный датчик температуры 3
   { 2, TelemetryDataType::I16, nullptr, FP_163_166, _T("NTCB.HighPrecisionTemperature4") }, // 166: Высокоточный датчик температуры 4
   { 1, TelemetryDataType::U8, nullptr, FP_167_170, _T("NTCB.HighPrecisionHumidity1") },      // 167: Высокоточный датчик влажности 1
   { 1, TelemetryDataType::U8, nullptr, FP_167_170, _T("NTCB.HighPrecisionHumidity2") },      // 168: Высокоточный датчик влажности 2
   { 1, TelemetryDataType::U8, nullptr, FP_167_170, _T("NTCB.HighPrecisionHumidity3") },      // 169: Высокоточный датчик влажности 3
   { 1, TelemetryDataType::U8, nullptr, FP_167_170, _T("NTCB.HighPrecisionHumidity4") },      // 170: Высокоточный датчик влажности 4
   { 2, TelemetryDataType::U16, nullptr, FP_171, nullptr },     // 171: Датчик расхода топлива. Статус датчика
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },     // 172: Датчик расхода топлива. Информация о неисправностях
   { 4, TelemetryDataType::U32, nullptr, FP_centi, _T("NTCB.FuelConsumption.Total") },     // 173: Датчик расхода топлива. Суммарный расход топлива
   { 4, TelemetryDataType::U32, nullptr, FP_centi, _T("NTCB.FuelConsumption.Trip") },     // 174: Датчик расхода топлива. Расход топлива за поездку
   { 2, TelemetryDataType::I16, nullptr, FP_deci, _T("NTCB.FuelConsumption.CurrentFlow") },     // 175: Датчик расхода топлива. Текущая скорость потока
   { 4, TelemetryDataType::U32, nullptr, FP_centi, _T("NTCB.FuelConsumption.SupplyAmount") },     // 176: Датчик расхода топлива. Суммарный объем топлива камеры подачи
   { 2, TelemetryDataType::I16, nullptr, FP_deci, _T("NTCB.FuelConsumption.SupplyFlow") },     // 177: Датчик расхода топлива. Текущая скорость потока камеры подачи
   { 2, TelemetryDataType::I16, nullptr, FP_deci, _T("NTCB.FuelConsumption.SupplyTemperature") },     // 178: Датчик расхода топлива. Температура камеры подачи
   { 4, TelemetryDataType::U32, nullptr, FP_centi, _T("NTCB.FuelConsumption.ReturnAmount") },     // 179: Датчик расхода топлива. Суммарный объем топлива камеры обратки
   { 2, TelemetryDataType::I16, nullptr, FP_deci, _T("NTCB.FuelConsumption.ReturnFlow") },     // 180: Датчик расхода топлива. Текущая скорость потока камеры обратки
   { 2, TelemetryDataType::I16, nullptr, FP_deci, _T("NTCB.FuelConsumption.ReturnTemperature") },     // 181: Датчик расхода топлива. Температура камеры обратки
   { 2, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 182: Рефрижераторная установка. Состояние установки
   { 2, TelemetryDataType::I16, nullptr, nullptr, nullptr },     // 183: Рефрижераторная установка. Температура рефрижератора в секции 1 (Температура ХОУ)
   { 2, TelemetryDataType::I16, nullptr, nullptr, nullptr },     // 184: Рефрижераторная установка. Температура рефрижератора в секции 2
   { 2, TelemetryDataType::I16, nullptr, nullptr, nullptr },     // 185: Рефрижераторная установка. Температура рефрижератора в секции 3
   { 2, TelemetryDataType::I16, nullptr, nullptr, nullptr },     // 186: Рефрижераторная установка. Температура установленная 1
   { 2, TelemetryDataType::I16, nullptr, nullptr, nullptr },     // 187: Рефрижераторная установка. Температура установленная 2
   { 2, TelemetryDataType::I16, nullptr, nullptr, nullptr },     // 188: Рефрижераторная установка. Температура установленная 3
   { 2, TelemetryDataType::I16, nullptr, nullptr, nullptr },     // 189: Рефрижераторная установка. Температура окружающего воздуха
   { 2, TelemetryDataType::I16, nullptr, nullptr, nullptr },     // 190: Рефрижераторная установка. Температура ОЖ
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },     // 191: Рефрижераторная установка. Напряжение аккумулятора
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },     // 192: Рефрижераторная установка. Сила тока аккумулятора
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },     // 193: Рефрижераторная установка. Моточасы работы от двигателя
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },     // 194: Рефрижераторная установка. Моточасы работы от сети
   { 4, TelemetryDataType::U16, nullptr, nullptr, nullptr },     // 195: Рефрижераторная установка. Количество ошибок. Код самой важной ошибки
   { 4, TelemetryDataType::U16, nullptr, nullptr, nullptr },     // 196: Рефрижераторная установка. Код 2й и 3й по важности ошибки
   { 6, TelemetryDataType::U16, nullptr, nullptr, nullptr },     // 197: Рефрижераторная установка. Код 4й-6й по важности ошибки
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr, nullptr },  // 198: Рефрижераторная установка. Состояние двигателя
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },      // 199: Рефрижераторная установка. Конфигурация компрессора
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },     // 200: Информация о нахождении в геозонах
   { 2, TelemetryDataType::U16, nullptr, FP_201, nullptr },     // 201: CAN. Флаги состояния безопасности
   { 1, TelemetryDataType::U8, _T("NTCB.CAN.LastSecurityEvent"), nullptr, nullptr },      // 202: CAN. События состояния безопасности
   { 4, TelemetryDataType::U32, nullptr, FP_203, nullptr },     // 203: CAN. Контроллеры аварии
   { 5, TelemetryDataType::STRUCT, nullptr, FP_204, nullptr },  // 204: CAN. Информация о неисправностях
   { 4, TelemetryDataType::U32, _T("NTCB.UserEngineHours.UnderLoad"), nullptr, nullptr },     // 205: Пользовательские моточасы 1 (работа под нагрузкой)
   { 4, TelemetryDataType::U32, _T("NTCB.UserEngineHours.WithoutLoad"), nullptr, nullptr },     // 206: Пользовательские моточасы 2 (работа без нагрузки)
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 207: Пользовательский параметр 1 байт No1
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 208: Пользовательский параметр 1 байт No2
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 209: Пользовательский параметр 1 байт No3
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 210: Пользовательский параметр 1 байт No4
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 211: Пользовательский параметр 1 байт No5
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 212: Пользовательский параметр 1 байт No6
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 213: Пользовательский параметр 1 байт No7
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 214: Пользовательский параметр 1 байт No8
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 215: Пользовательский параметр 1 байт No9
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 216: Пользовательский параметр 1 байт No10
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 217: Пользовательский параметр 1 байт No11
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 218: Пользовательский параметр 1 байт No12
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 219: Пользовательский параметр 1 байт No13
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 220: Пользовательский параметр 1 байт No14
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 221: Пользовательский параметр 1 байт No15
   { 1, TelemetryDataType::U8, nullptr, nullptr, nullptr },    // 222: Пользовательский параметр 1 байт No16
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 223: Пользовательский параметр 2 байта No1
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 224: Пользовательский параметр 2 байта No2
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 225: Пользовательский параметр 2 байта No3
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 226: Пользовательский параметр 2 байта No4
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 227: Пользовательский параметр 2 байта No5
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 228: Пользовательский параметр 2 байта No6
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 229: Пользовательский параметр 2 байта No7
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 230: Пользовательский параметр 2 байта No8
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 231: Пользовательский параметр 2 байта No9
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 232: Пользовательский параметр 2 байта No10
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 233: Пользовательский параметр 2 байта No11
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 234: Пользовательский параметр 2 байта No12
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 235: Пользовательский параметр 2 байта No13
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 236: Пользовательский параметр 2 байта No14
   { 2, TelemetryDataType::U16, nullptr, nullptr, nullptr },   // 237: Пользовательский параметр 2 байта No15
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 238: Пользовательский параметр 4 байта No1
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 239: Пользовательский параметр 4 байта No2
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 240: Пользовательский параметр 4 байта No3
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 241: Пользовательский параметр 4 байта No4
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 242: Пользовательский параметр 4 байта No5
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 243: Пользовательский параметр 4 байта No6
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 244: Пользовательский параметр 4 байта No7
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 245: Пользовательский параметр 4 байта No8
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 246: Пользовательский параметр 4 байта No9
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 247: Пользовательский параметр 4 байта No10
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 248: Пользовательский параметр 4 байта No11
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 249: Пользовательский параметр 4 байта No12
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 250: Пользовательский параметр 4 байта No13
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 251: Пользовательский параметр 4 байта No14
   { 4, TelemetryDataType::U32, nullptr, nullptr, nullptr },   // 252: Пользовательский параметр 4 байта No15
   { 8, TelemetryDataType::U64, nullptr, nullptr, nullptr },   // 253: Пользовательский параметр 8 байт No1
   { 8, TelemetryDataType::U64, nullptr, nullptr, nullptr },   // 254: Пользовательский параметр 8 байт No2
   { 8, TelemetryDataType::U64, nullptr, nullptr }    // 255: Пользовательский параметр 8 байт No3
};

/**
 * Convert value of given field to string
 */
static TCHAR *TelemetryFieldToString(const TelemetryField *field, const TelemetryValue& value, TCHAR *buffer)
{
   switch(field->dataType)
   {
      case TelemetryDataType::I8:
         _sntprintf(buffer, 64, _T("%d"), static_cast<int>(value.i8));
         break;
      case TelemetryDataType::I16:
         _sntprintf(buffer, 64, _T("%d"), static_cast<int>(value.i16));
         break;
      case TelemetryDataType::I32:
         _sntprintf(buffer, 64, _T("%d"), value.i32);
         break;
      case TelemetryDataType::I64:
         _sntprintf(buffer, 64, INT64_FMT, value.i64);
         break;
      case TelemetryDataType::U8:
         _sntprintf(buffer, 64, _T("%u"), static_cast<unsigned int>(value.u8));
         break;
      case TelemetryDataType::U16:
         _sntprintf(buffer, 64, _T("%u"), static_cast<unsigned int>(value.u16));
         break;
      case TelemetryDataType::U32:
         _sntprintf(buffer, 64, _T("%u"), value.u32);
         break;
      case TelemetryDataType::U64:
         _sntprintf(buffer, 64, UINT64_FMT, value.u64);
         break;
      case TelemetryDataType::F32:
         _sntprintf(buffer, 64, _T("%f"), value.f32);
         break;
      case TelemetryDataType::F64:
         _sntprintf(buffer, 64, _T("%f"), value.f64);
         break;
      default:
         BinToStr(value.raw, field->size, buffer);
         break;
   }
   return buffer;
}

/**
 * Constructor for telemetry record structure
 */
TelemetryRecord::TelemetryRecord(const shared_ptr<MobileDevice>& device, bool archived) : m_device(device)
{
   m_timestamp = time(nullptr);
   m_deviceStatus.commProtocol = _T("NTCB/FLEX");
   m_archived = archived;
}

/**
 * Process received field
 */
void TelemetryRecord::processField(NTCBDeviceSession *session, int fieldIndex, const TelemetryField *field, const TelemetryValue& value)
{
   TCHAR stringValue[64];
   session->debugPrintf(7, _T("Telemetry record #%d = %s"), fieldIndex, TelemetryFieldToString(field, value, stringValue));

#if WORDS_BIGENDIAN
   // TODO: swap bytes on big endian system
#endif

   // Process fields with special meaning
   switch(fieldIndex)
   {
      case 3:
         m_timestamp = value.u32;
         m_deviceStatus.timestamp = value.u32;
         break;
      case 9:
         m_deviceStatus.geoLocation.setTimestamp(value.u32);
         break;
      case 10: // Latitude in 10,000th of minutes
         m_deviceStatus.geoLocation.setLatitude(static_cast<double>(value.i32) / 600000.0);
         m_deviceStatus.geoLocation.setType(GL_GPS);
         break;
      case 11: // Longitude in 10,000th of minutes
         m_deviceStatus.geoLocation.setLongitude(static_cast<double>(value.i32) / 600000.0);
         m_deviceStatus.geoLocation.setType(GL_GPS);
         break;
      case 12: // Altitude in decimeters
         m_deviceStatus.altitude = value.i32 / 10;
         break;
      case 13:
         m_deviceStatus.speed = value.f32;
         break;
      case 14:
         m_deviceStatus.direction = value.u16;
         break;
   }

   if (field->name != nullptr)
   {
      m_pushList.set(field->name, stringValue);
   }
   else if (field->handler != nullptr)
   {
      field->handler(field->dataType, value, field->options, &m_pushList);
   }
}

/**
 * Update device with collected data
 */
void TelemetryRecord::updateDevice()
{
   nxlog_debug_tag(DEBUG_TAG_NTCB, 5, _T("Processing %s telemetry record from device %s [%u]"), m_archived ? _T("archived") : _T("current"), m_device->getName(), m_device->getId());

   if (!m_archived && (m_timestamp <= m_device->getLastReportTime()))
   {
      nxlog_debug_tag(DEBUG_TAG_NTCB, 5, _T("Telemetry record from device %s [%u] ignored"), m_device->getName(), m_device->getId());
      return;
   }

   if ((m_deviceStatus.geoLocation.getTimestamp() > m_timestamp) || (m_deviceStatus.geoLocation.getTimestamp() == 0))
      m_deviceStatus.geoLocation.setTimestamp(m_timestamp);
   m_device->updateStatus(m_deviceStatus);

   shared_ptr<Table> tableValue; // Empty pointer for processNewDCValue()
   for (KeyValuePair<const TCHAR>* data : m_pushList)
   {
      shared_ptr<DCObject> dci = m_device->getDCObjectByName(data->key, 0);
      if ((dci == nullptr) || (dci->getDataSource() != DS_PUSH_AGENT) || (dci->getType() != DCO_TYPE_ITEM))
      {
         nxlog_debug_tag(DEBUG_TAG_NTCB, 5, _T("DCI %s on device %s [%u] %s"), data->key, m_device->getName(), m_device->getId(),
                  (dci == nullptr) ? _T("does not exist") : _T("is of incompatible type"));
         continue;
      }
      m_device->processNewDCValue(dci, Timestamp::fromTime(m_timestamp), data->value, tableValue, false);
   }
}

/**
 * Read single FLEX telemetry record
 */
bool NTCBDeviceSession::readTelemetryRecord(bool archived)
{
   shared_ptr<TelemetryRecord> record = make_shared<TelemetryRecord>(m_device, archived);

   int fieldIndex = 0;
   int maskByte = 0;
   uint8_t mask = 0x80;
   while(fieldIndex < m_flexFieldCount)
   {
      if (m_flexFieldMask[maskByte] & mask)
      {
         TelemetryField *field = &s_telemetryFields[fieldIndex];
         TelemetryValue value;
         if (!m_socket.readFully(&value, field->size, g_ntcbSocketTimeout))
         {
            debugPrintf(5, _T("readTelemetryRecord: failed to read field #%d"), fieldIndex + 1);
            return false;
         }

         record->processField(this, fieldIndex + 1, field, value);
      }

      fieldIndex++;
      mask >>= 1;
      if (mask == 0)
      {
         mask = 0x80;
         maskByte++;
      }
   }

   TCHAR key[64];
   _sntprintf(key, 64, _T("MD-Telemetry-%u"), m_device->getId());
   ThreadPoolExecuteSerialized(g_mobileThreadPool, key, record, &TelemetryRecord::updateDevice);
   return true;
}
