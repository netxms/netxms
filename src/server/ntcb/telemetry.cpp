/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
static void FP_4(TelemetryDataType dataType, const TelemetryValue& value, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.DeviceStatus.TestMode"), value.testBit8(0));
   pushValues->set(_T("NTCB.DeviceStatus.AlarmNotification"), value.testBit8(1));
   pushValues->set(_T("NTCB.DeviceStatus.Alarm"), value.testBit8(2));
   pushValues->set(_T("NTCB.DeviceStatus.Mode"), static_cast<int32_t>((value.u8 >> 3) & 0x03));
   pushValues->set(_T("NTCB.DeviceStatus.Evacuation"), value.testBit8(5));
   pushValues->set(_T("NTCB.DeviceStatus.PowerSavingMode"), value.testBit8(6));
   pushValues->set(_T("NTCB.DeviceStatus.Accelerometer.Calibration"), value.testBit8(7));
}

/**
 * Parser for field 5
 */
static void FP_5(TelemetryDataType dataType, const TelemetryValue& value, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.DeviceStatus.GSM.Status"), value.testBit8(0));
   pushValues->set(_T("NTCB.DeviceStatus.USB.Status"), value.testBit8(1));
   pushValues->set(_T("NTCB.DeviceStatus.NavReceiver"), value.testBit8(2));
   pushValues->set(_T("NTCB.DeviceStatus.ClockSync"), value.testBit8(3));
   pushValues->set(_T("NTCB.DeviceStatus.GSM.ActiveSIM"), value.testBit8(4) + 1);
   pushValues->set(_T("NTCB.DeviceStatus.GSM.Registration"), value.testBit8(5));
   pushValues->set(_T("NTCB.DeviceStatus.GSM.Roaming"), value.testBit8(6));
   pushValues->set(_T("NTCB.DeviceStatus.EngineStatus"), value.testBit8(7));
}

/**
 * Parser for field 6
 */
static void FP_6(TelemetryDataType dataType, const TelemetryValue& value, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.DeviceStatus.GSM.Interference"), static_cast<int32_t>(value.u8 & 0x03));
   pushValues->set(_T("NTCB.DeviceStatus.GNSS.Interference"), value.testBit8(2));
   pushValues->set(_T("NTCB.DeviceStatus.GNSS.ApproximationMode"), value.testBit8(3));
   pushValues->set(_T("NTCB.DeviceStatus.Accelerometer.Status"), value.testBit8(4));
   pushValues->set(_T("NTCB.DeviceStatus.Bluetooth.Status"), value.testBit8(5));
   pushValues->set(_T("NTCB.DeviceStatus.WiFi.Status"), value.testBit8(6));
}

/**
 * Parser for field 7
 * 0: -113 Дб/м или меньше
 * 1: -111 Дб/м
 * 2..30: -109..-53 Дб/м
 * 31: -51 Дб/м или больше
 * 99: нет сигнала сотовой сети.
 */
static void FP_7(TelemetryDataType dataType, const TelemetryValue& value, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.DeviceStatus.GSM.SignalLevel"), (value.u8 <= 31) ? static_cast<int32_t>(value.u8) * 2 - 113 : 0);
}

/**
 * Parser for field 8
 */
static void FP_8(TelemetryDataType dataType, const TelemetryValue& value, StringMap *pushValues)
{
   pushValues->set(_T("NTCB.DeviceStatus.GPS.Status"), value.testBit8(0));
   pushValues->set(_T("NTCB.DeviceStatus.GPS.ValidCoordinates"), value.testBit8(1));
   pushValues->set(_T("NTCB.DeviceStatus.GPS.Satellites"), static_cast<uint32_t>(value.u8 >> 2));
}

/**
 * Parser for field 53
 * Если разряд 15 равен единице 0-100 в % (точность до 1%)
 * Если разряд 15 равен нулю 0-32766 в 0,1 литра (0-3276,6 литра)
 *    32767 (0x7FFF) – параметр не считывается
 * Actual data from emulator is reversed - current code uses what is received from emulator
 */
static void FP_53(TelemetryDataType dataType, const TelemetryValue& value, StringMap *pushValues)
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
 * FLEX telemetry fields
 */
