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

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;

/**
 * Image provider tools
 */
class ImageProviderTools
{
   /**
    * Create resized image based on original image and required size.
    *
    * @param originalImage original image
    * @param requiredBaseSize base required size
    * @return resized image (will return original image if it match required size)
    */
   public static Image createResizedImage(Image originalImage, int requiredBaseSize)
   {
      ImageData imageData = originalImage.getImageData();
      if ((imageData.width == requiredBaseSize) && (imageData.height == requiredBaseSize))
         return originalImage;
      return new Image(originalImage.getDevice(), imageData.scaledTo(requiredBaseSize, requiredBaseSize));
   }
}
