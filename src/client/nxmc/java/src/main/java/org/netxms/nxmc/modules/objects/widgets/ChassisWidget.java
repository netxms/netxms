/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2021 Raden Solutions
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
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.NXCommon;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.configs.ChassisPlacement;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.modules.imagelibrary.ImageUpdateListener;
import org.netxms.nxmc.modules.objects.widgets.helpers.ElementSelectionListener;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.FontTools;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Chassis display widget
 */
public class ChassisWidget extends Canvas implements PaintListener, DisposeListener, ImageUpdateListener, MouseListener, MouseTrackListener
{   
   private static final int FULL_UNIT_WIDTH = 482;
   private static final int FULL_UNIT_HEIGHT = 45;
   private static final int MARGIN_HEIGHT = 10;
   private static final int MARGIN_WIDTH = 10;
   private static final int TITLE_HEIGHT = 20;
   private static final int STATUS_INDECATOR_HEIGHT = 3;
   private static final int STATUS_INDECATOR_WIDTH = 8;
   private static final int STATUS_INDECATOR_SHIFT = 2;
   private static final int STATUS_INDECATOR_BORDER = 1;
   private static final String[] FONT_NAMES = { "Segoe UI", "Liberation Sans", "DejaVu Sans", "Verdana", "Arial" };
   private static final String[] VIEW_LABELS = { "Front", "Back" };
   
   private Chassis chassis;
   private Font[] titleFonts;
   private List<ObjectImage> objects = new ArrayList<ObjectImage>();
   private Object selectedObject = null;
   private Set<ElementSelectionListener> selectionListeners = new HashSet<ElementSelectionListener>(0);
   private Object tooltipObject = null;
   private ObjectPopupDialog tooltipDialog = null;
   private RackOrientation view;
   
   /**
    * @param parent
    * @param style
    */
   public ChassisWidget(Composite parent, int style, Chassis chassis, RackOrientation view, boolean separateView)
   {
      super(parent, style | SWT.DOUBLE_BUFFERED);
      this.chassis = chassis;
      this.view = (view == RackOrientation.FILL) ? RackOrientation.FRONT : view;

      setBackground(ThemeEngine.getBackgroundColor("Rack"));

      titleFonts = FontTools.getFonts(FONT_NAMES, 6, SWT.BOLD, 16);

      addPaintListener(this);
      addMouseListener(this);
      WidgetHelper.attachMouseTrackListener(this, this);
      addDisposeListener(this);
      ImageProvider.getInstance().addUpdateListener(this);
   }

   /**
    * @return the currentObject
    */
   public Object getCurrentObject()
   {
      return selectedObject;
   }

   /**
    * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
    */
   @Override
   public void paintControl(PaintEvent e)
   {      
      final GC gc = e.gc;

      gc.setAntialias(SWT.ON);
      WidgetHelper.setHighInterpolation(gc);

      // Calculate bounding box for chassis picture
      Rectangle rect = getClientArea();
      rect.x += MARGIN_WIDTH; 
      rect.y += MARGIN_HEIGHT + TITLE_HEIGHT;
      rect.height -= MARGIN_HEIGHT * 2 + TITLE_HEIGHT;
      rect.width -= MARGIN_WIDTH * 2;

      if (rect.height <= 0 || rect.width <= 0)
         return;

      // Title
      gc.setFont(WidgetHelper.getBestFittingFont(gc, titleFonts, VIEW_LABELS[0], rect.width, TITLE_HEIGHT)); //$NON-NLS-1$
      Point titleSize = gc.textExtent(VIEW_LABELS[view.getValue() - 1]);
      gc.drawText(VIEW_LABELS[view.getValue() - 1], (rect.width / 2 - titleSize.x / 2) + MARGIN_WIDTH, rect.y - TITLE_HEIGHT - MARGIN_HEIGHT / 2);

      drawChassis(gc, rect, chassis, view, new ObjectRegistration() {
         @Override
         public void addObject(Object object, Rectangle rect)
         {
            objects.add(new ObjectImage(object, rect));
         }
      });
   }

