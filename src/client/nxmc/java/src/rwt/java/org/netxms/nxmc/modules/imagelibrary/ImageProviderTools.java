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

import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.netxms.ui.svg.SVGImage;

/**
 * Platform-specific image provider tools (RWT web version).
 */
public class ImageProviderTools
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

   /**
    * Create scaled copy of raster image. RWT GC cannot draw onto an image, so scaling is done on image data.
    *
    * @param source source image (not disposed)
    * @param width target width in pixels
    * @param height target height in pixels
    * @return new scaled image
    */
   public static Image scaleImage(Image source, int width, int height)
   {
      return new Image(source.getDevice(), source.getImageData().scaledTo(width, height));
   }

   /**
    * Rasterize SVG image to an off-screen image at the given dimensions with transparent background. Rasterization is done
    * through Java2D inside the SVG renderer, so it does not depend on RWT GC capabilities.
    *
    * @param display display to create image on
    * @param svgImage parsed SVG image
    * @param width target width in pixels
    * @param height target height in pixels
    * @return rasterized image
    */
   public static Image rasterizeSVG(Display display, SVGImage svgImage, int width, int height)
   {
      return svgImage.rasterize(display, width, height);
   }

   /**
    * Render an image (raster or SVG) directly to a GC at the specified position and size.
    * On RWT, SVG is rendered via GC operations that translate to browser-side canvas drawing.
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
         svgImage.render(gc, x, y, width, height);
      }
      else if (rasterImage != null)
      {
         Rectangle bounds = rasterImage.getBounds();
         gc.drawImage(rasterImage, 0, 0, bounds.width, bounds.height, x, y, width, height);
      }
   }
}
