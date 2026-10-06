/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: format.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>

/**
 * Test FormatNumber()
 */
void TestFormatNumber()
{
   StartTest(_T("FormatNumber - decimal prefixes"));
   AssertEquals(FormatNumber(512, false, 0, 2).cstr(), _T("512.00"));
   AssertEquals(FormatNumber(1536, false, 0, 2).cstr(), _T("1.54 k"));
   AssertEquals(FormatNumber(2500000, false, 0, 2).cstr(), _T("2.50 M"));
   AssertEquals(FormatNumber(-2500000, false, 0, 2).cstr(), _T("-2.50 M"));
   AssertEquals(FormatNumber(3e9, false, 0, 1).cstr(), _T("3.0 G"));
   AssertEquals(FormatNumber(4e12, false, 0, 0).cstr(), _T("4 T"));
   AssertEquals(FormatNumber(5e15, false, 0, 0).cstr(), _T("5 P"));
   AssertEquals(FormatNumber(6e18, false, 0, 0).cstr(), _T("6000 P"));
   EndTest();

   StartTest(_T("FormatNumber - binary prefixes"));
   AssertEquals(FormatNumber(512, true, 0, 2).cstr(), _T("512.00"));
   AssertEquals(FormatNumber(1536, true, 0, 2).cstr(), _T("1.50 Ki"));
   AssertEquals(FormatNumber(3145728, true, 0, 2).cstr(), _T("3.00 Mi"));
   AssertEquals(FormatNumber(1073741824.0, true, 0, 0).cstr(), _T("1 Gi"));
   EndTest();

   StartTest(_T("FormatNumber - small numbers"));
   AssertEquals(FormatNumber(0.005, false, 0, 2).cstr(), _T("5.00 m"));
   AssertEquals(FormatNumber(0.000002, false, 0, 2).cstr(), _T("2.00 μ"));
   AssertEquals(FormatNumber(0.000000003, false, 0, 2).cstr(), _T("3.00 n"));
   AssertEquals(FormatNumber(-0.005, false, 0, 2).cstr(), _T("-5.00 m"));
   EndTest();

   StartTest(_T("FormatNumber - zero"));
   AssertEquals(FormatNumber(0, false, 0, 2).cstr(), _T("0.00"));
   AssertEquals(FormatNumber(0, true, 0, 2).cstr(), _T("0.00"));
   EndTest();

   StartTest(_T("FormatNumber - fixed multiplier power"));
   AssertEquals(FormatNumber(2048, true, 1, 0).cstr(), _T("2 Ki"));
   AssertEquals(FormatNumber(1500, false, 2, 4).cstr(), _T("0.0015 M"));
   AssertEquals(FormatNumber(0.5, false, -1, 0).cstr(), _T("500 m"));
   AssertEquals(FormatNumber(1e20, false, 7, 0).cstr(), _T("100000 P"));
   EndTest();

   StartTest(_T("FormatNumber - negative precision"));
   AssertEquals(FormatNumber(1500, false, 0, -2).cstr(), _T("1.5 k"));
   AssertEquals(FormatNumber(1000, false, 0, -2).cstr(), _T("1 k"));
   AssertEquals(FormatNumber(1234, false, 0, -3).cstr(), _T("1.234 k"));
   AssertEquals(FormatNumber(7, false, 0, -2).cstr(), _T("7"));
   EndTest();

   StartTest(_T("FormatNumber - units"));
   // Prefix present: unit directly follows the prefix symbol (issue #3736)
   AssertEquals(FormatNumber(1536, false, 0, 2, _T("bps")).cstr(), _T("1.54 kbps"));
   AssertEquals(FormatNumber(1536, true, 0, 2, _T("B")).cstr(), _T("1.50 KiB"));
   AssertEquals(FormatNumber(0.005, false, 0, 2, _T("s")).cstr(), _T("5.00 ms"));
   AssertEquals(FormatNumber(2048, true, 1, 0, _T("B")).cstr(), _T("2 KiB"));
   // No prefix: single space between number and unit
   AssertEquals(FormatNumber(512, false, 0, 2, _T("bps")).cstr(), _T("512.00 bps"));
   AssertEquals(FormatNumber(0, false, 0, 2, _T("V")).cstr(), _T("0.00 V"));
   AssertEquals(FormatNumber(7, false, 0, -2, _T("V")).cstr(), _T("7 V"));
   EndTest();
}