   /**
    * @param gc
    * @param rect
    */
   public static void drawChassis(final GC gc, Rectangle rect, Chassis chassis, RackOrientation view, ObjectRegistration objectRegistration)
   {   
      Image image = (view == RackOrientation.REAR) ? RackWidget.getImageDefaultRear() : RackWidget.getImageDefaultTop();
      UUID imageGuid = (view == RackOrientation.FRONT) ? chassis.getFrontRackImage() : chassis.getRearRackImage();
      if (!imageGuid.equals(NXCommon.EMPTY_GUID))
         image = ImageProvider.getInstance().getImage(imageGuid);

      Rectangle r = image.getBounds();
      if(chassis.getRackHeight() == 1 || !imageGuid.equals(NXCommon.EMPTY_GUID))
      {
         gc.drawImage(image, 0, 0, r.width, r.height, rect.x, rect.y, rect.width, rect.height);
      }
      else 
      {
         Image imageMiddle = (view == RackOrientation.REAR) ? RackWidget.getImageDefaultRear() : RackWidget.getImageDefaultMiddle();
         Image imageBottom = (view == RackOrientation.REAR) ? RackWidget.getImageDefaultRear() : RackWidget.getImageDefaultBottom();
         int oneUnitHeight = rect.height/chassis.getRackHeight();

         gc.drawImage(image, 0, 0, r.width, r.height, rect.x, rect.y, rect.width, oneUnitHeight);
         r = imageMiddle.getBounds();
         for (int i = 1; i < (chassis.getRackHeight() - 1); i++)
            gc.drawImage(imageMiddle, 0, 0, r.width, r.height, rect.x, rect.y + oneUnitHeight * i, rect.width, oneUnitHeight); 
         
         r = imageBottom.getBounds();
         gc.drawImage(imageBottom, 0, 0, r.width, r.height, rect.x, rect.y + oneUnitHeight * (chassis.getRackHeight() - 1), rect.width, oneUnitHeight);                 
      }

      //Calculate inch to pixel proportion
      double proportion = rect.height / (double)(FULL_UNIT_HEIGHT * chassis.getRackHeight());
      final Color gray = gc.getDevice().getSystemColor(SWT.COLOR_GRAY);
      final Color black = gc.getDevice().getSystemColor(SWT.COLOR_BLACK);
      
      //draw chassis nodes
      for (AbstractObject object : chassis.getChildrenAsArray())
      {
         if(object instanceof Node)
         {           
            ChassisPlacement placemet = ((Node)object).getChassisPlacement();
            if (placemet.getOritentaiton() != view.getValue())           
               continue;

            final Rectangle unitRect = new Rectangle(rect.x + (int)(placemet.getPositionWidthInMm() * proportion),
                  rect.y + (int)(placemet.getPositionHeightInMm() * proportion), (int)(placemet.getWidthInMm() * proportion), 
                  (int)(placemet.getHeightInMm() * proportion));

            
            if ((unitRect.width <= 0) || (unitRect.height <= 0))
               continue;

            if(objectRegistration != null)
               objectRegistration.addObject(object, unitRect);
            
            boolean imageMissing = true;
            imageGuid = placemet.getImage();
            if (!imageGuid.equals(NXCommon.EMPTY_GUID))
            {
               image = null;
               image = ImageProvider.getInstance().getImage(imageGuid);
               if(image != null)
               {
                  r = image.getBounds();
                  gc.drawImage(image, 0, 0, r.width, r.height, unitRect.x, unitRect.y, unitRect.width, unitRect.height);   
                  imageMissing = false;
               }
            }
            if (imageMissing)
            {
               gc.setBackground(gray);
               gc.fillRectangle(unitRect);
            }

            gc.setBackground(black);
            gc.fillRectangle(unitRect.x + STATUS_INDECATOR_SHIFT, unitRect.y + STATUS_INDECATOR_SHIFT, (int)(STATUS_INDECATOR_WIDTH * proportion + STATUS_INDECATOR_BORDER * 2), (int)(STATUS_INDECATOR_HEIGHT * proportion + STATUS_INDECATOR_BORDER * 2));      
            
            gc.setBackground(StatusDisplayInfo.getStatusColor(object.getStatus()));
            gc.fillRectangle(unitRect.x + STATUS_INDECATOR_SHIFT + STATUS_INDECATOR_BORDER, unitRect.y + STATUS_INDECATOR_SHIFT + STATUS_INDECATOR_BORDER, (int)(STATUS_INDECATOR_WIDTH * proportion), (int)(STATUS_INDECATOR_HEIGHT * proportion));            
         }
      }
   }
   

   
   /* (non-Javadoc)
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      if (hHint == SWT.DEFAULT && wHint == SWT.DEFAULT)
         return new Point(10, 10);

      int titleSize = TITLE_HEIGHT + MARGIN_HEIGHT * 2; 
      hHint /= 2;
      hHint -= titleSize;
      wHint -= MARGIN_WIDTH * 2;
      
      int realHeight = FULL_UNIT_HEIGHT * chassis.getRackHeight();
      if((double)FULL_UNIT_WIDTH/realHeight >= (double)wHint/hHint)
      {
         //calculate by width
         hHint = wHint * realHeight/FULL_UNIT_WIDTH;
      }
      else
      {
         //clculate by height
         wHint = hHint * FULL_UNIT_WIDTH/realHeight;
      } 
      
      return new Point(wHint + MARGIN_WIDTH * 2, hHint + titleSize);
   }

   /**
    * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
    */
   @Override
   public void widgetDisposed(DisposeEvent e)
   {
      removePaintListener(this);
      ImageProvider.getInstance().removeUpdateListener(this);
   }

