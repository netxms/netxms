/* 
** nxgenguid - command line tool for GUID generation
** Copyright (C) 2004-2015 Victor Kirhenshtein
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
** File: nxcsum.cpp
**
**/

#include <nms_util.h>
#include <nxstat.h>

NETXMS_EXECUTABLE_HEADER(nxcsum)

#define MAX_READ_SIZE 1024

union HashState
{
	MD5_STATE md5;
   SHA1_STATE sha1;
   SHA256_STATE sha256;
};

static bool PrintHash(const char *fileName, size_t digestSize, void (*init)(HashState*), void (*update)(HashState*, BYTE*, size_t), void (*finish)(HashState*, BYTE*))
{   
	HashState state;
   BYTE buffer[MAX_READ_SIZE];
	BYTE hash[MD5_DIGEST_SIZE];

	init(&state);
   int hFile = _open(fileName, O_RDONLY | O_BINARY);
   if (hFile == -1)
      return false;

   while(true)
   {
      int readSize = _read(hFile, buffer, MAX_READ_SIZE);
      if (readSize <= 0)
         break;
	   update(&state, buffer, readSize);
   }
	finish(&state, hash);

   char dump[digestSize * 2 + 1];
   printf("%s", strlwr(BinToStrA(hash, digestSize, dump)));
   return true;
}

#define HashInit(t) t##Init
#define HashUpdate(t) t##Update
#define HashFinish(t) t##Finish
#define HashSize(t) t##_DIGEST_SIZE

#define CallPrintHash(f, t) \
   PrintHash(f, HashSize(t), (void (*)(HashState*))HashInit(t), (void (*)(HashState*, BYTE*, size_t))HashUpdate(t), (void (*)(HashState*, BYTE*))HashFinish(t));

/**
 * main
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   if((argc < 2))
   {
      printf("Invalid count of arguments\n"
             "Usage: nxcsum <method> <file>\n"
             "Valid methods are: md5, sha1, sha256\n"
             "\n");
      return 1;
   }

   bool success;
   if (!strcmp(argv[1], "md5"))
   {
      success = CallPrintHash(argv[2], MD5);
   }
   else if(!strcmp(argv[1], "sha1"))
   {
      success = CallPrintHash(argv[2], SHA1);
   }
   else if(!strcmp(argv[1], "sha256"))
   {
      success = CallPrintHash(argv[2], SHA256);
   }
   else
   {
      printf("Invalid method\n"
             "Valid methods are: md5, sha1, sha256\n"
             "\n");
      return 1;
   }

   return success ? 0 : 2;
}

