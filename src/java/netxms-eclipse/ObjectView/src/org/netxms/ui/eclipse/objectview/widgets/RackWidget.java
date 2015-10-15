/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.widgets;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.NXCommon;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageUpdateListener;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.widgets.helpers.RackSelectionListener;
import org.netxms.ui.eclipse.tools.FontTools;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Rack display widget
 */
public class RackWidget extends Canvas implements PaintListener, DisposeListener, ImageUpdateListener, MouseListener
{
   private static final double UNIT_WH_RATIO = 10.85;
   private static final int BORDER_WIDTH_RATIO = 15;
   private static final int FULL_UNIT_WIDTH = 482;
   private static final int FULL_UNIT_HEIGHT = 45;
   private static final int MARGIN_HEIGHT = 10;
   private static final int MARGIN_WIDTH = 10;
   private static final int UNIT_NUMBER_WIDTH = 30;
   private static final String[] FONT_NAMES = { "Segoe UI", "Liberation Sans", "Verdana", "Arial" };
   
   private Rack rack;
   private Font[] labelFonts;
   private Image imageDefaultTop;
   private Image imageDefaultMiddle;
   private Image imageDefaultBottom;
   private List<ObjectImage> objects = new ArrayList<ObjectImage>();
   private AbstractObject currentObject = null;
   private Set<RackSelectionListener> selectionListeners = new HashSet<RackSelectionListener>(0);
   
   /**
    * @param parent
    * @param style
    */
   public RackWidget(Composite parent, int style, Rack rack)
   {
      super(parent, style | SWT.DOUBLE_BUFFERED);
      this.rack = rack;
      
      setBackground(SharedColors.getColor(SharedColors.RACK_BACKGROUND, getDisplay()));
      
      final String fontName = FontTools.findFirstAvailableFont(FONT_NAMES);
      labelFonts = new Font[16];
      for(int i = 0; i < labelFonts.length; i++)
         labelFonts[i] = new Font(getDisplay(), fontName, i + 6, SWT.NORMAL);
      
      imageDefaultTop = Activator.getImageDescriptor("icons/rack-default-top.png").createImage();
      imageDefaultMiddle = Activator.getImageDescriptor("icons/rack-default-middle.png").createImage();
      imageDefaultBottom = Activator.getImageDescriptor("icons/rack-default-bottom.png").createImage();
      
      addPaintListener(this);
      addMouseListener(this);
      addDisposeListener(this);
      ImageProvider.getInstance().addUpdateListener(this);
   }

   /**
    * @return the currentObject
    */
   public AbstractObject getCurrentObject()
   {
      return currentObject;
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
    */
   @Override
   public void paintControl(PaintEvent e)
   {
      final GC gc = e.gc;
      
      gc.setAntialias(SWT.ON);
      gc.setInterpolation(SWT.HIGH);
      
      // Calculate bounding box for rack picture
      Rectangle rect = getClientArea();
      rect.x += MARGIN_WIDTH + UNIT_NUMBER_WIDTH;
      rect.y += MARGIN_HEIGHT;
      rect.height -= MARGIN_HEIGHT * 2;
      
      // Estimated unit width/height and calculate border width
      double unitHeight = (double)rect.height / (double)rack.getHeight();
      int unitWidth = (int)(unitHeight * UNIT_WH_RATIO);
      int borderWidth = unitWidth / BORDER_WIDTH_RATIO;
      if (borderWidth < 3)
         borderWidth = 3;
      rect.height -= borderWidth;

      // precise unit width and height taking borders into consideration
      unitHeight = (double)(rect.height - ((borderWidth + 1) / 2) * 2) / (double)rack.getHeight();
      unitWidth = (int)(unitHeight * UNIT_WH_RATIO);
      rect.width = unitWidth + borderWidth * 2;
      
      // Rack itself
      gc.setBackground(SharedColors.getColor(SharedColors.RACK_EMPTY_SPACE, getDisplay()));
      gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 3, 3);
      gc.setLineWidth(borderWidth);
      gc.setForeground(SharedColors.getColor(SharedColors.RACK_BORDER, getDisplay()));
      gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 3, 3);
      
