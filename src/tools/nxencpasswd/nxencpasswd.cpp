/* 
** nxencpasswd - command line tool for encrypting passwords using NetXMS server key
** Copyright (C) 2004-2013 Victor Kirhenshtein
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

   bool isAgentSecret = false;
   bool decrypt = false;

   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "ahvz")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxencpasswd [<options>] <login> <password>\n"
                   "       nxencpasswd [<options>] -a <password>\n"
                   "Valid options are:\n"
                   "   -a           : Encrypt agent's secret.\n"
                   "   -h           : Display help and exit.\n"
                   "   -v           : Display version and exit.\n"
                   "\n");
				return 0;
         case 'v':   // Print version and exit
            printf("NetXMS Password Encryption Tool Version " NETXMS_VERSION_STRING_A "\n");
				return 0;
         case 'a':   // Obfuscate agent's secret
            isAgentSecret = true;
            break;
         case 'z':   // Decrypt
            decrypt = true;
            break;
         case '?':
				return 1;
         default:
            break;
      }
   }

	if ((!isAgentSecret && (argc - optind < 2)) || (isAgentSecret && (argc - optind < 1)))
	{
		fprintf(stderr, "Required arguments missing. Run nxencpasswd -h for help.\n");
		return 1;
	}

   if (decrypt)
   {
#if UNICODE
      TCHAR login[128], password[256];
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, argv[optind], -1, login, 128);
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, argv[optind + 1], -1, password, 256);

      TCHAR decrypted[256] = {0};
#else
      char *login = argv[optind];
      char *password = argv[optind + 1];
      char decrypted[256] = {0};
#endif
      DecryptPassword(login, password, decrypted, 256);
      _tprintf(_T("Decrypted: \"%s\"\n"), decrypted);
      return 0;
   }

	// Encrypt password
	BYTE plainText[32], encrypted[32], key[16];
	memset(plainText, 0, 32);
	memset(encrypted, 0, 32);
	memset(key, 0, 16);

   char *login;
   if (isAgentSecret)
   {
      login = (char *)"netxms";
   }
   else
   {
      login = argv[optind];
   }
	CalculateMD5Hash((BYTE *)login, strlen(login), key);
	strncpy((char *)plainText, isAgentSecret ? argv[optind] : argv[optind + 1], 31);
	ICEEncryptData(plainText, 32, encrypted, key);

	// Convert encrypted password to base64 encoding and print
	char textForm[128];
	base64_encode((char *)encrypted, 32, textForm, 128);
	puts(textForm);

	return 0;
}
