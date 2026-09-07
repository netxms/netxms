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
import org.netxms.ui.svg.ScaleMode;

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
    * Create scaled copy of raster image using GC with high quality interpolation.
    *
    * @param source source image (not disposed)
    * @param width target width in pixels
    * @param height target height in pixels
    * @return new scaled image
    */
   public static Image scaleImage(Image source, int width, int height)
   {
      Rectangle bounds = source.getBounds();
      Image scaled = new Image(source.getDevice(), width, height);
      GC gc = new GC(scaled);
      gc.setInterpolation(SWT.HIGH);
      gc.drawImage(source, 0, 0, bounds.width, bounds.height, 0, 0, width, height);
      gc.dispose();
      return scaled;
   }

   /**
    * Rasterize SVG image to an off-screen SWT Image at the given base dimensions with transparent background. The image is
    * re-rasterized from vector data for each device zoom level SWT asks for, so it stays sharp on HiDPI displays.
    *
    * @param display display to create image on
    * @param svgImage parsed SVG image
    * @param width target width in pixels at 100% zoom
    * @param height target height in pixels at 100% zoom
    * @return rasterized SWT image
    */
   public static Image rasterizeSVG(Display display, SVGImage svgImage, int width, int height)
   {
      return new Image(display, new ImageDataProvider() {
         @Override
         public ImageData getImageData(int zoom)
         {
            return svgImage.rasterizeToImageData(Math.max(1, width * zoom / 100), Math.max(1, height * zoom / 100), null, ScaleMode.UNIFORM);
         }
      });
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
