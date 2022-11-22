/* 
** nxcsum - command line tool for checksum calculation
** Copyright (C) 2018-2022 Raden Solutions
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
#include <nxcrypto.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxcsum)

#define MAX_READ_SIZE 4096

union HashState
{
   MD5_STATE md5;
   SHA1_STATE sha1;
   SHA224_STATE sha224;
   SHA256_STATE sha256;
   SHA384_STATE sha384;
   SHA512_STATE sha512;
};

static bool PrintHash(const char *fileName, size_t digestSize, void (*init)(HashState*), void (*update)(HashState*, BYTE*, size_t), void (*finish)(HashState*, BYTE*))
{  
   bool useStdin = (strcmp(fileName, "-") == 0);

#ifdef _WIN32
   int hFile;
   if (useStdin)
   {
      hFile = _fileno(stdin);
      _setmode(hFile, _O_BINARY);
   }
   else
   {
      hFile = _sopen(fileName, O_RDONLY | O_BINARY, SH_DENYWR);
   }
#else
   int hFile = useStdin ? fileno(stdin) : _open(fileName, O_RDONLY | O_BINARY);
#endif
   if (hFile == -1)
   {
      _ftprintf(stderr, _T("Cannot open input file (%hs)\n"), strerror(errno));
      return false;
   }

   BYTE buffer[MAX_READ_SIZE];

   HashState state;
   init(&state);

   while(true)
   {
      int readSize = _read(hFile, buffer, MAX_READ_SIZE);
      if (readSize <= 0)
         break;
      update(&state, buffer, readSize);
   }

   if (!useStdin)
      _close(hFile);

   BYTE *hash = (BYTE *)MemAlloc(digestSize);
   finish(&state, hash);

   TCHAR *dump = MemAllocArray<TCHAR>(digestSize * 2 + 1);
   _tprintf(_T("%s"), _tcslwr(BinToStr(hash, digestSize, dump)));
   MemFree(dump);
   MemFree(hash);
   return true;
}

#define HashInit(t) t##Init
#define HashUpdate(t) t##Update
#define HashFinal(t) t##Final
#define HashSize(t) t##_DIGEST_SIZE

#define CallPrintHash(f, t) \
   PrintHash(f, HashSize(t), (void (*)(HashState*))HashInit(t), (void (*)(HashState*, BYTE*, size_t))HashUpdate(t), (void (*)(HashState*, BYTE*))HashFinal(t));

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
             "Valid methods are: md5, sha1, sha224, sha256, sha384, sha512\n"
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
   else if(!strcmp(argv[1], "sha224"))
   {
      success = CallPrintHash(argv[2], SHA224);
   }
   else if(!strcmp(argv[1], "sha256"))
   {
      success = CallPrintHash(argv[2], SHA256);
   }
   else if(!strcmp(argv[1], "sha384"))
   {
      success = CallPrintHash(argv[2], SHA384);
   }
   else if(!strcmp(argv[1], "sha512"))
   {
      success = CallPrintHash(argv[2], SHA512);
   }
   else
   {
      printf("Invalid method\n"
             "Valid methods are: md5, sha1, sha224, sha256, sha384, sha512\n"
             "\n");
      return 1;
   }

   return success ? 0 : 2;
}