/**
 * Test FormatDCIValue()
 */
void TestFormatDCIValue()
{
   StartTest(_T("FormatDCIValue - multipliers and units"));
   AssertEquals(FormatDCIValue(_T("bps"), _T("1536")).cstr(), _T("1.54 kbps"));
   AssertEquals(FormatDCIValue(_T("bps"), _T("512")).cstr(), _T("512.00 bps"));
   AssertEquals(FormatDCIValue(_T("W"), _T("0")).cstr(), _T("0.00 W"));
   AssertEquals(FormatDCIValue(_T("s"), _T("0.005")).cstr(), _T("5.00 ms"));
   AssertEquals(FormatDCIValue(_T(""), _T("1536")).cstr(), _T("1.54 k"));
   AssertEquals(FormatDCIValue(_T("B (Metric)"), _T("1536")).cstr(), _T("1.54 kB"));
   AssertEquals(FormatDCIValue(_T("B (IEC)"), _T("1536")).cstr(), _T("1.50 KiB"));
   AssertEquals(FormatDCIValue(_T("B (IEC)"), _T("512")).cstr(), _T("512.00 B"));
   EndTest();

   StartTest(_T("FormatDCIValue - units exempt from multipliers"));
   AssertEquals(FormatDCIValue(_T("%"), _T("95.5")).cstr(), _T("95.5 %"));
   AssertEquals(FormatDCIValue(_T("°C"), _T("21.5")).cstr(), _T("21.5 °C"));
   AssertEquals(FormatDCIValue(_T("dBm"), _T("-65")).cstr(), _T("-65 dBm"));
   AssertEquals(FormatDCIValue(_T("rpm"), _T("12000")).cstr(), _T("12000 rpm"));
   EndTest();

   StartTest(_T("FormatDCIValue - special units"));
   AssertEquals(FormatDCIValue(_T("Uptime"), _T("90061")).cstr(), _T("1 days,  1:01:01"));
   AssertEquals(FormatDCIValue(_T("Uptime"), _T("abc")).cstr(), _T("abc"));
   AssertEquals(FormatDCIValue(_T("Epoch time"), _T("abc")).cstr(), _T("abc"));
   EndTest();

   StartTest(_T("FormatDCIValue - non-numeric and empty values"));
   AssertEquals(FormatDCIValue(_T("bps"), _T("abc")).cstr(), _T("abc"));
   AssertEquals(FormatDCIValue(_T("bps"), _T("12abc")).cstr(), _T("12abc"));
   AssertEquals(FormatDCIValue(_T("bps"), _T("")).cstr(), _T(""));
   AssertEquals(FormatDCIValue(_T("bps"), nullptr).cstr(), _T(""));
   EndTest();

   StartTest(_T("FormatDCIValue - explicit multiplier mode"));
   AssertEquals(FormatDCIValue(_T("bps"), _T("1536"), 0).cstr(), _T("1.54 kbps"));
   AssertEquals(FormatDCIValue(_T("bps"), _T("1536"), 1).cstr(), _T("1.54 kbps"));
   AssertEquals(FormatDCIValue(_T("bps"), _T("1536"), 2).cstr(), _T("1536 bps"));
   AssertEquals(FormatDCIValue(_T("B (IEC)"), _T("1536"), 2).cstr(), _T("1536 B"));
   AssertEquals(FormatDCIValue(_T(""), _T("1536"), 2).cstr(), _T("1536"));
   AssertEquals(FormatDCIValue(nullptr, _T("1536"), 2).cstr(), _T("1536"));
   AssertEquals(FormatDCIValue(_T("bps"), nullptr, 2).cstr(), _T(""));
   EndTest();
}
