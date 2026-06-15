/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageDataProvider;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.internal.DPIUtil;
import org.eclipse.swt.widgets.Display;
import org.netxms.ui.svg.SVGImage;

/**
 * Platform-specific image provider tools (SWT desktop version).
 */
public class ImageProviderTools
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

      final ImageData sourceData = imageData;
      return new Image(originalImage.getDevice(), new ImageDataProvider() {
         @Override
         public ImageData getImageData(int zoom)
         {
            int size = requiredBaseSize * zoom / 100;
            return ((sourceData.width == size) && (sourceData.height == size)) ? sourceData : sourceData.scaledTo(size, size);
         }
      });
   }

   /**
    * Rasterize SVG image to an off-screen SWT Image at the given dimensions with transparent background.
    *
    * @param display display to create image on
    * @param svgImage parsed SVG image
    * @param width target width in pixels
    * @param height target height in pixels
    * @return rasterized SWT image or null on failure
    */
   public static Image rasterizeSVG(Display display, SVGImage svgImage, int width, int height)
   {
      return svgImage.rasterize(display, width, height);
   }

   /**
    * Render an image (raster or SVG) directly to a GC at the specified position and size.
    *
    * @param gc target graphics context
    * @param rasterImage raster image (may be null if SVG)
    * @param svgImage parsed SVG image (may be null if raster)
    * @param x target x coordinate
    * @param y target y coordinate
    * @param width target width
    * @param height target height
    */
   public static void renderImage(GC gc, Image rasterImage, SVGImage svgImage, int x, int y, int width, int height)
   {
      if (svgImage != null)
      {
         gc.setAdvanced(true);
         gc.setAntialias(SWT.ON);
         svgImage.render(gc, x, y, width, height);
      }
      else if (rasterImage != null)
      {
         Rectangle bounds = rasterImage.getBounds();
         gc.drawImage(rasterImage, 0, 0, bounds.width, bounds.height, x, y, width, height);
      }
   }
}
