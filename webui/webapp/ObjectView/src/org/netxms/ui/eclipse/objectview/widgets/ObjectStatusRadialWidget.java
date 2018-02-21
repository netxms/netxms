/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.core.runtime.NullProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Transform;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Widget representing object status
 */
public class ObjectStatusRadialWidget extends Canvas implements PaintListener
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
	
	private List<ObjectPosition> objectMap = new ArrayList<ObjectPosition>();
	
	/**
	 * @param parent
	 * @param aceptedlist 
	 */
	public ObjectStatusRadialWidget(Composite parent, AbstractObject rootObject, Collection<AbstractObject> objects)
	{
		super(parent, SWT.FILL);
		this.rootObject = rootObject;
		
		this.objects = new HashMap<Long, ObjectData>();
		for(AbstractObject o : objects)
		   this.objects.put(o.getObjectId(), new ObjectData(o));
      
		cCache = new ColorCache(this);

      setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
      addPaintListener(this);
	}
	
	/**
	 * Calculate substring for string to fit in the sector
	 * 
	 * @param text object name
	 * @param gc gc object
	 * @param lineNum number of lines that can be used to display object name
	 * @return  formated string
	 */
	private String substring(String text, GC gc, int lineNum)
	{
      StringBuilder name = new StringBuilder("");
      int start = 0;
      for(int i = 0; i < lineNum; i++)
      {
         int end = text.length();
         String substr = text.substring(start);
         int nameL = gc.textExtent(substr).x;
         int numOfCharToLeave = (int)((diameter / 2 - 6)/(nameL/substr.length())); //make best guess
         if(numOfCharToLeave >= substr.length())
            numOfCharToLeave = substr.length() - 1;
         String tmp;
         for(int j = 0; nameL > diameter / 2 - 6; j++)
         {
            if(i+1 == lineNum)
            {
               tmp = substr.substring(0, numOfCharToLeave-j).toString(); 
               tmp += "...";
            }
            else
            {
               tmp = substr.substring(0, numOfCharToLeave-j).toString();
            }
            nameL = gc.textExtent(tmp).x;      
            end = numOfCharToLeave - j + start;
         }         
         
         name.append(text.substring(start, end));
         if(end == text.length())
         {
            break;
         }
         else
         {
            name.append((i+1 == lineNum) ? "..." : "\n" );
         }
         start = end;
      }	   
	   
	   return name.toString();	   
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
	   for(AbstractObject obj : object.getChildsAsArray())
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
         gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
         gc.fillArc(centerX - (diameter * lvl) / 2 - 3, centerY - (diameter * lvl) / 2 - 3, 
               diameter * lvl + 6, diameter * lvl + 6, Math.round(degree), Math.round(currObjsize) + compensation);
         
         gc.setBackground(StatusDisplayInfo.getStatusColor(obj.getStatus()));
         gc.fillArc(centerX - (diameter * lvl) / 2, centerY - (diameter * lvl) / 2, 
               diameter * lvl, diameter * lvl, Math.round(degree), Math.round(currObjsize) - 1 + compensation); 
        
         objectMap.add(new ObjectPosition(degree, degree + currObjsize, lvl, obj));
	      
	      Transform oldTransform = new Transform(gc.getDevice());  
	      gc.getTransform(oldTransform);
	      
         //draw text         
         String text = obj.getObjectName();
               
         Transform tr = new Transform(getDisplay());
         tr.translate(centerX, centerY);
         
         //text centering
         float rotate = 0;
         float middle = degree + currObjsize / 2;
         if (middle >= 90 && middle < 180)
            rotate = 90 - (middle - 90);
         if (middle >= 270 && middle <= 360)
            rotate = 90 - (middle - 270);
         if (middle >= 0 && middle < 90)
            rotate = -middle;
         if (middle >= 180 && middle < 270)
            rotate = -(middle - 180);
         
         tr.rotate(rotate);
         gc.setTransform(tr);
         
         // cut text function
         int l = gc.textExtent(text).x;
         int h = gc.textExtent(text).y;
         //high of sector
         int high = (int)((Math.tan(currObjsize/2))*((diameter * lvl) / 2)*2)-6;
         int lineNum = high/(h+13);
         if(lineNum < 0 || lineNum > 3)
            lineNum = 3;
         if (l > diameter / 2 - 6)
         {
            text = substring(text, gc, lineNum);
         }
         h = gc.textExtent(text).y;
         
         gc.setForeground(ColorConverter.selectTextColorByBackgroundColor(StatusDisplayInfo.getStatusColor(obj.getStatus()), cCache));

         if(middle>=90 && middle <=180 || middle>180 && middle < 270)
         {
            l = gc.textExtent(text).x;
            gc.drawText(text, -(diameter*(lvl-1))/2-6-l, -h/2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);
         }
         else
            gc.drawText(text, (diameter*(lvl-1))/2+5, -h/2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);

         gc.setTransform(oldTransform);    
         tr.dispose();
         oldTransform.dispose();
	      
         degree += currObjsize;
         objectSize += currObjsize;
	   }   
      return objectSize;
	}
	
	/**
    * 
    * @param object
    * @param lvl
    * @return
    */
   private int calculateMaxLVLAndObjCount(AbstractObject object, int lvl)
   {     
      int objcoutn = 0;
      int contFound = 0;
      AbstractObject[] objSet = object.getChildsAsArray();
      for(AbstractObject obj : objSet )
      {
         if (objects != null)            
         {
            if (AbstractObjectStatusMap.isContainerObject(obj))
            {
               int tmp = calculateMaxLVLAndObjCount(obj, lvl+1);    
               if(tmp > 0)
               {
                  contFound++;
                  objcoutn+=tmp;
               }
            }
            else
            {
               if (objects.containsKey(obj.getObjectId()))
                  objcoutn++;
            }
         }
         else
         {
            if (AbstractObjectStatusMap.isContainerObject(obj))
            {
               objcoutn += calculateMaxLVLAndObjCount(obj, lvl+1);    
               contFound++;
            }
            else
               objcoutn++;
         }
      }
      
      if ((objects == null) && AbstractObjectStatusMap.isContainerObject(object) && (objcoutn == 0))            
      {
         objcoutn++;
      }
         
      if (contFound == 0)
      {
         if (maxLvl < lvl)
            maxLvl = lvl;
      }
      
      return objcoutn;
   }
   
	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
	   GC gc = e.gc;
      gc.setAdvanced(true);
      if (!gc.getAdvanced()) 
      {
        gc.drawText("Advanced graphics not supported", 30, 30, true);
        return;
      } 
	   
	   objectMap.clear();
	   leafObjectCount = calculateMaxLVLAndObjCount(rootObject, 1);
      leafObjectSize = 360 / (float)leafObjectCount;
      
		Rectangle rect = getClientArea();
		rect.width--;
		rect.height--;
		
		gc.setAntialias(SWT.ON);
		gc.setTextAntialias(SWT.ON);
		gc.setForeground(SharedColors.getColor(SharedColors.TEXT_NORMAL, getDisplay()));
		gc.setLineWidth(1);
		//calculate values
      int rectSide = Math.min(rect.width, rect.height);
      diameter = rectSide / (maxLvl + 1);
      centerX = rect.x+(rectSide/2);
      centerY = rect.y+(rectSide/2);

		//draw objects
		drawParts(gc, rootObject, 2, 0);		
		
		//draw general oval with 	
		//draw white oval
      gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
      gc.setAlpha(255);
      gc.fillOval(centerX-diameter/2-3, centerY-diameter/2-3, diameter+6, diameter+6);
      
		gc.setBackground(StatusDisplayInfo.getStatusColor(rootObject.getStatus()));
		gc.setAlpha(255);
      gc.fillOval(centerX-diameter/2, centerY-diameter/2, diameter, diameter);

		final String text = (rootObject instanceof AbstractNode) ?
		      (rootObject.getObjectName() + "\n" + ((AbstractNode)rootObject).getPrimaryIP().getHostAddress()) : //$NON-NLS-1$
		      rootObject.getObjectName();
		      
		gc.setClipping(rect);
		int h = gc.textExtent(text).y;
      int l = gc.textExtent(text).x;
      gc.setForeground(ColorConverter.selectTextColorByBackgroundColor(StatusDisplayInfo.getStatusColor(rootObject.getStatus()), cCache));
		gc.drawText(text, rect.x+(rectSide/2) - l/2, rect.y+(rectSide/2)-h/2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);

      objectMap.add(new ObjectPosition(0, 360, 1, rootObject));           
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
	   if (maxLvl == 0)
	      calculateMaxLVLAndObjCount(rootObject, 1);
		return new Point(240 * maxLvl, 240 * maxLvl);
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
      if (rootObject.getObjectId() == object.getObjectId())
         return true;
      ObjectData d = objects.get(object.getObjectId());
      return (d != null) ? d.isChanged(object) : false;
   }

   /**
    * Get object at given point within widget
    * 
    * @param x
    * @param y
    * @return
    */
   public AbstractObject getObjectFromPoint(int x, int y)
   {
      AbstractObject object = null;      
      int clickLvl = (int)(Math.sqrt(Math.pow((x-centerX),2)+Math.pow((y-centerY),2))/(diameter/2)+1);      
      double clickDegree = Math.toDegrees(Math.atan2(centerX - x, centerY - y));
      
      clickDegree += 90;
      if(clickDegree < 0)
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
      int lvl;
      AbstractObject obj;
      
      public ObjectPosition(float startDegree, float endDegree, int lvl, AbstractObject obj)
      {
         this.startDegree = startDegree;
         this.endDegree = endDegree;
         this.lvl = lvl;
         this.obj = obj;
      }
      
      public AbstractObject getObject()
      {
         return obj;
      }
      
      /* (non-Javadoc)
       * @see java.lang.Object#equals(java.lang.Object)
       */
      @Override
      public boolean equals(Object arg0)
      {
         if(((ObjectPosition)arg0).lvl != lvl)
         {
            return false;
         }
         if(((ObjectPosition)arg0).startDegree >= startDegree && ((ObjectPosition)arg0).endDegree <= endDegree)
         {
            return true;
         }
         return false;         
      }
      
      /* (non-Javadoc)
       * @see java.lang.Object#hashCode()
       */
      @Override
      public int hashCode()
      {
         return Float.floatToIntBits(startDegree)+lvl;
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
         name = object.getObjectName();
         status = object.getStatus();
         children = AbstractObjectStatusMap.isContainerObject(object) ? object.getChildIdList() : null;
      }
      
      public boolean isChanged(AbstractObject object)
      {
         return (status != object.getStatus()) || !name.equals(object.getObjectName()) ||
               (AbstractObjectStatusMap.isContainerObject(object) && !Arrays.equals(children, object.getChildIdList()));
      }
   }
}
