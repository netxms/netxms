/*
** NetXMS - Network Management System
** SMS driver for Portech MV-37x VoIP GSM gateways
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: pdumode.cpp
**
**/

#include "portech.h"

/**
 * Maximum number of 7-bit characters in single SMS
 */
#define MAX_SMS_SEPTETS    160

/**
 * Maximum number of digits in destination address
 */
#define MAX_ADDRESS_DIGITS 20

/**
 * Pack 7-bit characters into octets. Returns number of octets written (ceil(length * 7 / 8)).
 */
static int SMSPack7BitChars(const char *input, int length, BYTE *output)
{
   int used = 0;
   int shift = 0;
   for (int i = 0; i < length; i++)
   {
      if (shift == 7)
      {
         shift = 0;
         continue;
      }

      BYTE octet = static_cast<BYTE>((input[i] & 0x7F) >> shift);
      if (i < length - 1)
         octet |= static_cast<BYTE>((input[i + 1] & 0x7F) << (7 - shift));
      output[used++] = octet;
      shift++;
   }
   return used;
}

/**
 * Create SMS-SUBMIT PDU (as hex string, with default SMSC) from phone number and message text.
 * Message text is truncated to 160 characters. Returns false if phone number is too long.
 */
bool SMSCreatePDUString(const char *phoneNumber, const char *message, char *pduBuffer, int pduBufferSize)
{
   const char *digits;
   int numberFormat;
   if (phoneNumber[0] == '+')
   {
      digits = &phoneNumber[1];
      numberFormat = 0x91; // International format
   }
   else if (!strncmp(phoneNumber, "00", 2))
   {
      digits = &phoneNumber[2];
      numberFormat = 0x91; // International format
   }
   else
   {
      digits = phoneNumber;
      numberFormat = 0x81; // Unknown number format
   }

   int numDigits = static_cast<int>(strlen(digits));
   if (numDigits > MAX_ADDRESS_DIGITS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Phone number \"%hs\" is too long"), phoneNumber);
      return false;
   }

   // Address is encoded as swapped semi-octets, padded with F if number of digits is odd
   char address[MAX_ADDRESS_DIGITS + 2];
   int i;
   for (i = 0; i < numDigits; i += 2)
   {
      address[i] = (i + 1 < numDigits) ? digits[i + 1] : 'F';
      address[i + 1] = digits[i];
   }
   address[i] = 0;
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Formatted phone: %hs (%d digits)"), address, numDigits);

   int septets = static_cast<int>(strlen(message));
   if (septets > MAX_SMS_SEPTETS)
      septets = MAX_SMS_SEPTETS;

   BYTE payload[(MAX_SMS_SEPTETS * 7 + 7) / 8];
   int payloadSize = SMSPack7BitChars(message, septets, payload);
   nxlog_debug_tag(DEBUG_TAG, 7, _T("%d characters packed into %d octets"), septets, payloadSize);

   char payloadHex[sizeof(payload) * 2 + 1];
   BinToStrA(payload, payloadSize, payloadHex);

   // SMSC length 0, SMS-SUBMIT with relative validity period, message reference 0,
   // destination address, protocol ID 0, 7-bit default alphabet, validity period 4 days
   snprintf(pduBuffer, pduBufferSize, "001100%02X%02X%s0000AA%02X%s", numDigits, numberFormat, address, septets, payloadHex);
   return true;
}
