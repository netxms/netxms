/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: unicode_case.cpp
**
** Locale independent Unicode case conversion.
**
** The C library wide character functions (towupper, towlower, wcscasecmp and
** friends) fold according to LC_CTYPE, so in the C locale they only handle
** ASCII, and implementations disagree on everything else. NetXMS must evaluate
** scripts and match strings identically regardless of how the process was
** started, so case conversion is done here from Unicode simple case mappings
** instead.
**
** Locale specific mappings (Turkish and Azeri dotless i, Lithuanian accented i)
** and multi-character expansions (German sharp s to SS) are not implemented -
** these cannot be expressed as a per-character mapping.
**
**/

#include "libnetxms.h"

/**
 * Case mapping range. Codepoints start, start + step, start + 2 * step, ... end
 * are mapped to codepoint + delta.
 */
struct CaseMappingRange
{
   uint32_t start;
   uint32_t end;
   uint32_t step;
   int32_t delta;
};

/* BEGIN GENERATED TABLES */

/**
 * Simple uppercase mappings from Unicode 17.0.0
 */
static const CaseMappingRange s_upperCaseMapping[] =
{
   { 0x00061, 0x0007A, 1, -32 },
   { 0x000B5, 0x000B5, 1, 743 },
   { 0x000E0, 0x000F6, 1, -32 },
   { 0x000F8, 0x000FE, 1, -32 },
   { 0x000FF, 0x000FF, 1, 121 },
   { 0x00101, 0x0012F, 2, -1 },
   { 0x00131, 0x00131, 1, -232 },
   { 0x00133, 0x00137, 2, -1 },
   { 0x0013A, 0x00148, 2, -1 },
   { 0x0014B, 0x00177, 2, -1 },
   { 0x0017A, 0x0017E, 2, -1 },
   { 0x0017F, 0x0017F, 1, -300 },
   { 0x00180, 0x00180, 1, 195 },
   { 0x00183, 0x00185, 2, -1 },
   { 0x00188, 0x00188, 1, -1 },
   { 0x0018C, 0x0018C, 1, -1 },
   { 0x00192, 0x00192, 1, -1 },
   { 0x00195, 0x00195, 1, 97 },
   { 0x00199, 0x00199, 1, -1 },
   { 0x0019A, 0x0019A, 1, 163 },
   { 0x0019B, 0x0019B, 1, 42561 },
   { 0x0019E, 0x0019E, 1, 130 },
   { 0x001A1, 0x001A5, 2, -1 },
   { 0x001A8, 0x001A8, 1, -1 },
   { 0x001AD, 0x001AD, 1, -1 },
   { 0x001B0, 0x001B0, 1, -1 },
   { 0x001B4, 0x001B6, 2, -1 },
   { 0x001B9, 0x001B9, 1, -1 },
   { 0x001BD, 0x001BD, 1, -1 },
   { 0x001BF, 0x001BF, 1, 56 },
   { 0x001C5, 0x001C5, 1, -1 },
   { 0x001C6, 0x001C6, 1, -2 },
   { 0x001C8, 0x001C8, 1, -1 },
   { 0x001C9, 0x001C9, 1, -2 },
   { 0x001CB, 0x001CB, 1, -1 },
   { 0x001CC, 0x001CC, 1, -2 },
   { 0x001CE, 0x001DC, 2, -1 },
   { 0x001DD, 0x001DD, 1, -79 },
   { 0x001DF, 0x001EF, 2, -1 },
   { 0x001F2, 0x001F2, 1, -1 },
   { 0x001F3, 0x001F3, 1, -2 },
   { 0x001F5, 0x001F5, 1, -1 },
   { 0x001F9, 0x0021F, 2, -1 },
   { 0x00223, 0x00233, 2, -1 },
   { 0x0023C, 0x0023C, 1, -1 },
   { 0x0023F, 0x00240, 1, 10815 },
   { 0x00242, 0x00242, 1, -1 },
   { 0x00247, 0x0024F, 2, -1 },
   { 0x00250, 0x00250, 1, 10783 },
   { 0x00251, 0x00251, 1, 10780 },
   { 0x00252, 0x00252, 1, 10782 },
   { 0x00253, 0x00253, 1, -210 },
   { 0x00254, 0x00254, 1, -206 },
   { 0x00256, 0x00257, 1, -205 },
   { 0x00259, 0x00259, 1, -202 },
   { 0x0025B, 0x0025B, 1, -203 },
   { 0x0025C, 0x0025C, 1, 42319 },
   { 0x00260, 0x00260, 1, -205 },
   { 0x00261, 0x00261, 1, 42315 },
   { 0x00263, 0x00263, 1, -207 },
   { 0x00264, 0x00264, 1, 42343 },
   { 0x00265, 0x00265, 1, 42280 },
   { 0x00266, 0x00266, 1, 42308 },
   { 0x00268, 0x00268, 1, -209 },
   { 0x00269, 0x00269, 1, -211 },
   { 0x0026A, 0x0026A, 1, 42308 },
   { 0x0026B, 0x0026B, 1, 10743 },
   { 0x0026C, 0x0026C, 1, 42305 },
   { 0x0026F, 0x0026F, 1, -211 },
   { 0x00271, 0x00271, 1, 10749 },
   { 0x00272, 0x00272, 1, -213 },
   { 0x00275, 0x00275, 1, -214 },
   { 0x0027D, 0x0027D, 1, 10727 },
   { 0x00280, 0x00280, 1, -218 },
   { 0x00282, 0x00282, 1, 42307 },
   { 0x00283, 0x00283, 1, -218 },
   { 0x00287, 0x00287, 1, 42282 },
   { 0x00288, 0x00288, 1, -218 },
   { 0x00289, 0x00289, 1, -69 },
   { 0x0028A, 0x0028B, 1, -217 },
   { 0x0028C, 0x0028C, 1, -71 },
   { 0x00292, 0x00292, 1, -219 },
   { 0x0029D, 0x0029D, 1, 42261 },
   { 0x0029E, 0x0029E, 1, 42258 },
   { 0x00345, 0x00345, 1, 84 },
   { 0x00371, 0x00373, 2, -1 },
   { 0x00377, 0x00377, 1, -1 },
   { 0x0037B, 0x0037D, 1, 130 },
   { 0x003AC, 0x003AC, 1, -38 },
   { 0x003AD, 0x003AF, 1, -37 },
   { 0x003B1, 0x003C1, 1, -32 },
   { 0x003C2, 0x003C2, 1, -31 },
   { 0x003C3, 0x003CB, 1, -32 },
   { 0x003CC, 0x003CC, 1, -64 },
   { 0x003CD, 0x003CE, 1, -63 },
   { 0x003D0, 0x003D0, 1, -62 },
   { 0x003D1, 0x003D1, 1, -57 },
   { 0x003D5, 0x003D5, 1, -47 },
   { 0x003D6, 0x003D6, 1, -54 },
   { 0x003D7, 0x003D7, 1, -8 },
   { 0x003D9, 0x003EF, 2, -1 },
   { 0x003F0, 0x003F0, 1, -86 },
   { 0x003F1, 0x003F1, 1, -80 },
   { 0x003F2, 0x003F2, 1, 7 },
   { 0x003F3, 0x003F3, 1, -116 },
   { 0x003F5, 0x003F5, 1, -96 },
   { 0x003F8, 0x003F8, 1, -1 },
   { 0x003FB, 0x003FB, 1, -1 },
   { 0x00430, 0x0044F, 1, -32 },
   { 0x00450, 0x0045F, 1, -80 },
   { 0x00461, 0x00481, 2, -1 },
   { 0x0048B, 0x004BF, 2, -1 },
   { 0x004C2, 0x004CE, 2, -1 },
   { 0x004CF, 0x004CF, 1, -15 },
   { 0x004D1, 0x0052F, 2, -1 },
   { 0x00561, 0x00586, 1, -48 },
   { 0x010D0, 0x010FA, 1, 3008 },
   { 0x010FD, 0x010FF, 1, 3008 },
   { 0x013F8, 0x013FD, 1, -8 },
   { 0x01C80, 0x01C80, 1, -6254 },
   { 0x01C81, 0x01C81, 1, -6253 },
   { 0x01C82, 0x01C82, 1, -6244 },
   { 0x01C83, 0x01C84, 1, -6242 },
   { 0x01C85, 0x01C85, 1, -6243 },
   { 0x01C86, 0x01C86, 1, -6236 },
   { 0x01C87, 0x01C87, 1, -6181 },
   { 0x01C88, 0x01C88, 1, 35266 },
   { 0x01C8A, 0x01C8A, 1, -1 },
   { 0x01D79, 0x01D79, 1, 35332 },
   { 0x01D7D, 0x01D7D, 1, 3814 },
   { 0x01D8E, 0x01D8E, 1, 35384 },
   { 0x01E01, 0x01E95, 2, -1 },
   { 0x01E9B, 0x01E9B, 1, -59 },
   { 0x01EA1, 0x01EFF, 2, -1 },
   { 0x01F00, 0x01F07, 1, 8 },
   { 0x01F10, 0x01F15, 1, 8 },
   { 0x01F20, 0x01F27, 1, 8 },
   { 0x01F30, 0x01F37, 1, 8 },
   { 0x01F40, 0x01F45, 1, 8 },
   { 0x01F51, 0x01F57, 2, 8 },
   { 0x01F60, 0x01F67, 1, 8 },
   { 0x01F70, 0x01F71, 1, 74 },
   { 0x01F72, 0x01F75, 1, 86 },
   { 0x01F76, 0x01F77, 1, 100 },
   { 0x01F78, 0x01F79, 1, 128 },
   { 0x01F7A, 0x01F7B, 1, 112 },
   { 0x01F7C, 0x01F7D, 1, 126 },
   { 0x01F80, 0x01F87, 1, 8 },
   { 0x01F90, 0x01F97, 1, 8 },
   { 0x01FA0, 0x01FA7, 1, 8 },
   { 0x01FB0, 0x01FB1, 1, 8 },
   { 0x01FB3, 0x01FB3, 1, 9 },
   { 0x01FBE, 0x01FBE, 1, -7205 },
   { 0x01FC3, 0x01FC3, 1, 9 },
   { 0x01FD0, 0x01FD1, 1, 8 },
   { 0x01FE0, 0x01FE1, 1, 8 },
   { 0x01FE5, 0x01FE5, 1, 7 },
   { 0x01FF3, 0x01FF3, 1, 9 },
   { 0x0214E, 0x0214E, 1, -28 },
   { 0x02170, 0x0217F, 1, -16 },
   { 0x02184, 0x02184, 1, -1 },
   { 0x024D0, 0x024E9, 1, -26 },
   { 0x02C30, 0x02C5F, 1, -48 },
   { 0x02C61, 0x02C61, 1, -1 },
   { 0x02C65, 0x02C65, 1, -10795 },
   { 0x02C66, 0x02C66, 1, -10792 },
   { 0x02C68, 0x02C6C, 2, -1 },
   { 0x02C73, 0x02C73, 1, -1 },
   { 0x02C76, 0x02C76, 1, -1 },
   { 0x02C81, 0x02CE3, 2, -1 },
   { 0x02CEC, 0x02CEE, 2, -1 },
   { 0x02CF3, 0x02CF3, 1, -1 },
   { 0x02D00, 0x02D25, 1, -7264 },
   { 0x02D27, 0x02D27, 1, -7264 },
   { 0x02D2D, 0x02D2D, 1, -7264 },
   { 0x0A641, 0x0A66D, 2, -1 },
   { 0x0A681, 0x0A69B, 2, -1 },
   { 0x0A723, 0x0A72F, 2, -1 },
   { 0x0A733, 0x0A76F, 2, -1 },
   { 0x0A77A, 0x0A77C, 2, -1 },
   { 0x0A77F, 0x0A787, 2, -1 },
   { 0x0A78C, 0x0A78C, 1, -1 },
   { 0x0A791, 0x0A793, 2, -1 },
   { 0x0A794, 0x0A794, 1, 48 },
   { 0x0A797, 0x0A7A9, 2, -1 },
   { 0x0A7B5, 0x0A7C3, 2, -1 },
   { 0x0A7C8, 0x0A7CA, 2, -1 },
   { 0x0A7CD, 0x0A7DB, 2, -1 },
   { 0x0A7F6, 0x0A7F6, 1, -1 },
   { 0x0AB53, 0x0AB53, 1, -928 },
   { 0x0AB70, 0x0ABBF, 1, -38864 },
   { 0x0FF41, 0x0FF5A, 1, -32 },
   { 0x10428, 0x1044F, 1, -40 },
   { 0x104D8, 0x104FB, 1, -40 },
   { 0x10597, 0x105A1, 1, -39 },
   { 0x105A3, 0x105B1, 1, -39 },
   { 0x105B3, 0x105B9, 1, -39 },
   { 0x105BB, 0x105BC, 1, -39 },
   { 0x10CC0, 0x10CF2, 1, -64 },
   { 0x10D70, 0x10D85, 1, -32 },
   { 0x118C0, 0x118DF, 1, -32 },
   { 0x16E60, 0x16E7F, 1, -32 },
   { 0x16EBB, 0x16ED3, 1, -27 },
   { 0x1E922, 0x1E943, 1, -34 },
};

