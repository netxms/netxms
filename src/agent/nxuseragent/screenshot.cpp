/*
** NetXMS User Support Agent
** Copyright (C) 2009-2024 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: screenshot.cpp
**/

#include "nxuseragent.h"
#include <png.h>

/**
 * Write PNG data
 */
static void WriteData(png_structp png_ptr, png_bytep data, png_size_t length)
{
   static_cast<ByteStream*>(png_get_io_ptr(png_ptr))->write(data, length);
}

/**
 * Flush write buffer
 */
static void FlushBuffer(png_structp png_ptr)
{
}

/**
 * Save given bitmap as PNG
 */
ByteStream *SaveBitmapToPng(HBITMAP hBitmap)
{
   BITMAP bitmap;
   GetObject(hBitmap, sizeof(bitmap), (LPSTR)&bitmap);

   DWORD scanlineSize = ((bitmap.bmWidth * 4) + (4 - 1)) & ~(4 - 1);
   DWORD bufferSize = scanlineSize * bitmap.bmHeight;
   BYTE *buffer = static_cast<BYTE*>(MemAlloc(bufferSize));
   if (buffer == nullptr)
      return nullptr;

   HDC hDC = GetDC(nullptr);

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
      ReleaseDC(nullptr, hDC);
      MemFree(buffer);
      return nullptr;
   }

   const int width = bitmap.bmWidth;
   const int height = bitmap.bmHeight;
   const int depth = 8;
   const int bytesPerPixel = 4;

   png_structp png_ptr = nullptr;
   png_infop info_ptr = nullptr;
   ByteStream *pngData = nullptr;

   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
   if (png_ptr == nullptr)
      goto png_create_write_struct_failed;

   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == nullptr)
      goto png_create_info_struct_failed;

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      delete_and_null(pngData);
      goto png_failure;
   }

   png_set_IHDR(png_ptr,
      info_ptr,
      width,
      height,
      depth,
      PNG_COLOR_TYPE_RGB,
      PNG_INTERLACE_NONE,
      PNG_COMPRESSION_TYPE_DEFAULT,
      PNG_FILTER_TYPE_DEFAULT);
   png_byte **row_pointers = (png_byte **)alloca(height * sizeof(png_byte *));
   for (int y = 0; y < height; ++y)
   {
      png_byte *row = (png_byte *)&buffer[y * bitmap.bmWidth * 4];
      row_pointers[height - y - 1] = row;

      // Convert RGBA to BGR
      for (int i = 0, j = 0; i < width * 4; i++)
      {
         png_byte r = row[i++];
         png_byte g = row[i++];
         png_byte b = row[i++];
         row[j++] = b;
         row[j++] = g;
         row[j++] = r;
      }
   }

   pngData = new ByteStream(bufferSize + 4096); // Use uncompresed bitmap size + 4K as buffer size
   pngData->setAllocationStep(65536);
   png_set_write_fn(png_ptr, pngData, WriteData, FlushBuffer);
   png_set_rows(png_ptr, info_ptr, row_pointers);
   png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

png_failure:
png_create_info_struct_failed:
   png_destroy_write_struct(&png_ptr, &info_ptr);
png_create_write_struct_failed:
   ReleaseDC(nullptr, hDC);
   MemFree(buffer);
   return pngData;
}

/**
 * Take screenshot
 */
void TakeScreenshot(NXCPMessage *response)
{
   uint32_t rcc = ERR_INTERNAL_ERROR;

   HDC dc = CreateDC(_T("DISPLAY"), nullptr, nullptr, nullptr);
   if (dc != nullptr)
   {
      HDC memdc = CreateCompatibleDC(dc);
      if (memdc != nullptr)
      {
         int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
         int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
         int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
         int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

         HBITMAP bitmap = CreateCompatibleBitmap(dc, cx, cy);
         if (bitmap != nullptr)
         {
            SelectObject(memdc, bitmap);
            BitBlt(memdc, 0, 0, cx, cy, dc, x, y, SRCCOPY | CAPTUREBLT);
         }

         DeleteDC(memdc);

         ByteStream *png = SaveBitmapToPng(bitmap);
         if (png != nullptr)
         {
            rcc = ERR_SUCCESS;
            response->setField(VID_FILE_DATA, png->buffer(), png->size());
            delete png;
         }
         DeleteObject(bitmap);
      }
      DeleteDC(dc);
   }

   response->setField(VID_RCC, rcc);
}
