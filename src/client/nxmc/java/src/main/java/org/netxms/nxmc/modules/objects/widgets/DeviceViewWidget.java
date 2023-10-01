/**
 * NetXMS - open source network management system
 * Copyright (C) 2023 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets;

import java.io.ByteArrayInputStream;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.DeviceView;
import org.netxms.client.DeviceViewElement;
import org.netxms.client.DeviceViewImage;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Device view widget
 */
public class DeviceViewWidget extends Canvas
{
   private static final Logger logger = LoggerFactory.getLogger(DeviceViewWidget.class);

   private final I18n i18n = LocalizationHelper.getI18n(DeviceViewWidget.class);

   private View view;
   private long nodeId;
   private List<DeviceViewElement> elements;
   private Map<String, ImageHandle> images = new HashMap<>();
   private ColorCache colors;
   private Runnable sizeChangeListener = null;

   /**
    * Create device view widget.
    *
    * @param parent parent composite
    * @param style style bits
    * @param view owning view
    */
   public DeviceViewWidget(Composite parent, int style, View view)
   {
      super(parent, style);
      this.view = view;
      colors = new ColorCache(this);
      addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
            if (elements != null)
               for(DeviceViewElement element : elements)
                  paintElement(e.gc, element);
         }
      });
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            for(ImageHandle i : images.values())
            {
               if (i.image != null)
                  i.image.dispose();
            }
         }
      });
   }

   /**
    * Paint single element.
    *
    * @param gc graphics context
    * @param e element to paint
    */
   private void paintElement(GC gc, DeviceViewElement element)
   {
      if ((element.flags & DeviceViewElement.BACKGROUND) != 0)
      {
         gc.setBackground(colors.create(ColorConverter.rgbFromInt(element.backgroundColor)));
         gc.fillRectangle(element.x, element.y, element.width, element.height);
      }

      if ((element.flags & DeviceViewElement.BORDER) != 0)
      {
         gc.setForeground(colors.create(ColorConverter.rgbFromInt(element.borderColor)));
         gc.drawRectangle(element.x, element.y, element.width, element.height);
      }

      if (element.imageName != null)
      {
         Image image = getDeviceViewImage(element.imageName);
         if (image != null)
            gc.drawImage(image, 0, 0, image.getImageData().width, image.getImageData().height, element.x, element.y, element.width, element.height);
      }

      if (element.commands != null)
      {
         String[] commands = element.commands.split(";");
         for(String c : commands)
         {
            try
            {
               executeDrawingCommand(gc, c.trim(), element.x, element.y);
            }
            catch(Exception e)
            {
            }
         }
      }
   }

   /**
    * Execute single drawing command. Possible commands: B r,g,b - set background color<br>
    * F r,g,b - set foreground color<br>
    * R x,y,w,h - draw rectangle<br>
    * RF x,y,w,h - draw filled rectangle<br>
    * C x,y,r - draw circle with radius r and center at (x,y)<br>
    * CF x,y,r - draw filled circle with radius r and center at (x,y)<br>
    * L x1,y1,x2,y2 - draw line from (x1,y1) to (x2,y2)<br>
    * P x1,y1,x2,y2,x3,y3,... - draw polygon<br>
    * PF x1,y1,x2,y2,x3,y3,... - draw filled polygon<br>
    * T x1,y1,text - draw text at (x,y)<br>
    * I x,y,name - draw image with given name at (x,y)<br>
    *
    * @param gc graphics context
    * @param command command to execute
    * @param baseX element's base on X axis
    * @param baseY element's base on Y axis
    * @throws Exception on error
    */
   private void executeDrawingCommand(GC gc, String command, int baseX, int baseY) throws Exception
   {
      int[] args;
      boolean fill;
      switch(command.charAt(0))
      {
         case 'B':
            args = extractIntArguments(command.substring(1));
            if (args.length == 3)
               gc.setBackground(colors.create(args[0], args[1], args[2]));
            break;
         case 'C':
            fill = (command.charAt(1) == 'F');
            args = extractIntArguments(command.substring(fill ? 2 : 1));
            if (args.length == 3)
            {
               if (fill)
                  gc.fillOval(args[0] - args[2] / 2 + baseX, args[1] - args[2] / 2 + baseY, args[2], args[2]);
               gc.drawOval(args[0] - args[2] / 2 + baseX, args[1] - args[2] / 2 + baseY, args[2], args[2]);
            }
            break;
         case 'F':
            args = extractIntArguments(command.substring(1));
            if (args.length == 3)
               gc.setForeground(colors.create(args[0], args[1], args[2]));
            break;
         case 'I':
            drawImage(gc, command.substring(1), baseX, baseY);
            break;
         case 'L':
            args = extractIntArguments(command.substring(1));
            if (args.length == 4)
               gc.drawLine(args[0] + baseX, args[1] + baseY, args[2] + baseX, args[3] + baseY);
            break;
         case 'R':
            fill = (command.charAt(1) == 'F');
            args = extractIntArguments(command.substring(fill ? 2 : 1));
            if (args.length == 4)
            {
               if (fill)
                  gc.fillRectangle(args[0] + baseX, args[1] + baseY, args[2], args[3]);
               gc.drawRectangle(args[0] + baseX, args[1] + baseY, args[2], args[3]);
            }
            break;
         case 'T':
            drawText(gc, command.substring(1), baseX, baseY);
            break;
      }
   }

   /**
    * Draw text (execute "T" command).
    *
    * @param gc graphical context
    * @param data command's data
    * @param baseX element's base on X axis
    * @param baseY element's base on Y axis
    */
   private static void drawText(GC gc, String data, int baseX, int baseY)
   {
      String[] parts = data.trim().split(",", 3);
      if (parts.length < 3)
         return;

      int x = Integer.parseInt(parts[0]) + baseX;
      int y = Integer.parseInt(parts[1]) + baseY;
      gc.drawText(parts[2], x, y, true);
   }

   /**
    * Draw image (execute "I" command).
    *
    * @param gc graphical context
    * @param data command's data
    * @param baseX element's base on X axis
    * @param baseY element's base on Y axis
    */
   private void drawImage(GC gc, String data, int baseX, int baseY)
   {
      String[] parts = data.trim().split(",", 3);
      if (parts.length < 3)
         return;
      
      Image image = getDeviceViewImage(parts[2]);
      if (image == null)
         return;

      int x = Integer.parseInt(parts[0]) + baseX;
      int y = Integer.parseInt(parts[1]) + baseY;
      gc.drawImage(image, x, y);
   }

   /**
    * Extract array of integer arguments from command data.
    *
    * @param data command data
    * @return array of integer arguments
    * @throws NumberFormatException if non-integer element encountered
    */
   private static int[] extractIntArguments(String data) throws NumberFormatException
   {
      String[] parts = data.trim().split(",");
      int[] args = new int[parts.length];
      for(int i = 0; i < parts.length; i++)
         args[i] = Integer.parseInt(parts[i].trim());
      return args;
   }

   /**
    * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      if ((elements == null) || elements.isEmpty())
         return super.computeSize(wHint, hHint, changed);
      return sizeFromElementList(elements);
   }

   /**
    * Set node ID.
    *
    * @param nodeId node object ID
    */
   public void setNodeId(long nodeId)
   {
      this.nodeId = nodeId;
      refresh();
   }

   /**
    * Refresh view
    */
   public void refresh()
   {
      if (nodeId != 0)
      {
         final NXCSession session = Registry.getSession();
         Job job = new Job(i18n.tr("Reading device view data"), view) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final DeviceView deviceView = session.getDeviceView(nodeId);
               runInUIThread(() -> {
                  Point oldSize = sizeFromElementList(elements);
                  Point newSize = sizeFromElementList(deviceView.elements);
                  elements = deviceView.elements;

                  for(DeviceViewImage i : deviceView.images)
                  {
                     ImageHandle h = images.get(i.name);
                     if (h == null)
                     {
                        h = new ImageHandle(i);
                        images.put(i.name, h);
                     }
                  }

                  redraw();
                  if ((sizeChangeListener != null) && !oldSize.equals(newSize))
                     sizeChangeListener.run();
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot get device view data");
            }
         };
         job.setUser(false);
         job.start();
      }
      else if (elements != null)
      {
         elements = null;
         if (sizeChangeListener != null)
            sizeChangeListener.run();
         redraw();
      }
   }

   /**
    * Set listener that will be called when device view size is changed
    *
    * @param listener
    */
   public void setSizeChangeListener(Runnable listener)
   {
      sizeChangeListener = listener;
   }

   /**
    * Get widget size from element list.
    *
    * @param elements element list
    * @return widget size (size of first element or 0x0)
    */
   private static Point sizeFromElementList(List<DeviceViewElement> elements)
   {
      if ((elements == null) || elements.isEmpty())
         return new Point(0, 0);
      DeviceViewElement e = elements.get(0);
      return new Point(e.width, e.height);
   }

   /**
    * Get device view image by name.
    *
    * @param name image name
    * @return ready to use image or null
    */
   private Image getDeviceViewImage(String name)
   {
      ImageHandle h = images.get(name);
      if (h == null)
         return null;
      if ((h.image == null) && !h.failed)
      {
         try
         {
            h.image = new Image(getDisplay(), new ByteArrayInputStream(h.imageData));
         }
         catch(Exception e)
         {
            logger.debug("Cannot create image \"" + name + "\" from binary data", e);
            h.failed = true;
         }
      }
      return h.image;
   }

   /**
    * Device view image handle
    */
   private static class ImageHandle
   {
      byte[] imageData;
      Image image;
      boolean failed;

      ImageHandle(DeviceViewImage i)
      {
         imageData = i.data;
         image = null;
         failed = false;
      }
   }
}