/**
 * Simple lowercase mappings from Unicode 17.0.0
 */
static const CaseMappingRange s_lowerCaseMapping[] =
{
   { 0x00041, 0x0005A, 1, 32 },
   { 0x000C0, 0x000D6, 1, 32 },
   { 0x000D8, 0x000DE, 1, 32 },
   { 0x00100, 0x0012E, 2, 1 },
   { 0x00130, 0x00130, 1, -199 },
   { 0x00132, 0x00136, 2, 1 },
   { 0x00139, 0x00147, 2, 1 },
   { 0x0014A, 0x00176, 2, 1 },
   { 0x00178, 0x00178, 1, -121 },
   { 0x00179, 0x0017D, 2, 1 },
   { 0x00181, 0x00181, 1, 210 },
   { 0x00182, 0x00184, 2, 1 },
   { 0x00186, 0x00186, 1, 206 },
   { 0x00187, 0x00187, 1, 1 },
   { 0x00189, 0x0018A, 1, 205 },
   { 0x0018B, 0x0018B, 1, 1 },
   { 0x0018E, 0x0018E, 1, 79 },
   { 0x0018F, 0x0018F, 1, 202 },
   { 0x00190, 0x00190, 1, 203 },
   { 0x00191, 0x00191, 1, 1 },
   { 0x00193, 0x00193, 1, 205 },
   { 0x00194, 0x00194, 1, 207 },
   { 0x00196, 0x00196, 1, 211 },
   { 0x00197, 0x00197, 1, 209 },
   { 0x00198, 0x00198, 1, 1 },
   { 0x0019C, 0x0019C, 1, 211 },
   { 0x0019D, 0x0019D, 1, 213 },
   { 0x0019F, 0x0019F, 1, 214 },
   { 0x001A0, 0x001A4, 2, 1 },
   { 0x001A6, 0x001A6, 1, 218 },
   { 0x001A7, 0x001A7, 1, 1 },
   { 0x001A9, 0x001A9, 1, 218 },
   { 0x001AC, 0x001AC, 1, 1 },
   { 0x001AE, 0x001AE, 1, 218 },
   { 0x001AF, 0x001AF, 1, 1 },
   { 0x001B1, 0x001B2, 1, 217 },
   { 0x001B3, 0x001B5, 2, 1 },
   { 0x001B7, 0x001B7, 1, 219 },
   { 0x001B8, 0x001B8, 1, 1 },
   { 0x001BC, 0x001BC, 1, 1 },
   { 0x001C4, 0x001C4, 1, 2 },
   { 0x001C5, 0x001C5, 1, 1 },
   { 0x001C7, 0x001C7, 1, 2 },
   { 0x001C8, 0x001C8, 1, 1 },
   { 0x001CA, 0x001CA, 1, 2 },
   { 0x001CB, 0x001DB, 2, 1 },
   { 0x001DE, 0x001EE, 2, 1 },
   { 0x001F1, 0x001F1, 1, 2 },
   { 0x001F2, 0x001F4, 2, 1 },
   { 0x001F6, 0x001F6, 1, -97 },
   { 0x001F7, 0x001F7, 1, -56 },
   { 0x001F8, 0x0021E, 2, 1 },
   { 0x00220, 0x00220, 1, -130 },
   { 0x00222, 0x00232, 2, 1 },
   { 0x0023A, 0x0023A, 1, 10795 },
   { 0x0023B, 0x0023B, 1, 1 },
   { 0x0023D, 0x0023D, 1, -163 },
   { 0x0023E, 0x0023E, 1, 10792 },
   { 0x00241, 0x00241, 1, 1 },
   { 0x00243, 0x00243, 1, -195 },
   { 0x00244, 0x00244, 1, 69 },
   { 0x00245, 0x00245, 1, 71 },
   { 0x00246, 0x0024E, 2, 1 },
   { 0x00370, 0x00372, 2, 1 },
   { 0x00376, 0x00376, 1, 1 },
   { 0x0037F, 0x0037F, 1, 116 },
   { 0x00386, 0x00386, 1, 38 },
   { 0x00388, 0x0038A, 1, 37 },
   { 0x0038C, 0x0038C, 1, 64 },
   { 0x0038E, 0x0038F, 1, 63 },
   { 0x00391, 0x003A1, 1, 32 },
   { 0x003A3, 0x003AB, 1, 32 },
   { 0x003CF, 0x003CF, 1, 8 },
   { 0x003D8, 0x003EE, 2, 1 },
   { 0x003F4, 0x003F4, 1, -60 },
   { 0x003F7, 0x003F7, 1, 1 },
   { 0x003F9, 0x003F9, 1, -7 },
   { 0x003FA, 0x003FA, 1, 1 },
   { 0x003FD, 0x003FF, 1, -130 },
   { 0x00400, 0x0040F, 1, 80 },
   { 0x00410, 0x0042F, 1, 32 },
   { 0x00460, 0x00480, 2, 1 },
   { 0x0048A, 0x004BE, 2, 1 },
   { 0x004C0, 0x004C0, 1, 15 },
   { 0x004C1, 0x004CD, 2, 1 },
   { 0x004D0, 0x0052E, 2, 1 },
   { 0x00531, 0x00556, 1, 48 },
   { 0x010A0, 0x010C5, 1, 7264 },
   { 0x010C7, 0x010C7, 1, 7264 },
   { 0x010CD, 0x010CD, 1, 7264 },
   { 0x013A0, 0x013EF, 1, 38864 },
   { 0x013F0, 0x013F5, 1, 8 },
   { 0x01C89, 0x01C89, 1, 1 },
   { 0x01C90, 0x01CBA, 1, -3008 },
   { 0x01CBD, 0x01CBF, 1, -3008 },
   { 0x01E00, 0x01E94, 2, 1 },
   { 0x01E9E, 0x01E9E, 1, -7615 },
   { 0x01EA0, 0x01EFE, 2, 1 },
   { 0x01F08, 0x01F0F, 1, -8 },
   { 0x01F18, 0x01F1D, 1, -8 },
   { 0x01F28, 0x01F2F, 1, -8 },
   { 0x01F38, 0x01F3F, 1, -8 },
   { 0x01F48, 0x01F4D, 1, -8 },
   { 0x01F59, 0x01F5F, 2, -8 },
   { 0x01F68, 0x01F6F, 1, -8 },
   { 0x01F88, 0x01F8F, 1, -8 },
   { 0x01F98, 0x01F9F, 1, -8 },
   { 0x01FA8, 0x01FAF, 1, -8 },
   { 0x01FB8, 0x01FB9, 1, -8 },
   { 0x01FBA, 0x01FBB, 1, -74 },
   { 0x01FBC, 0x01FBC, 1, -9 },
   { 0x01FC8, 0x01FCB, 1, -86 },
   { 0x01FCC, 0x01FCC, 1, -9 },
   { 0x01FD8, 0x01FD9, 1, -8 },
   { 0x01FDA, 0x01FDB, 1, -100 },
   { 0x01FE8, 0x01FE9, 1, -8 },
   { 0x01FEA, 0x01FEB, 1, -112 },
   { 0x01FEC, 0x01FEC, 1, -7 },
   { 0x01FF8, 0x01FF9, 1, -128 },
   { 0x01FFA, 0x01FFB, 1, -126 },
   { 0x01FFC, 0x01FFC, 1, -9 },
   { 0x02126, 0x02126, 1, -7517 },
   { 0x0212A, 0x0212A, 1, -8383 },
   { 0x0212B, 0x0212B, 1, -8262 },
   { 0x02132, 0x02132, 1, 28 },
   { 0x02160, 0x0216F, 1, 16 },
   { 0x02183, 0x02183, 1, 1 },
   { 0x024B6, 0x024CF, 1, 26 },
   { 0x02C00, 0x02C2F, 1, 48 },
   { 0x02C60, 0x02C60, 1, 1 },
   { 0x02C62, 0x02C62, 1, -10743 },
   { 0x02C63, 0x02C63, 1, -3814 },
   { 0x02C64, 0x02C64, 1, -10727 },
   { 0x02C67, 0x02C6B, 2, 1 },
   { 0x02C6D, 0x02C6D, 1, -10780 },
   { 0x02C6E, 0x02C6E, 1, -10749 },
   { 0x02C6F, 0x02C6F, 1, -10783 },
   { 0x02C70, 0x02C70, 1, -10782 },
   { 0x02C72, 0x02C72, 1, 1 },
   { 0x02C75, 0x02C75, 1, 1 },
   { 0x02C7E, 0x02C7F, 1, -10815 },
   { 0x02C80, 0x02CE2, 2, 1 },
   { 0x02CEB, 0x02CED, 2, 1 },
   { 0x02CF2, 0x02CF2, 1, 1 },
   { 0x0A640, 0x0A66C, 2, 1 },
   { 0x0A680, 0x0A69A, 2, 1 },
   { 0x0A722, 0x0A72E, 2, 1 },
   { 0x0A732, 0x0A76E, 2, 1 },
   { 0x0A779, 0x0A77B, 2, 1 },
   { 0x0A77D, 0x0A77D, 1, -35332 },
   { 0x0A77E, 0x0A786, 2, 1 },
   { 0x0A78B, 0x0A78B, 1, 1 },
   { 0x0A78D, 0x0A78D, 1, -42280 },
   { 0x0A790, 0x0A792, 2, 1 },
   { 0x0A796, 0x0A7A8, 2, 1 },
   { 0x0A7AA, 0x0A7AA, 1, -42308 },
   { 0x0A7AB, 0x0A7AB, 1, -42319 },
   { 0x0A7AC, 0x0A7AC, 1, -42315 },
   { 0x0A7AD, 0x0A7AD, 1, -42305 },
   { 0x0A7AE, 0x0A7AE, 1, -42308 },
   { 0x0A7B0, 0x0A7B0, 1, -42258 },
   { 0x0A7B1, 0x0A7B1, 1, -42282 },
   { 0x0A7B2, 0x0A7B2, 1, -42261 },
   { 0x0A7B3, 0x0A7B3, 1, 928 },
   { 0x0A7B4, 0x0A7C2, 2, 1 },
   { 0x0A7C4, 0x0A7C4, 1, -48 },
   { 0x0A7C5, 0x0A7C5, 1, -42307 },
   { 0x0A7C6, 0x0A7C6, 1, -35384 },
   { 0x0A7C7, 0x0A7C9, 2, 1 },
   { 0x0A7CB, 0x0A7CB, 1, -42343 },
   { 0x0A7CC, 0x0A7DA, 2, 1 },
   { 0x0A7DC, 0x0A7DC, 1, -42561 },
   { 0x0A7F5, 0x0A7F5, 1, 1 },
   { 0x0FF21, 0x0FF3A, 1, 32 },
   { 0x10400, 0x10427, 1, 40 },
   { 0x104B0, 0x104D3, 1, 40 },
   { 0x10570, 0x1057A, 1, 39 },
   { 0x1057C, 0x1058A, 1, 39 },
   { 0x1058C, 0x10592, 1, 39 },
   { 0x10594, 0x10595, 1, 39 },
   { 0x10C80, 0x10CB2, 1, 64 },
   { 0x10D50, 0x10D65, 1, 32 },
   { 0x118A0, 0x118BF, 1, 32 },
   { 0x16E40, 0x16E5F, 1, 32 },
   { 0x16EA0, 0x16EB8, 1, 27 },
   { 0x1E900, 0x1E921, 1, 34 },
};

