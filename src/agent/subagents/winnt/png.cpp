/*
** Windows platform subagent
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: main.cpp
**
**/

#include "winnt_subagent.h"
#include <png.h>

/**
 * Save given bitmap as PNG file
 */
bool SaveBitmapToPng(HBITMAP hBitmap, const TCHAR *fileName)
{
   BITMAP bitmap;
   GetObject(hBitmap, sizeof(bitmap), (LPSTR)&bitmap);
   
   DWORD scanlineSize = ((bitmap.bmWidth * 4) + (4 - 1)) & ~(4 - 1);
   DWORD bufferSize = scanlineSize * bitmap.bmHeight;
   BYTE *buffer = (BYTE *)malloc(bufferSize);
   if (buffer == NULL)
      return false;

   HDC hDC = GetDC(NULL);

   BITMAPINFO bitmapInfo;
   memset(&bitmapInfo, 0, sizeof(bitmapInfo));
   bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
   bitmapInfo.bmiHeader.biWidth = bitmap.bmWidth;
   bitmapInfo.bmiHeader.biHeight = bitmap.bmHeight;
   bitmapInfo.bmiHeader.biPlanes = 1;
   bitmapInfo.bmiHeader.biBitCount = 32;
   bitmapInfo.bmiHeader.biCompression = BI_RGB;
   bitmapInfo.bmiHeader.biClrUsed = 0;
   bitmapInfo.bmiHeader.biClrImportant = 0;
   if (!GetDIBits(hDC, hBitmap, 0, bitmap.bmHeight, buffer, &bitmapInfo, DIB_RGB_COLORS))
   {
      ReleaseDC(NULL, hDC);
      free(buffer);
      return false;
   }

   const int width = bitmap.bmWidth;
   const int height = bitmap.bmHeight;
   const int depth = 8;
   const int bytesPerPixel = sizeof(BYTE) * 4;

   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;
   png_byte **row_pointers = NULL;
   bool success = false;

   FILE *fp = _tfopen(fileName, _T("wb"));
   if (fp == NULL)
      goto fopen_failed;

   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (png_ptr == NULL)
      goto png_create_write_struct_failed;

   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
      goto png_create_info_struct_failed;

   if (setjmp(png_jmpbuf(png_ptr)))
      goto png_failure;

   png_set_IHDR(png_ptr,
      info_ptr,
      width,
      height,
      depth,
      PNG_COLOR_TYPE_RGB_ALPHA,
      PNG_INTERLACE_NONE,
      PNG_COMPRESSION_TYPE_DEFAULT,
      PNG_FILTER_TYPE_DEFAULT);
   row_pointers = (png_byte **)png_malloc(png_ptr, height * sizeof (png_byte *));
   for(int y = 0; y < height; ++y)
   {
      png_byte *row = (png_byte *)&buffer[y * bitmap.bmWidth * 4];
      row_pointers[height - y - 1] = row;
      
      // Convert RGBA to BGRA
      for(int i = 0; i < width * 4; i += 4)
      {
         png_byte r = row[i];
         row[i] = row[i + 2];
         row[i + 2] = r;
      }
   }
   png_init_io(png_ptr, fp);
   png_set_rows(png_ptr, info_ptr, row_pointers);
   png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

   success = true;

   png_free(png_ptr, row_pointers);
png_failure:
png_create_info_struct_failed:
   png_destroy_write_struct (&png_ptr, &info_ptr);
png_create_write_struct_failed:
   fclose (fp);
fopen_failed:
   free(buffer);
   ReleaseDC(NULL, hDC);
   return success;
}
