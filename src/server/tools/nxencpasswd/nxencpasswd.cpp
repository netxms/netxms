/* 
** nxencpasswd - command line tool for encrypting passwords using NetXMS server key
** Copyright (C) 2004-2009 Victor Kirhenshtein
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
** File: nxencpasswd.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>


//
// main
//

int main(int argc, char *argv[])
{
	int ch;

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "hK:v")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxencpasswd [<options>] <login> <password>\n"
                   "Valid options are:\n"
                   "   -h           : Display help and exit.\n"
                   "   -v           : Display version and exit.\n"
                   "\n");
				return 0;
         case 'v':   // Print version and exit
            printf("NetXMS Password Encryption Tool Version " NETXMS_VERSION_STRING_A "\n");
				return 0;
         case '?':
				return 1;
         default:
            break;
      }
   }

	if (argc - optind < 2)
	{
		fprintf(stderr, "Required arguments missing. Run nxencpasswd -h for help.\n");
		return 1;
	}

	// Encrypt password
	BYTE plainText[32], encrypted[32], key[16];
	memset(plainText, 0, 32);
	memset(encrypted, 0, 32);
	memset(key, 0, 16);

	CalculateMD5Hash((BYTE *)argv[optind], strlen(argv[optind]), key);
	strncpy((char *)plainText, argv[optind + 1], 31);
	ICEEncryptData(plainText, 32, encrypted, key);

	// Convert encrypted password to base64 encoding and print
	char textForm[128];
	base64_encode((char *)encrypted, 32, textForm, 128);
	puts(textForm);

	return 0;
}