/* END GENERATED TABLES */

/**
 * Map single codepoint using given table
 */
static inline uint32_t MapCodepoint(const CaseMappingRange *table, size_t size, uint32_t codepoint)
{
   size_t first = 0, last = size;
   while(first < last)
   {
      size_t mid = first + (last - first) / 2;
      if (codepoint < table[mid].start)
         last = mid;
      else if (codepoint > table[mid].end)
         first = mid + 1;
      else
         return ((codepoint - table[mid].start) % table[mid].step == 0) ? codepoint + table[mid].delta : codepoint;
   }
   return codepoint;
}

/**
 * Convert single character to uppercase
 */
wchar_t LIBNETXMS_EXPORTABLE nx_towupper(wchar_t ch)
{
   uint32_t codepoint = static_cast<uint32_t>(ch);
   if (codepoint < 128)
      return ((codepoint >= 'a') && (codepoint <= 'z')) ? static_cast<wchar_t>(codepoint - 32) : ch;
   return static_cast<wchar_t>(MapCodepoint(s_upperCaseMapping, sizeof(s_upperCaseMapping) / sizeof(CaseMappingRange), codepoint));
}

/**
 * Convert single character to lowercase
 */
wchar_t LIBNETXMS_EXPORTABLE nx_towlower(wchar_t ch)
{
   uint32_t codepoint = static_cast<uint32_t>(ch);
   if (codepoint < 128)
      return ((codepoint >= 'A') && (codepoint <= 'Z')) ? static_cast<wchar_t>(codepoint + 32) : ch;
   return static_cast<wchar_t>(MapCodepoint(s_lowerCaseMapping, sizeof(s_lowerCaseMapping) / sizeof(CaseMappingRange), codepoint));
}