      // Rack bottom
      gc.setBackground(SharedColors.getColor(SharedColors.RACK_BORDER, getDisplay()));
      gc.fillRectangle(rect.x + borderWidth * 2 - (borderWidth + 1) / 2, rect.y + rect.height, borderWidth * 2, (int)(borderWidth * 1.5));
      gc.fillRectangle(rect.x + rect.width - borderWidth * 3 - (borderWidth + 1) / 2, rect.y + rect.height, borderWidth * 2, (int)(borderWidth * 1.5));

      // Draw unit numbers
      int[] unitBaselines = new int[rack.getHeight() + 1];
      gc.setFont(WidgetHelper.getBestFittingFont(gc, labelFonts, "00", UNIT_NUMBER_WIDTH, (int)unitHeight - 2));
      gc.setForeground(SharedColors.getColor(SharedColors.RACK_TEXT, getDisplay()));
      gc.setBackground(SharedColors.getColor(SharedColors.RACK_BACKGROUND, getDisplay()));
      gc.setLineWidth(1);
      double dy = rect.y + rect.height - (borderWidth + 1) / 2;
      for(int u = 1; u <= rack.getHeight(); u++, dy -= unitHeight)
      {
         int y = (int)dy;
         unitBaselines[u - 1] = y;
         gc.drawLine(MARGIN_WIDTH, y, UNIT_NUMBER_WIDTH, y);
         String label = Integer.toString(u);
         Point textExtent = gc.textExtent(label);
         gc.drawText(label, UNIT_NUMBER_WIDTH - textExtent.x, y - (int)unitHeight / 2 - textExtent.y / 2);
      }
      unitBaselines[rack.getHeight()] = (int)dy;
      
