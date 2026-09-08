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
package org.netxms.nxmc.modules.imagelibrary.widgets;

import java.io.ByteArrayInputStream;
import org.eclipse.swt.SWT;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.LibraryImage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.modules.imagelibrary.ImageProviderTools;
import org.netxms.nxmc.tools.FontTools;
import org.netxms.nxmc.tools.WidgetHelper;
import org.netxms.ui.svg.SVGImage;
import org.netxms.ui.svg.SVGParseException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Image preview widget for image library
 */
public class ImagePreview extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(ImagePreview.class);

   private static final Logger logger = LoggerFactory.getLogger(ImagePreview.class);

   private static final String[] HEADER_FONTS = { "Verdana", "DejaVu Sans", "Liberation Sans", "Arial" };

   private LibraryImage imageDescriptor;
   private Image image;
   private SVGImage svgImage;
   private Label imageName;
   private Label imageSize;
   private Canvas imagePreview;
   private Font headerFont;

   /**
    * @param parent
    * @param style
    */
   public ImagePreview(Composite parent, int style)
   {
      super(parent, style);

      headerFont = FontTools.createFont(HEADER_FONTS, +3, SWT.BOLD);

      setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      GridLayout layout = new GridLayout();
      setLayout(layout);

      imageName = new Label(this, SWT.NONE);
      imageName.setBackground(getBackground());
      imageName.setText(i18n.tr("No image selected"));
      imageName.setFont(headerFont);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      imageName.setLayoutData(gd);

      imageSize = new Label(this, SWT.NONE);
      imageSize.setBackground(getBackground());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      imageSize.setLayoutData(gd);

      imagePreview = new Canvas(this, SWT.DOUBLE_BUFFERED);
      imagePreview.setBackground(getBackground());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      imagePreview.setLayoutData(gd);
      imagePreview.addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
            drawImagePreview(e.gc);
         }
      });

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            headerFont.dispose();
            if (image != null)
               image.dispose();
         }
      });
   }

   /**
    * Set new image to display
    *
    * @param imageDescriptor new image to display
    */
   public void setImage(LibraryImage imageDescriptor)
   {
      if ((imageDescriptor == null) && (this.imageDescriptor == null))
         return;

      this.imageDescriptor = imageDescriptor;
      imageName.setText((imageDescriptor != null) ? imageDescriptor.getName() : "No image selected");

      if (image != null)
         image.dispose();
      image = null;
      svgImage = null;

      if (imageDescriptor != null)
      {
         if (imageDescriptor.isSVG())
         {
            if (imageDescriptor.getBinaryData() != null)
            {
               try
               {
                  svgImage = SVGImage.createFromStream(new ByteArrayInputStream(imageDescriptor.getBinaryData()));
               }
               catch(SVGParseException e)
               {
                  logger.error("Cannot parse SVG image", e);
               }
            }

            if (svgImage != null)
            {
               if ((svgImage.getWidth() > 0) && (svgImage.getHeight() > 0))
                  imageSize.setText(String.format("SVG (%.0f x %.0f)", svgImage.getWidth(), svgImage.getHeight()));
               else
                  imageSize.setText("SVG");
            }
            else
            {
               imageSize.setText("");
            }
         }
         else
         {
            image = createRasterImageFromDescriptor(getDisplay(), imageDescriptor);
            if (image != null)
            {
               Rectangle rect = image.getBounds();
               imageSize.setText(String.format("%d x %d", rect.width, rect.height));
            }
            else
            {
               imageSize.setText("");
            }
         }
      }
      else
      {
         imageSize.setText("");
      }

      imagePreview.redraw();
   }

   /**
    * @param gc
    */
   private void drawImagePreview(GC gc)
   {
      Rectangle rect = imagePreview.getClientArea();
      rect.width -= 2;
      rect.height -= 2;
      rect.x++;
      rect.y++;

      if (svgImage != null)
      {
         // SVG: render directly to GC at canvas size, preserving aspect ratio
         float aspect = svgImage.getAspectRatio();
         int drawWidth = rect.width;
         int drawHeight = rect.height;
         if (aspect > 0)
         {
            if ((float)rect.width / rect.height > aspect)
               drawWidth = (int)(rect.height * aspect);
            else
               drawHeight = (int)(rect.width / aspect);
         }
         ImageProviderTools.renderImage(gc, null, svgImage, rect.x, rect.y, drawWidth, drawHeight);
      }
      else if (image != null)
      {
         Rectangle imageBounds = image.getBounds();
         if ((imageBounds.width > rect.width) || (imageBounds.height > rect.height))
         {
            try
            {
               gc.setAntialias(SWT.ON);
               WidgetHelper.setHighInterpolation(gc);
            }
            catch(SWTException e)
            {
               // Ignore SWT exception and continue with current GC settings
            }

            float scaleFactorW = (float)imageBounds.width / (float)rect.width;
            float scaleFactorH = (float)imageBounds.height / (float)rect.height;
            float scaleFactor = Math.max(scaleFactorW, scaleFactorH);
            gc.drawImage(image, 0, 0, imageBounds.width, imageBounds.height, 0, 0,
                  (int)((float)imageBounds.width / scaleFactor), (int)((float)imageBounds.height / scaleFactor));
         }
         else
         {
            gc.drawImage(image, 0, 0);
         }
      }
      else
      {
         if (rect.height > rect.width)
            rect.height = rect.width;
         else
            rect.width = rect.height;
         gc.drawRectangle(rect);
         gc.drawLine(0, 0, rect.width, rect.height);
         gc.drawLine(rect.width, 0, 0, rect.height);
      }
   }

   /**
    * Create a raster SWT Image from library descriptor (non-SVG images only).
    *
    * @param display target display
    * @param imageDescriptor library image descriptor
    * @return SWT Image or null
    */
   private static Image createRasterImageFromDescriptor(Display display, LibraryImage imageDescriptor)
   {
      if ((imageDescriptor == null) || imageDescriptor.isSVG())
         return null;

      if (imageDescriptor.getBinaryData() != null)
      {
         try
         {
            return new Image(display, new ByteArrayInputStream(imageDescriptor.getBinaryData()));
         }
         catch(Exception e)
         {
            logger.error("Cannot create image from library descriptor", e);
            return null;
         }
      }

      Image image = ImageProvider.getInstance().getImage(imageDescriptor.getGuid());
      return (image != null) ? new Image(display, image, SWT.IMAGE_COPY) : null;
   }

   /**
    * Create image from library descriptor. For raster images, returns the raster image directly.
    * For SVG images, rasterizes at a default size (256x256 or viewBox dimensions).
    *
    * @param display target display
    * @param imageDescriptor library image descriptor
    * @return SWT Image or null
    */
   public static Image createImageFromDescriptor(Display display, LibraryImage imageDescriptor)
   {
      if (imageDescriptor == null)
         return null;

      if (imageDescriptor.isSVG())
      {
         if (imageDescriptor.getBinaryData() == null)
            return null;

         try
         {
            SVGImage svg = SVGImage.createFromStream(new ByteArrayInputStream(imageDescriptor.getBinaryData()));
            int width = (svg.getWidth() > 0) ? (int)svg.getWidth() : 256;
            int height = (svg.getHeight() > 0) ? (int)svg.getHeight() : 256;
            if ((width > 1024) || (height > 1024))
            {
               float scale = 1024.0f / Math.max(width, height);
               width = Math.max(1, Math.round(width * scale));
               height = Math.max(1, Math.round(height * scale));
            }
            return ImageProviderTools.rasterizeSVG(display, svg, width, height);
         }
         catch(SVGParseException e)
         {
            logger.error("Cannot parse SVG image for rasterization", e);
            return null;
         }
      }

      return createRasterImageFromDescriptor(display, imageDescriptor);
   }
}