/**
 * Compare two strings ignoring case
 */
int LIBNETXMS_EXPORTABLE nx_wcsicmp(const wchar_t *s1, const wchar_t *s2)
{
   if (s1 == s2)
      return 0;

   wchar_t c1, c2;
   do
   {
      c1 = nx_towupper(*s1++);
      c2 = nx_towupper(*s2++);
      if (c1 == 0)
         break;
   } while(c1 == c2);
   return COMPARE_NUMBERS(static_cast<uint32_t>(c1), static_cast<uint32_t>(c2));
}

/**
 * Compare first n characters of two strings ignoring case
 */
int LIBNETXMS_EXPORTABLE nx_wcsnicmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
   if ((s1 == s2) || (n == 0))
      return 0;

   wchar_t c1, c2;
   do
   {
      c1 = nx_towupper(*s1++);
      c2 = nx_towupper(*s2++);
      if (c1 == 0)
         break;
   } while((c1 == c2) && (--n > 0));
   return (c1 == c2) ? 0 : COMPARE_NUMBERS(static_cast<uint32_t>(c1), static_cast<uint32_t>(c2));
}

/**
 * Find first occurrence of substring in string ignoring case
 */
wchar_t LIBNETXMS_EXPORTABLE *nx_wcsistr(const wchar_t *s, const wchar_t *ss)
{
   if (*ss == 0)
      return const_cast<wchar_t*>(s);

   wchar_t first = nx_towupper(*ss);
   for(const wchar_t *p = s; *p != 0; p++)
   {
      if (nx_towupper(*p) != first)
         continue;

      const wchar_t *m = p + 1, *n = ss + 1;
      while((*n != 0) && (nx_towupper(*m) == nx_towupper(*n)))
      {
         m++;
         n++;
      }
      if (*n == 0)
         return const_cast<wchar_t*>(p);
   }
   return nullptr;
}

/**
 * Convert string to uppercase in place
 */
wchar_t LIBNETXMS_EXPORTABLE *nx_wcsupr(wchar_t *s)
{
   for(wchar_t *p = s; *p != 0; p++)
      *p = nx_towupper(*p);
   return s;
}

/**
 * Convert string to lowercase in place
 */
wchar_t LIBNETXMS_EXPORTABLE *nx_wcslwr(wchar_t *s)
{
   for(wchar_t *p = s; *p != 0; p++)
      *p = nx_towlower(*p);
   return s;
}