      // Draw units
      objects.clear();
      List<AbstractNode> units = rack.getUnits();
      for(AbstractNode n : units)
      {
         if ((n.getRackPosition() > rack.getHeight()) || (n.getRackPosition() - n.getRackHeight() < 0))
            continue;
         
         int bottomLine = unitBaselines[n.getRackPosition() - n.getRackHeight()]; // lower border
         int topLine = unitBaselines[n.getRackPosition()];   // upper border
         final Rectangle unitRect = new Rectangle(rect.x + (borderWidth + 1) / 2, topLine + 1, rect.width - borderWidth, bottomLine - topLine);

         if ((unitRect.width <= 0) || (unitRect.height <= 0))
            break;
         
         objects.add(new ObjectImage(n, unitRect));

         // draw status indicator
         gc.setBackground(StatusDisplayInfo.getStatusColor(n.getStatus()));
         gc.fillRectangle(unitRect.x - borderWidth + borderWidth / 4 + 1, unitRect.y + 1, borderWidth / 2 - 1, Math.min(borderWidth, (int)unitHeight - 2));
         
         if ((n.getRackImage() != null) && !n.getRackImage().equals(NXCommon.EMPTY_GUID))
         {
            Image image = ImageProvider.getInstance().getImage(n.getRackImage());
            Rectangle r = image.getBounds();
            gc.drawImage(image, 0, 0, r.width, r.height, unitRect.x, unitRect.y, unitRect.width, unitRect.height);
         }
         else // Draw default representation
         {
            Rectangle r = imageDefaultTop.getBounds();
            if (n.getRackHeight() == 1)
            {
               gc.drawImage(imageDefaultTop, 0, 0, r.width, r.height, unitRect.x, unitRect.y, unitRect.width, unitRect.height);
            }
            else
            {
               unitRect.height = unitBaselines[n.getRackPosition() - 1] - topLine;
               gc.drawImage(imageDefaultTop, 0, 0, r.width, r.height, unitRect.x, unitRect.y, unitRect.width, unitRect.height);

               r = imageDefaultMiddle.getBounds();
               int u = n.getRackPosition() - 1;
               for(int i = 1; i < n.getRackHeight() - 1; i++, u--)
               {
                  unitRect.y = unitBaselines[u];
                  unitRect.height = unitBaselines[u - 1] - unitRect.y;
                  gc.drawImage(imageDefaultMiddle, 0, 0, r.width, r.height, unitRect.x, unitRect.y, unitRect.width, unitRect.height);
               }
               
               r = imageDefaultBottom.getBounds();
               unitRect.y = unitBaselines[u];
               unitRect.height = unitBaselines[u - 1] - unitRect.y;
               gc.drawImage(imageDefaultBottom, 0, 0, r.width, r.height, unitRect.x, unitRect.y, unitRect.width, unitRect.height);
            }
         }
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      if (hHint == SWT.DEFAULT)
      {
         int borderWidth = FULL_UNIT_WIDTH / BORDER_WIDTH_RATIO;
         return new Point(FULL_UNIT_WIDTH + MARGIN_WIDTH * 2 + UNIT_NUMBER_WIDTH + borderWidth * 2, rack.getHeight() * FULL_UNIT_HEIGHT + MARGIN_HEIGHT * 2 + borderWidth * 2);
      }
      
      double unitHeight = (double)hHint / (double)rack.getHeight();
      int unitWidth = (int)(unitHeight * UNIT_WH_RATIO);
      int borderWidth = unitWidth / BORDER_WIDTH_RATIO;
      if (borderWidth < 3)
         borderWidth = 3;
      
      unitWidth = (int)((double)(hHint - ((borderWidth + 1) / 2) * 2 - MARGIN_HEIGHT * 2) / (double)rack.getHeight() * UNIT_WH_RATIO);
      return new Point(unitWidth + MARGIN_WIDTH * 2 + UNIT_NUMBER_WIDTH + borderWidth * 2, hHint);
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
    */
   @Override
   public void widgetDisposed(DisposeEvent e)
   {
      for(int i = 0; i < labelFonts.length; i++)
         labelFonts[i].dispose();
      
      imageDefaultTop.dispose();
      imageDefaultMiddle.dispose();
      imageDefaultBottom.dispose();
      
      ImageProvider.getInstance().removeUpdateListener(this);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.imagelibrary.shared.ImageUpdateListener#imageUpdated(java.util.UUID)
    */
   @Override
   public void imageUpdated(UUID guid)
   {
      boolean found = false;
      List<AbstractNode> units = rack.getUnits();
      for(AbstractNode n : units)
      {
         if (guid.equals(n.getRackImage()))
         {
            found = true;
            break;
         }
      }
      if (found)
      {
         getDisplay().asyncExec(new Runnable() {
            @Override
            public void run()
            {
               redraw();
            }
         });
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.MouseListener#mouseDoubleClick(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseDoubleClick(MouseEvent e)
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.MouseListener#mouseDown(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseDown(MouseEvent e)
   {  
      AbstractObject o = null;
      Point p = new Point(e.x, e.y);
      for(ObjectImage i : objects)
         if (i.contains(p))
         {
            o = i.getObject();
            break;
         }
      setCurrentObject(o);
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.MouseListener#mouseUp(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseUp(MouseEvent e)
   {
   }
   
   /**
    * Add selection listener
    * 
    * @param listener
    */
   public void addSelectionListener(RackSelectionListener listener)
   {
      selectionListeners.add(listener);
   }
   
   /**
    * Remove selection listener
    * 
    * @param listener
    */
   public void removeSelectionListener(RackSelectionListener listener)
   {
      selectionListeners.remove(listener);
   }
   
   /**
    * Set current selection
    * 
    * @param o
    */
   private void setCurrentObject(AbstractObject o)
   {
      currentObject = o;
      for(RackSelectionListener l : selectionListeners)
         l.objectSelected(currentObject);
   }
   
   /**
    * Object image information
    */
   private class ObjectImage
   {
      private AbstractObject object;
      private Rectangle rect;

      public ObjectImage(AbstractObject object, Rectangle rect)
      {
         this.object = object;
         this.rect = new Rectangle(rect.x, rect.y, rect.width, rect.height);
      }
      
      public boolean contains(Point p)
      {
         return rect.contains(p);
      }
      
      public AbstractObject getObject()
      {
         return object;
      }
   }
}