   /**
    * @see org.netxms.nxmc.modules.imagelibrary.ImageUpdateListener#imageUpdated(java.util.UUID)
    */
   @Override
   public void imageUpdated(UUID guid)
   {
      boolean found = false;
      for(AbstractObject obj : chassis.getChildrenAsArray())
      {
         if (guid.equals(((Node)obj).getChassisPlacement().getImage()))
         {
            found = true;
            break;
         }
      }
      if (found)
      {
         redraw();
      }
   }

   /**
    * Get object at given point
    * 
    * @param p
    * @return
    */
   private Object getObjectAtPoint(Point p)
   {
      for(ObjectImage i : objects)
         if (i.contains(p))
         {
            return i.getObject();
         }
      return null;
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
      if ((tooltipDialog != null) && (tooltipDialog.getShell() != null) && !tooltipDialog.getShell().isDisposed() && (e.display.getActiveShell() != tooltipDialog.getShell()))
      {
         tooltipDialog.close();
         tooltipDialog = null;
      }
      tooltipObject = null;
      setCurrentObject(getObjectAtPoint(new Point(e.x, e.y)));
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.MouseListener#mouseUp(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseUp(MouseEvent e)
   {
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.swt.events.MouseTrackListener#mouseEnter(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseEnter(MouseEvent e)
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.MouseTrackListener#mouseExit(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseExit(MouseEvent e)
   {
      if ((tooltipDialog != null) && (tooltipDialog.getShell() != null) && !tooltipDialog.getShell().isDisposed() && (e.display.getActiveShell() != tooltipDialog.getShell()))
      {
         tooltipDialog.close();
         tooltipDialog = null;
      }
      tooltipObject = null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.MouseTrackListener#mouseHover(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseHover(MouseEvent e)
   {
      Object object = getObjectAtPoint(new Point(e.x, e.y));
      if ((object != null) && ((object != tooltipObject) || (tooltipDialog == null) || (tooltipDialog.getShell() == null) || tooltipDialog.getShell().isDisposed()))
      {
         if ((tooltipDialog != null) && (tooltipDialog.getShell() != null) && !tooltipDialog.getShell().isDisposed())
            tooltipDialog.close();
      
         tooltipObject = object;
         tooltipDialog = new ObjectPopupDialog(getShell(), object, toDisplay(e.x, e.y));
         tooltipDialog.open();
      }
      else if ((object == null) && (tooltipDialog != null) && (tooltipDialog.getShell() != null) && !tooltipDialog.getShell().isDisposed())
      {
         tooltipDialog.close();
         tooltipDialog = null;
      }
   }

   /**
    * Add selection listener
    * 
    * @param listener
    */
   public void addSelectionListener(ElementSelectionListener listener)
   {
      selectionListeners.add(listener);
   }
   
   /**
    * Remove selection listener
    * 
    * @param listener
    */
   public void removeSelectionListener(ElementSelectionListener listener)
   {
      selectionListeners.remove(listener);
   }
   
   /**
    * Set current selection
    * 
    * @param o
    */
   private void setCurrentObject(Object o)
   {
      selectedObject = o;
      for(ElementSelectionListener l : selectionListeners)
         l.objectSelected(selectedObject);
   }
   
   /**
    * Object image information
    */
   private class ObjectImage
   {
      private Object object;
      private Rectangle rect;

      public ObjectImage(Object object, Rectangle rect)
      {
         this.object = object;
         this.rect = new Rectangle(rect.x, rect.y, rect.width, rect.height);
      }
      
      public boolean contains(Point p)
      {
         return rect.contains(p);
      }
      
      public Object getObject()
      {
         return object;
      }
   }
   
   public interface ObjectRegistration
   {
      void addObject(Object object, Rectangle rect);
   }
}
