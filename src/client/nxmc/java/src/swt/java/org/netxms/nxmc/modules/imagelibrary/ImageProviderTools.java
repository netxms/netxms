/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.modules.imagelibrary;

import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.internal.DPIUtil;

/**
 * Image provider tools
 */
class ImageProviderTools
{
   /**
    * Create resized image based on original image and required size. Required size will be scaled to current device zoom level.
    *
    * @param originalImage original image
    * @param requiredBaseSize base required size
    * @return resized image (will return original image if it match required size)
    */
   public static Image createResizedImage(Image originalImage, int requiredBaseSize)
   {
      ImageData imageData = originalImage.getImageData();
      int zoom = DPIUtil.getDeviceZoom();
      int requiredSize = requiredBaseSize * zoom / 100;
      if ((imageData.width == requiredSize) && (imageData.height == requiredSize))
         return originalImage;

      if (imageData.width != imageData.height)
      {
         int size = Math.min(imageData.width, imageData.height);
         Image trimmedImage = new Image(originalImage.getDevice(), size, size);
         GC gc = new GC(trimmedImage);
         gc.drawImage(originalImage, 0, 0);
         gc.dispose();
         imageData = trimmedImage.getImageData();
         trimmedImage.dispose();
      }

      return new Image(originalImage.getDevice(), new DPIUtil.AutoScaleImageDataProvider(originalImage.getDevice(), imageData, imageData.width * 100 / requiredSize));
   }
}
