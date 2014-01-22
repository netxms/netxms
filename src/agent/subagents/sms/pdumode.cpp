/* 
** NetXMS SMS sending subagent
** Copyright (C) 2003-2014 Raden Solutions
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

#include "sms.h"

/**
 * Pack 7-bit characters into 8-bit characters
 */
static bool SMSPack7BitChars(const char* input, char* output, int* outputLength, const int maxOutputLen)
{
	int	bits = 0;
	int i;
	unsigned char octet;
	int used = 0;
	
   int inputLength = (int)strlen(input);
   if (inputLength > 160)
      inputLength = 160;

	for (i = 0; i < inputLength; i++)
	{
		if (bits == 7)
		{
			bits = 0;
			continue;
		}

		if (used == maxOutputLen)
			return false;

		octet = (input[i] & 0x7f) >> bits;

		if (i < inputLength - 1)
			octet |= input[i + 1] << (7 - bits);

		*output++ = (char)octet;
		used++;
		bits++;
	}

	*outputLength = used;

	return true;
}

/**
 * Create SMS PDU from phone number and message text
 */
bool SMSCreatePDUString(const char* phoneNumber, const char* message, char* pduBuffer)
{
	const int bufferSize = 512;
	char phoneNumberFormatted[32];
	char payload[bufferSize];
	char payloadHex[bufferSize*2 + 1];
	int payloadSize = 0;
	int phoneLength = (int)strlen(phoneNumber);
	int numberFormat = 0x91; // International format
	int i;

	if (phoneNumber[0] == '+')
		strncpy(phoneNumberFormatted, &phoneNumber[1], 32);
	else if (!strncmp(phoneNumber, "00", 2))
		strncpy(phoneNumberFormatted, &phoneNumber[2], 32);
	else
	{
		strncpy(phoneNumberFormatted, phoneNumber, 32);
		numberFormat = 0x81;	// Unknown number format
	}
	strcat(phoneNumberFormatted, "F");

	AgentWriteDebugLog(7, _T("SMSCreatePDUString: Formatted phone before: %hs,%d"), phoneNumberFormatted, phoneLength);
	for (i = 0; i <= phoneLength; i += 2)
	{
		char tmp = phoneNumberFormatted[i+1];
		phoneNumberFormatted[i+1] = phoneNumberFormatted[i];
		phoneNumberFormatted[i] = tmp;
	}
	phoneNumberFormatted[phoneLength + (phoneLength % 2)] = '\0';
	AgentWriteDebugLog(7, _T("SMSCreatePDUString: Formatted phone: %hs"), phoneNumberFormatted);
	SMSPack7BitChars(message, payload, &payloadSize, bufferSize);
	AgentWriteDebugLog(7, _T("SMSCreatePDUString: Got payload size: %d"), payloadSize);

	for (i = 0; i < payloadSize; i++)
	{
		payloadHex[i*2]		= bin2hex((((unsigned char)(payload[i]))&0xF0)>>4);
		payloadHex[i*2 + 1]	= bin2hex(payload[i]&0xF);
	}
	payloadHex[i*2] = '\0';
	snprintf(pduBuffer, PDU_BUFFER_SIZE, "0011000%X%X%s0000AA%02X%s", (int)strlen(phoneNumber), numberFormat, 
		phoneNumberFormatted, (int)strlen(message), payloadHex);

	return true;
}