static struct TelemetryField s_telemetryFields[255] =
{
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 1: Сквозной номер записи в энергонезависимой памяти
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 2: Код события, соответствующий данной записи
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 3: Время события
   { 1, TelemetryDataType::U8, nullptr, FP_4 },    // 4: Статус устройства
   { 1, TelemetryDataType::U8, nullptr, FP_5 },    // 5: Статус функциональных модулей 1
   { 1, TelemetryDataType::U8, nullptr, FP_6 },    // 6: Статус функциональных модулей 2
   { 1, TelemetryDataType::U8, nullptr, FP_7 },    // 7: Уровень GSM
   { 1, TelemetryDataType::U8, nullptr, FP_8 },    // 8: Состояние навигационного датчика GPS/ГЛОНАСС
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 9: Время последних валидных координат (до произошедшего события)
   { 4, TelemetryDataType::I32, nullptr, nullptr },   // 10: Последняя валидная широта
   { 4, TelemetryDataType::I32, nullptr, nullptr },   // 11: Последняя валидная долгота
   { 4, TelemetryDataType::I32, nullptr, nullptr },   // 12: Последняя валидная высота
   { 4, TelemetryDataType::F32, _T("NTCB.Speed"), nullptr },   // 13: Скорость
   { 2, TelemetryDataType::U16, _T("NTCB.Direction"), nullptr },   // 14: Курс
   { 4, TelemetryDataType::F32, _T("NTCB.CurrentMileage"), nullptr },   // 15: Текущий пробег
   { 4, TelemetryDataType::F32, nullptr, nullptr },   // 16: Последний отрезок пути
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 17: Общее количество секунд на последнем отрезке пути
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 18: Количество секунд на последнем отрезке пути, по которым вычислялся пробег
   { 2, TelemetryDataType::U16, _T("NTCB.PrimaryPowerSupply.Voltage"), nullptr },   // 19: Напряжение на основном источнике питания
   { 2, TelemetryDataType::U16, _T("NTCB.SecondaryPowerSupply.Voltage"), nullptr },   // 20: Напряжение на резервном источнике питания
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput1.Voltage"), nullptr },   // 21: Напряжение на аналоговом входе 1 (Ain1)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput2.Voltage"), nullptr },   // 22: Напряжение на аналоговом входе 2 (Ain2)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput3.Voltage"), nullptr },   // 23: Напряжение на аналоговом входе 3 (Ain3)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput4.Voltage"), nullptr },   // 24: Напряжение на аналоговом входе 4 (Ain4)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput5.Voltage"), nullptr },   // 25: Напряжение на аналоговом входе 5 (Ain5)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput6.Voltage"), nullptr },   // 26: Напряжение на аналоговом входе 6 (Ain6)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput7.Voltage"), nullptr },   // 27: Напряжение на аналоговом входе 7 (Ain7)
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogInput8.Voltage"), nullptr },   // 28: Напряжение на аналоговом входе 8 (Ain8)
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 29: Текущие показания дискретных датчиков 1
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 30: Текущие показания дискретных датчиков 2
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 31: Текущее состояние выходов 1
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 32: Текущее состояние выходов 2
   { 4, TelemetryDataType::U32, _T("NTCB.ImpulseCount1"), nullptr },   // 33: Показания счетчика импульсов 1
   { 4, TelemetryDataType::U32, _T("NTCB.ImpulseCount2"), nullptr },   // 34: Показания счетчика импульсов 2
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogFuelLevelSensor1.Frequency"), nullptr },   // 35: Частота на аналогово-частотном датчике уровня топлива 1
   { 2, TelemetryDataType::U16, _T("NTCB.AnalogFuelLevelSensor2.Frequency"), nullptr },   // 36: Частота на аналогово-частотном датчике уровня топлива 2
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 37: Моточасы, подсчитанные во время срабатывания датчика работы генератора
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 38: Уровень топлива, измеренный датчиком уровня топлива 1 RS-485
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 39: Уровень топлива, измеренный датчиком уровня топлива 2 RS-485
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 40: Уровень топлива, измеренный датчиком уровня топлива 3 RS-485
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 41: Уровень топлива, измеренный датчиком уровня топлива 4 RS-485
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 42: Уровень топлива, измеренный датчиком уровня топлива 5 RS-485
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 43: Уровень топлива, измеренный датчиком уровня топлива 6 RS-485
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 44: Уровень топлива, измеренный датчиком уровня топлива RS-232
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature1"), nullptr },    // 45: Температура с цифрового датчика 1 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature2"), nullptr },    // 46: Температура с цифрового датчика 2 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature3"), nullptr },    // 47: Температура с цифрового датчика 3 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature4"), nullptr },    // 48: Температура с цифрового датчика 4 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature5"), nullptr },    // 49: Температура с цифрового датчика 5 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature6"), nullptr },    // 50: Температура с цифрового датчика 6 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature7"), nullptr },    // 51: Температура с цифрового датчика 7 (в градусах Цельсия)
   { 1, TelemetryDataType::I8, _T("NTCB.Temperature8"), nullptr },    // 52: Температура с цифрового датчика 8 (в градусах Цельсия)
   { 2, TelemetryDataType::U16, nullptr, FP_53 },   // 53: CAN Уровень топлива в баке
   { 4, TelemetryDataType::F32, _T("NTCB.CAN.FuelConsumption"), nullptr },   // 54: CAN Полный расход топлива
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.EngineRPM"), nullptr },   // 55: CAN Обороты двигателя
   { 1, TelemetryDataType::I8, _T("NTCB.CAN.CoolantTemperature"), nullptr },    // 56: CAN Температура охлаждающей жидкости (двигателя)
   { 4, TelemetryDataType::F32, _T("NTCB.CAN.Mileage"), nullptr },   // 57: CAN Полный пробег ТС
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.Axis1Load"), nullptr },   // 58: CAN Нагрузка на ось 1
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.Axis2Load"), nullptr },   // 59: CAN Нагрузка на ось 2
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.Axis3Load"), nullptr },   // 60: CAN Нагрузка на ось 3
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.Axis4Load"), nullptr },   // 61: CAN Нагрузка на ось 4
   { 2, TelemetryDataType::U16, _T("NTCB.CAN.Axis5Load"), nullptr },   // 62: CAN Нагрузка на ось 5
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 63: CAN Положение педали газа
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 64: CAN Положение педали тормоза
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 65: CAN Нагрузка на двигатель
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 66: CAN Уровень жидкости в дизельном фильтре выхлопных газов
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 67: CAN Полное время работы двигателя
   { 2, TelemetryDataType::I16, nullptr, nullptr },   // 68: CAN Расстояние до ТО
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 69: CAN Скорость ТС
   { 8, TelemetryDataType::U8, nullptr, nullptr },    // 70: Информация о навигации
   { 2, TelemetryDataType::U8, nullptr, nullptr },    // 71: HDOP штатного приёмника PDOP штатного приёмника
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 72: Состояние дополнительного высокоточного навигационного приёмника
   { 16, TelemetryDataType::I64, nullptr, nullptr },  // 73: Широта координаты от высокоточного приёмника Долгота координаты от высокоточного приёмника
   { 4, TelemetryDataType::I32, nullptr, nullptr },   // 74: Высота от высокоточного приёмника
   { 2, TelemetryDataType::U16, nullptr, nullptr },      // 75: Курс от высокоточного приёмника
   { 4, TelemetryDataType::F32, nullptr, nullptr },      // 76: Скорость от высокоточного приёмника
   { 37, TelemetryDataType::STRUCT, nullptr, nullptr },  // 77: Информация о соседней станции No1 (LBS)
   { 1, TelemetryDataType::I8, nullptr, nullptr },       // 78: Температура, измеренная датчиком уровня топлива 1 RS485
   { 1, TelemetryDataType::I8, nullptr, nullptr },       // 79: Температура, измеренная датчиком уровня топлива 2 RS485
   { 1, TelemetryDataType::I8, nullptr, nullptr },       // 80: Температура, измеренная датчиком уровня топлива 3 RS485
   { 1, TelemetryDataType::I8, nullptr, nullptr },       // 81: Температура, измеренная датчиком уровня топлива 4 RS485
   { 1, TelemetryDataType::I8, nullptr, nullptr },       // 82: Температура, измеренная датчиком уровня топлива 5 RS485
   { 1, TelemetryDataType::I8, nullptr, nullptr },       // 83: Температура, измеренная датчиком уровня топлива 6 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },   // 84: Уровень топлива измеренный датчиком уровня топлива 7 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },   // 85: Уровень топлива измеренный датчиком уровня топлива 8 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },   // 86: Уровень топлива измеренный датчиком уровня топлива 9 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },   // 87: Уровень топлива измеренный датчиком уровня топлива 10 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },   // 88: Уровень топлива измеренный датчиком уровня топлива 11 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },   // 89: Уровень топлива измеренный датчиком уровня топлива 12 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },   // 90: Уровень топлива измеренный датчиком уровня топлива 13 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },   // 91: Уровень топлива измеренный датчиком уровня топлива 14 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },   // 92: Уровень топлива измеренный датчиком уровня топлива 15 RS485
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },   // 93: Уровень топлива измеренный датчиком уровня топлива 16 RS485
   { 6, TelemetryDataType::STRUCT, nullptr, nullptr },   // 94: Информация о 1 и 2 датчике давления в шинах
   { 12, TelemetryDataType::STRUCT, nullptr, nullptr },  // 95: Информация о 3-6 датчике давления в шинах
   { 24, TelemetryDataType::STRUCT, nullptr, nullptr },  // 96: Информация о 7-14 датчике давления в шинах
   { 48, TelemetryDataType::STRUCT, nullptr, nullptr },  // 97: Информация о 15-30 датчике давления в шинах
   { 1, TelemetryDataType::U8, nullptr, nullptr },       // 98: Данные от тахографа: Активность водителей и состояние слотов карт.
   { 1, TelemetryDataType::U8, nullptr, nullptr },       // 99: Режим работы тахографа/ карта
   { 2, TelemetryDataType::U16, nullptr, nullptr },      // 100: Флаги состояния от тахографа
   { 1, TelemetryDataType::U8, nullptr, nullptr },       // 101: Скорость по тахографу
   { 4, TelemetryDataType::U32, nullptr, nullptr },      // 102: Одометр по тахографу
   { 4, TelemetryDataType::U32, nullptr, nullptr },      // 103: Время по тахографу
   { 1, TelemetryDataType::U8, nullptr, nullptr },       // 104: Текущее состояние водителя принятое от дисплейного модуля
   { 4, TelemetryDataType::U32, nullptr, nullptr },      // 105: Индекс последнего полученного/прочитанного сообщения на дисплейном модуле.
   { 2, TelemetryDataType::U16, nullptr, nullptr },      // 106: Приращение к времени относительно предыдущей записи
   { 6, TelemetryDataType::I16, nullptr, nullptr },      // 107: Линейное ускорение по оси X Y Z
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 108: EcoDriving. Длительность превышения порога
   { 6, TelemetryDataType::I16, nullptr, nullptr },     // 109: EcoDriving. Максимальное значение положительного, отрицательного и бокового ускорения за период
   { 2, TelemetryDataType::U8, nullptr, nullptr },      // 110: Данные счетчиков пассажиропотока  1 и 2
   { 2, TelemetryDataType::U8, nullptr, nullptr },      // 111: Данные счетчиков пассажиропотока  3 и 4
   { 2, TelemetryDataType::U8, nullptr, nullptr },      // 112: Данные счетчиков пассажиропотока  5 и 6
   { 2, TelemetryDataType::U8, nullptr, nullptr },      // 113: Данные счетчиков пассажиропотока  7 и 8
   { 2, TelemetryDataType::U8, nullptr, nullptr },      // 114: Данные счетчиков пассажиропотока  9 и 10
   { 2, TelemetryDataType::U8, nullptr, nullptr },      // 115: Данные счетчиков пассажиропотока  11 и 12
   { 2, TelemetryDataType::U8, nullptr, nullptr },      // 116: Данные счетчиков пассажиропотока  13 и 14
   { 2, TelemetryDataType::U8, nullptr, nullptr },      // 117: Данные счетчиков пассажиропотока  15 и 16
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 118: Статус автоинформатора
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 119: ID последней геозоны
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 120: ID последней остановки
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 121: ID текущего маршрута
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 122: Статус камеры
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 123: Статус устройства 2
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 124: Статус функциональных модулей 3
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 125: Статус состояния связи
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 126: Текущие показания дискретных датчиков 3
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 127: Показания счетчика импульсов 3
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 128: Показания счетчика импульсов 4
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 129: Показания счетчика импульсов 5
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 130: Показания счетчика импульсов 6
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 131: Показания счетчика импульсов 7
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 132: Показания счетчика импульсов 8
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 133: Частота на аналогово-частотном датчике 3
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 134: Частота на аналогово-частотном датчике 4
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 135: Частота на аналогово-частотном датчике 5
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 136: Частота на аналогово-частотном датчике 6
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 137: Частота на аналогово-частотном датчике 7
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 138: Частота на аналогово-частотном датчике 8
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 139: Состояние виртуальных датчиков акселерометра
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 140: Внутренний датчик угла наклона.Угол наклона относительно местной вертикали
   { 2, TelemetryDataType::I8, nullptr, nullptr },      // 141: Внутренний датчик наклона. Углы наклона относительно отвесной линии
   { 3, TelemetryDataType::U8, nullptr, nullptr },      // 142: Внешний датчик угла наклона. Отклонения по осям
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 143: EcoDriving. Максимальное значение вертикального ускорения за период
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 144: EcoDriving. Максимальное значение скорости за период
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 145: EcoDriving. Состояние порогов скорости
   { 3, TelemetryDataType::U8, nullptr, nullptr },      // 146: EcoDriving. Состояние порогов ускорения
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 147: Частота на выходе ДУТ 485 No1
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 148: Частота на выходе ДУТ 485 No2
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 149: Частота на выходе ДУТ 485 No3
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 150: Частота на выходе ДУТ 485 No4
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 151: Частота на выходе ДУТ 485 No5
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 152: Частота на выходе ДУТ 485 No6
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 153: Частота на выходе ДУТ 485 No7
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 154: Частота на выходе ДУТ 485 No8
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 155: Частота на выходе ДУТ 485 No9
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 156: Частота на выходе ДУТ 485 No10
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 157: Частота на выходе ДУТ 485 No11
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 158: Частота на выходе ДУТ 485 No12
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 159: Частота на выходе ДУТ 485 No13
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 160: Частота на выходе ДУТ 485 No14
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 161: Частота на выходе ДУТ 485 No15
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 162: Частота на выходе ДУТ 485 No16
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 163: Высокоточный датчик температуры 1
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 164: Высокоточный датчик температуры 2
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 165: Высокоточный датчик температуры 3
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 166: Высокоточный датчик температуры 4
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 167: Высокоточный датчик влажности 1
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 168: Высокоточный датчик влажности 2
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 169: Высокоточный датчик влажности 3
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 170: Высокоточный датчик влажности 4
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 171: Датчик расхода топлива. Статус датчика
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 172: Датчик расхода топлива. Информация о неисправностях
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 173: Датчик расхода топлива. Суммарный расход топлива
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 174: Датчик расхода топлива. Расход топлива за поездку
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 175: Датчик расхода топлива. Текущая скорость потока
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 176: Датчик расхода топлива. Суммарный объем топлива камеры подачи
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 177: Датчик расхода топлива. Текущая скорость потока камеры подачи
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 178: Датчик расхода топлива. Температура камеры подачи
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 179: Датчик расхода топлива. Суммарный объем топлива камеры обратки
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 180: Датчик расхода топлива. Текущая скорость потока камеры обратки
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 181: Датчик расхода топлива. Температура камеры обратки
   { 2, TelemetryDataType::U8, nullptr, nullptr },      // 182: Рефрижераторная установка. Состояние установки
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 183: Рефрижераторная установка. Температура рефрижератора в секции 1 (Температура ХОУ)
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 184: Рефрижераторная установка. Температура рефрижератора в секции 2
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 185: Рефрижераторная установка. Температура рефрижератора в секции 3
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 186: Рефрижераторная установка. Температура установленная 1
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 187: Рефрижераторная установка. Температура установленная 2
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 188: Рефрижераторная установка. Температура установленная 3
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 189: Рефрижераторная установка. Температура окружающего воздуха
   { 2, TelemetryDataType::I16, nullptr, nullptr },     // 190: Рефрижераторная установка. Температура ОЖ
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 191: Рефрижераторная установка. Напряжение аккумулятора
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 192: Рефрижераторная установка. Сила тока аккумулятора
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 193: Рефрижераторная установка. Моточасы работы от двигателя
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 194: Рефрижераторная установка. Моточасы работы от сети
   { 4, TelemetryDataType::U16, nullptr, nullptr },     // 195: Рефрижераторная установка. Количество ошибок. Код самой важной ошибки
   { 4, TelemetryDataType::U16, nullptr, nullptr },     // 196: Рефрижераторная установка. Код 2й и 3й по важности ошибки
   { 6, TelemetryDataType::U16, nullptr, nullptr },     // 197: Рефрижераторная установка. Код 4й-6й по важности ошибки
   { 3, TelemetryDataType::STRUCT, nullptr, nullptr },  // 198: Рефрижераторная установка. Состояние двигателя
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 199: Рефрижераторная установка. Конфигурация компрессора
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 200: Информация о нахождении в геозонах
   { 2, TelemetryDataType::U16, nullptr, nullptr },     // 201: CAN. Флаги состояния безопасности
   { 1, TelemetryDataType::U8, nullptr, nullptr },      // 202: CAN. События состояния безопасности
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 203: CAN. Контроллеры аварии
   { 5, TelemetryDataType::STRUCT, nullptr, nullptr },  // 204: CAN. Информация о неисправностях
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 205: Пользовательские моточасы 1 (работа под нагрузкой)
   { 4, TelemetryDataType::U32, nullptr, nullptr },     // 206: Пользовательские моточасы 2 (работа без нагрузки)
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 207: Пользовательский параметр 1 байт No1
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 208: Пользовательский параметр 1 байт No2
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 209: Пользовательский параметр 1 байт No3
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 210: Пользовательский параметр 1 байт No4
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 211: Пользовательский параметр 1 байт No5
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 212: Пользовательский параметр 1 байт No6
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 213: Пользовательский параметр 1 байт No7
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 214: Пользовательский параметр 1 байт No8
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 215: Пользовательский параметр 1 байт No9
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 216: Пользовательский параметр 1 байт No10
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 217: Пользовательский параметр 1 байт No11
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 218: Пользовательский параметр 1 байт No12
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 219: Пользовательский параметр 1 байт No13
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 220: Пользовательский параметр 1 байт No14
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 221: Пользовательский параметр 1 байт No15
   { 1, TelemetryDataType::U8, nullptr, nullptr },    // 222: Пользовательский параметр 1 байт No16
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 223: Пользовательский параметр 2 байта No1
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 224: Пользовательский параметр 2 байта No2
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 225: Пользовательский параметр 2 байта No3
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 226: Пользовательский параметр 2 байта No4
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 227: Пользовательский параметр 2 байта No5
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 228: Пользовательский параметр 2 байта No6
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 229: Пользовательский параметр 2 байта No7
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 230: Пользовательский параметр 2 байта No8
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 231: Пользовательский параметр 2 байта No9
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 232: Пользовательский параметр 2 байта No10
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 233: Пользовательский параметр 2 байта No11
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 234: Пользовательский параметр 2 байта No12
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 235: Пользовательский параметр 2 байта No13
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 236: Пользовательский параметр 2 байта No14
   { 2, TelemetryDataType::U16, nullptr, nullptr },   // 237: Пользовательский параметр 2 байта No15
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 238: Пользовательский параметр 4 байта No1
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 239: Пользовательский параметр 4 байта No2
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 240: Пользовательский параметр 4 байта No3
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 241: Пользовательский параметр 4 байта No4
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 242: Пользовательский параметр 4 байта No5
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 243: Пользовательский параметр 4 байта No6
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 244: Пользовательский параметр 4 байта No7
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 245: Пользовательский параметр 4 байта No8
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 246: Пользовательский параметр 4 байта No9
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 247: Пользовательский параметр 4 байта No10
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 248: Пользовательский параметр 4 байта No11
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 249: Пользовательский параметр 4 байта No12
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 250: Пользовательский параметр 4 байта No13
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 251: Пользовательский параметр 4 байта No14
   { 4, TelemetryDataType::U32, nullptr, nullptr },   // 252: Пользовательский параметр 4 байта No15
   { 8, TelemetryDataType::U64, nullptr, nullptr },   // 253: Пользовательский параметр 8 байт No1
   { 8, TelemetryDataType::U64, nullptr, nullptr },   // 254: Пользовательский параметр 8 байт No2
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
TelemetryRecord::TelemetryRecord(const shared_ptr<MobileDevice>& device) : m_device(device)
{
   m_timestamp = time(nullptr);
   m_deviceStatus.commProtocol = _T("NTCB/FLEX");
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
      field->handler(field->dataType, value, &m_pushList);
   }
}

/**
 * Update device with collected data
 */
void TelemetryRecord::updateDevice()
{
   m_device->updateStatus(m_deviceStatus);

   auto it = m_pushList.iterator();
   while(it->hasNext())
   {
      auto data = it->next();
      shared_ptr<DCObject> dci = m_device->getDCObjectByName(data->first, 0);
      if ((dci == nullptr) || (dci->getDataSource() != DS_PUSH_AGENT) || (dci->getType() != DCO_TYPE_ITEM))
      {
         nxlog_debug_tag(DEBUG_TAG_NTCB, 5, _T("DCI %s on device %s [%u] %s"), data->first, m_device->getName(), m_device->getId(),
                  (dci == nullptr) ? _T("does not exist") : _T("is of incompatible type"));
         continue;
      }
      m_device->processNewDCValue(dci, m_timestamp, const_cast<TCHAR*>(data->second));
   }
   delete it;
}

/**
 * Read single FLEX telemetry record
 */
bool NTCBDeviceSession::readTelemetryRecord()
{
   shared_ptr<TelemetryRecord> record = make_shared<TelemetryRecord>(m_device);

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
