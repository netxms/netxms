/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Transform;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.FontTools;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Widget representing object status as radial ("sunburst") diagram
 */
public class ObjectStatusSunburstDiagram extends Canvas implements PaintListener
{
	private AbstractObject rootObject;
	private int maxLvl;
	private int leafObjectCount;
	private float leafObjectSize;   // angular size of single leaf object
	private int diameter;
	private int centerX;
	private int centerY;
	private ColorCache cCache;
	private Map<Long, ObjectData> objects;
   protected Image chartImage = null; 
   private List<Integer> fontSize;
   private static final String[] FONT_NAMES = { "Segoe UI", "Liberation Sans", "DejaVu Sans", "Verdana", "Arial" };
   private Font[] valueFonts;
   private static final int FONT_BASE_SIZE = 7;
   private static final int PADDING_HORIZONTAL = 6;
   private static final int PADDING_VERTICAL = 6;
   private static final int MARGIN = 6;
   private static final int SHIFT = 3;
   private boolean fitToScreen = true;
   private boolean needRender = false;
	private List<ObjectPosition> objectMap = new ArrayList<ObjectPosition>();

	/**
	 * @param parent
	 * @param aceptedlist 
	 */
	public ObjectStatusSunburstDiagram(Composite parent, AbstractObject rootObject, Collection<AbstractObject> objects)
	{
		super(parent, SWT.FILL);
		this.rootObject = rootObject;    
      cCache = new ColorCache(this);
      valueFonts = FontTools.getFonts(FONT_NAMES, FONT_BASE_SIZE, SWT.BOLD, 16);

		this.objects = new HashMap<Long, ObjectData>();
		if (objects != null)
		{
   		for(AbstractObject o : objects)
   		   this.objects.put(o.getObjectId(), new ObjectData(o));
		}

      setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
      addPaintListener(this);
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            removePaintListener(ObjectStatusSunburstDiagram.this);
            if (chartImage != null)
               chartImage.dispose();
         }
      });
      addControlListener(new ControlListener() {
         @Override
         public void controlResized(ControlEvent e)
         {
            refresh();
         }

         @Override
         public void controlMoved(ControlEvent e)
         {
         }
      });
	}

   /**
    * Refresh objects
    */
   public void refresh()
   {
      if (rootObject == null)
         return;
      needRender = true;
      redraw();
   }

   /**
    * Render chart
    */
   private void render()
   {
      if (chartImage != null)
      {
         chartImage.dispose();
         chartImage = null;
      }

      Point size = getSize();
      if ((size.x <= 0) || (size.y <= 0))
         return;
      chartImage = new Image(getDisplay(), size.x, size.y);

      GC gc = new GC(chartImage);
      paint(gc);
      gc.dispose();
   }
	
	/**
	 * Draw sectors for all objects
	 * 
	 * @param object the root object
	 */
	private float drawParts(GC gc, AbstractObject object, int lvl, float degree)
	{	   
      gc.setAlpha(255);

      float objectSize = 0;
	   for(AbstractObject obj : object.getChildrenAsArray())
	   {	      
	      float currObjsize = 0;	      
	      if (AbstractObjectStatusMap.isContainerObject(obj))
	      {
	         currObjsize = drawParts(gc, obj, lvl + 1, degree); 
	         if (currObjsize == 0)
	         {
	            if (objects != null)
	               continue;
	            currObjsize = leafObjectSize;
	         }
	      }
	      else
	      {
	         if (objects != null && !objects.containsKey(obj.getObjectId()))            
	            continue;
	         
            currObjsize = leafObjectSize;
	      }

         // compensate for rounding errors by either extending or shrinking current object by one degree
         int compensation = Integer.signum(Math.round(degree + currObjsize) - (Math.round(degree) + Math.round(currObjsize)));

         // white circle before each level
         if ((int)degree == 0 && (int)currObjsize == 360)
         {
            gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
            gc.fillOval(centerX - (diameter * lvl) / 2 - SHIFT, centerY - (diameter * lvl) / 2 - SHIFT, diameter * lvl + MARGIN, diameter * lvl + MARGIN);

            gc.setBackground(StatusDisplayInfo.getStatusColor(obj.getStatus()));
            gc.fillOval(centerX - (diameter * lvl) / 2, centerY - (diameter * lvl) / 2, diameter * lvl, diameter * lvl);
         }
         else
         {
            gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
            gc.fillArc(centerX - (diameter * lvl) / 2 - SHIFT, centerY - (diameter * lvl) / 2 - SHIFT, diameter * lvl + MARGIN, diameter * lvl + MARGIN, Math.round(degree),
                  Math.round(currObjsize) + compensation);

            gc.setBackground(StatusDisplayInfo.getStatusColor(obj.getStatus()));
            gc.fillArc(centerX - (diameter * lvl) / 2, centerY - (diameter * lvl) / 2, diameter * lvl, diameter * lvl, Math.round(degree), Math.round(currObjsize) - 1 + compensation);
         }

         objectMap.add(new ObjectPosition(degree, degree + currObjsize, lvl, obj));

	      Transform oldTransform = new Transform(gc.getDevice());  
         gc.getTransform(oldTransform);

         // draw text
         String text = obj.getNameWithAlias();

         Transform tr = new Transform(getDisplay());
         tr.translate(centerX, centerY);

         // text centering
         float rotate = 0;
         float middle = degree + currObjsize / 2;
         if (middle >= 90 && middle < 180)
            rotate = 90 - (middle - 90);
         if (middle >= 270 && middle <= 360)
            rotate = 90 - (middle - 270);
         if (middle >= 0 && middle < 90)
            rotate = -middle;
         if (middle > 180 && middle < 270)
            rotate = -(middle - 180);

         tr.rotate(rotate);
         gc.setTransform(tr);

         // cut text function
         gc.setFont(valueFonts[fontSize.get(lvl - 1)]);
         int l = gc.textExtent(text).x;
         int h = gc.textExtent(text).y;
         // height of sector
         int lineNum;
         if (currObjsize < 360)
         {
            int sectorHeight = (int)(Math.tan(Math.toRadians(currObjsize / 2)) * (diameter / 2 * (lvl - 1)) * 2) - PADDING_VERTICAL * 2;
            lineNum = sectorHeight / h;
            if (lineNum < 0 || lineNum > 3)
               lineNum = 3;
         }
         else
         {
            lineNum = 3;
         }
         text = WidgetHelper.fitStringToArea(gc, text, diameter / 2 - PADDING_HORIZONTAL * 2, lineNum).getResult();
         h = gc.textExtent(text).y;

         gc.setForeground(ColorConverter.selectTextColorByBackgroundColor(StatusDisplayInfo.getStatusColor(obj.getStatus()), cCache));

         if (middle >= 90 && middle <= 180 || middle > 180 && middle < 270)
         {
            l = gc.textExtent(text).x;
            gc.drawText(text, -(diameter * (lvl - 1)) / 2 - PADDING_HORIZONTAL - l - SHIFT, -h / 2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);
         }
         else
         {
            gc.drawText(text, (diameter * (lvl - 1)) / 2 + PADDING_HORIZONTAL + SHIFT, -h / 2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);
         }

         gc.setTransform(oldTransform);    
         tr.dispose();
         oldTransform.dispose();

         degree += currObjsize;
         objectSize += currObjsize;
	   }   
      return objectSize;
	}

	/**
    * Max level and object count calculation
    * 
    * @param object
    * @param level
    * @return object count
    */
   private int calculateMaxLVLAndObjCount(AbstractObject object, int level)
   {     
      int objectCount = 0;
      int foundObjectsCount = 0;
      AbstractObject[] objSet = object.getChildrenAsArray();
      for(AbstractObject obj : objSet)
      {
         if (objects != null)            
         {
            if (AbstractObjectStatusMap.isContainerObject(obj))
            {
               int tmp = calculateMaxLVLAndObjCount(obj, level + 1);    
               if(tmp > 0)
               {
                  foundObjectsCount++;
                  objectCount+=tmp;
               }
            }
            else
            {
               if (objects.containsKey(obj.getObjectId()))
                  objectCount++;
            }
         }
         else
         {
            if (AbstractObjectStatusMap.isContainerObject(obj))
            {
               objectCount += calculateMaxLVLAndObjCount(obj, level+1);    
               foundObjectsCount++;
            }
            else
               objectCount++;
         }
      }

      if ((objects == null) && AbstractObjectStatusMap.isContainerObject(object) && (objectCount == 0))            
      {
         objectCount++;
      }

      if (foundObjectsCount == 0)
      {
         if (maxLvl < level)
            maxLvl = level;
      }

      return objectCount;
   }

   /**
    * Calculate optimal font size
    * 
    * @param object
    * @param level
    * @param gc
    * @return object count inside of current object
    */
   private int calculateLayerSize(AbstractObject object, int level, GC gc)
   {
      int objCount = 0;
      AbstractObject[] objSet = object.getChildrenAsArray();
      gc.setFont(valueFonts[0]);// set minimal font
      for(AbstractObject obj : objSet)
      {
         int contSize;
         if (AbstractObjectStatusMap.isContainerObject(obj))
         {
            contSize = calculateLayerSize(obj, level + 1, gc);
            objCount += contSize;
         }
         else
         {
            contSize = 1;
            objCount++;
         }

         calculateOptimalDiameter(obj, level, contSize, gc);

      }
      if (objSet.length == 0)
         objCount++;

      return objCount;
   }

   /**
    * Calculates optimal diameter to display objects with fully seen name
    * 
    * @param obj
    * @param level
    * @param objCount
    * @param gc
    */
   private void calculateOptimalDiameter(AbstractObject obj, int level, int objCount, GC gc)
   {
      String text = obj.getNameWithAlias();
      int sectorHeight = (int)((Math.tan(Math.toRadians(leafObjectSize * objCount / 2))) * ((diameter * level) / 2) * 2) - PADDING_VERTICAL * 2;
      if (sectorHeight <= 0)
         sectorHeight = 1286;

      while(!WidgetHelper.fitStringToRect(gc, text, diameter / 2 - PADDING_HORIZONTAL * 2, sectorHeight, 3))
      {
         diameter = (int)(diameter + diameter * 0.01);
         sectorHeight = (int)((Math.tan(Math.toRadians(leafObjectSize * objCount / 2))) * ((diameter * level) / 2) * 2) - PADDING_VERTICAL * 2;
         if (sectorHeight < 0)
            sectorHeight = 1286;
      }
   }

   /**
    * Calculate optimal font size and sets it to the global list fontSize
    * 
    * @param object
    * @param level
    * @param gc
    * @return object count inside of current object
    */
   private int calculateLayerFontSize(AbstractObject object, int level, GC gc)
   {
      int objCount = 0;
      AbstractObject[] objSet = object.getChildrenAsArray();
      for(AbstractObject obj : objSet)
      {
         int contSize;
         if (AbstractObjectStatusMap.isContainerObject(obj))
         {
            contSize = calculateLayerFontSize(obj, level + 1, gc);
            objCount += contSize;
         }
         else
         {
            contSize = 1;
            objCount++;
         }

         Integer optimalFontSie = calculateOptimalFontsie(obj, level, contSize, gc);
         if (fontSize.get(level) > optimalFontSie)
            fontSize.set(level, optimalFontSie);

      }
      if (objSet.length == 0)
         objCount++;

      return objCount;
   }

   /**
    * Calcualtes optimal font size for each separate object 
    * 
    * @param obj
    * @param lvl
    * @param objCount
    * @param gc
    * @return ordinal number of the font in the array
    */
   private Integer calculateOptimalFontsie(AbstractObject obj, int lvl, int objCount, GC gc)
   {
      String text = obj.getNameWithAlias();
      int sectorHeight = (int)((Math.tan(Math.toRadians(leafObjectSize * objCount / 2))) * ((diameter * lvl) / 2) * 2) - PADDING_VERTICAL * 2;
      if (sectorHeight < 0)
         sectorHeight = 1286;
      return WidgetHelper.getBestFittingFontMultiline(gc, valueFonts, text, diameter / 2 - PADDING_HORIZONTAL * 2, sectorHeight, 3); // $NON-NLS-1$
   }

   /**
    * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
    */
   @Override
   public void paintControl(PaintEvent e)
   {
      if (needRender)
      {
         render();
         needRender = false;
      }
      if (chartImage != null)
         e.gc.drawImage(chartImage, 0, 0);
   }
   
   /**
    * Makes all required recalculations in case of object change or view resize
    * 
    * @param gc
    */
   public void recalculateData(GC gc)
   {
      leafObjectCount = calculateMaxLVLAndObjCount(rootObject, 1);
      diameter = 200;
      leafObjectSize = 360 / (float)leafObjectCount;
      if (!fitToScreen)
         calculateLayerSize(rootObject, 1, gc);
   }

	/**
	 * Draw radial map
	 * 
	 * @param gc
	 */
	public void paint(GC gc)
	{
      gc.setAdvanced(true);
      if (!gc.getAdvanced()) 
      {
        gc.drawText("Advanced graphics not supported", 30, 30, true);
        return;
      } 

      gc.setAntialias(SWT.ON);
      gc.setTextAntialias(SWT.ON);
      gc.setForeground(ThemeEngine.getForegroundColor("StatusMap.Text"));
      gc.setLineWidth(1);

	   objectMap.clear();
	   recalculateData(gc);

		Rectangle rect = getClientArea();
		rect.width--;
		rect.height--;
		//calculate values
      int rectSide = Math.min(rect.width, rect.height);
      diameter = Math.max(diameter, rectSide / (maxLvl + 1));
      centerX = rect.x+(rectSide/2);
      centerY = rect.y+(rectSide/2);

      // calculate optimal font size
      fontSize = new ArrayList<Integer>(Collections.nCopies(maxLvl + 1, Integer.valueOf(100)));
      // font calculation for general container
      final int squareSide = (int)(diameter / Math.sqrt(2));
      fontSize.set(0, Integer.valueOf(WidgetHelper.getBestFittingFontMultiline(gc, valueFonts, rootObject.getNameWithAlias(), squareSide, squareSide, 3)));
      // font calculation for sectors
      calculateLayerFontSize(rootObject, 1, gc);
      Integer prevLVL = 100;
      for(int i = 0; i < fontSize.size(); i++)
      {
         if (prevLVL < fontSize.get(i))
         {
            fontSize.set(i, prevLVL);
         }
         prevLVL = fontSize.get(i);
      }

      // draw objects
      drawParts(gc, rootObject, 2, 0);

      // draw general oval with
      // draw white oval
      gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
      gc.setAlpha(255);
      gc.fillOval(centerX - diameter / 2 - SHIFT, centerY - diameter / 2 - SHIFT, diameter + MARGIN, diameter + MARGIN);

      gc.setBackground(StatusDisplayInfo.getStatusColor(rootObject.getStatus()));
      gc.setAlpha(255);
      gc.fillOval(centerX - diameter / 2, centerY - diameter / 2, diameter, diameter);

		String text = (rootObject instanceof AbstractNode) ?
            (rootObject.getNameWithAlias() + "\n" + ((AbstractNode)rootObject).getPrimaryIP().getHostAddress()) : //$NON-NLS-1$
            rootObject.getNameWithAlias();

      gc.setFont(valueFonts[fontSize.get(0)]);

      gc.setClipping(rect);
      int h = gc.textExtent(text).y;
      int l = gc.textExtent(text).x;
      if (l + PADDING_HORIZONTAL >= diameter)
      {
         text = WidgetHelper.fitStringToArea(gc, text, squareSide, squareSide / h).getResult();
         h = gc.textExtent(text).y;
         l = gc.textExtent(text).x;
      }
      gc.setForeground(ColorConverter.selectTextColorByBackgroundColor(StatusDisplayInfo.getStatusColor(rootObject.getStatus()), cCache));
      gc.drawText(text, centerX - l / 2, centerY - h / 2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);

      objectMap.add(new ObjectPosition(0, 360, 1, rootObject));
	}

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      if (rootObject == null)
         return super.computeSize(wHint, hHint, changed);

      GC gc = new GC(getDisplay());
      fitToScreen = !(wHint == SWT.DEFAULT && hHint == SWT.DEFAULT);
      recalculateData(gc);
      gc.dispose();

      return fitToScreen ? new Point(Math.max(wHint, 240 * maxLvl), Math.max(hHint, 240 * maxLvl)) : new Point(diameter * (maxLvl + 1), diameter * (maxLvl + 1));
   }

   /**
    * @param aceptedlist
    * @param node
    */
   protected void updateObjects(AbstractObject rootObject, Collection<AbstractObject> objects)
   {
      this.rootObject = rootObject;
      this.objects.clear();
      for(AbstractObject o : objects)
         this.objects.put(o.getObjectId(), new ObjectData(o));
      needRender = true;
      redraw();
   }

	/**
	 * Check if given object is displayed
	 * 
	 * @param id object id
	 * @return true if given object is displayed
	 */
	protected boolean containsObject(long id)
	{
	   return (rootObject.getObjectId() == id) || objects.containsKey(id);
	}

   /**
    * Check if given object is displayed and is changed
    * 
    * @param id object id
    * @return true if given object is displayed
    */
   protected boolean containsChangedObject(AbstractObject object)
   {
      if (rootObject == null)
         return false;
      if (rootObject.getObjectId() == object.getObjectId())
         return true;
      ObjectData d = objects.get(object.getObjectId());
      return (d != null) ? d.isChanged(object) : false;
   }

   /**
    * Get object at given point within widget
    * 
    * @param x X coordinate
    * @param y Y coordinate
    * @return object at point or null
    */
   public AbstractObject getObjectFromPoint(int x, int y)
   {
      AbstractObject object = null;
      int clickLvl = (int)(Math.sqrt(Math.pow((x - centerX), 2) + Math.pow((y - centerY), 2)) / (diameter / 2) + 1);
      double clickDegree = Math.toDegrees(Math.atan2(centerX - x, centerY - y));

      clickDegree += 90;
      if (clickDegree < 0)
         clickDegree = 360 + clickDegree;

      ObjectPosition curr = new ObjectPosition((float)clickDegree, (float)clickDegree, clickLvl, null);

      for(ObjectPosition p : objectMap)
      {
         if (p.equals(curr))
         {
            object = p.getObject();
            break;
         }
      }

      return object;
   }

   /**
    * Object position information
    */
   class ObjectPosition
   {
      float startDegree;
      float endDegree;
      int level;
      AbstractObject object;
      
      public ObjectPosition(float startDegree, float endDegree, int lvl, AbstractObject obj)
      {
         this.startDegree = startDegree;
         this.endDegree = endDegree;
         this.level = lvl;
         this.object = obj;
      }

      public AbstractObject getObject()
      {
         return object;
      }

      /**
       * @see java.lang.Object#equals(java.lang.Object)
       */
      @Override
      public boolean equals(Object other)
      {
         if (((ObjectPosition)other).level != level)
         {
            return false;
         }
         if (((ObjectPosition)other).startDegree >= startDegree && ((ObjectPosition)other).endDegree <= endDegree)
         {
            return true;
         }
         return false;
      }

      /**
       * @see java.lang.Object#hashCode()
       */
      @Override
      public int hashCode()
      {
         return Float.floatToIntBits(startDegree) + level;
      }
   }

   /**
    * Object data shown on status map
    */
   private class ObjectData
   {
      private String name;
      private ObjectStatus status;
      private long[] children;
      
      public ObjectData(AbstractObject object)
      {
         name = object.getNameWithAlias();
         status = object.getStatus();
         children = AbstractObjectStatusMap.isContainerObject(object) ? object.getChildIdList() : null;
      }
      
      public boolean isChanged(AbstractObject object)
      {
         return (status != object.getStatus()) || !name.equals(object.getNameWithAlias()) ||
               (AbstractObjectStatusMap.isContainerObject(object) && !Arrays.equals(children, object.getChildIdList()));
      }
   }
}
