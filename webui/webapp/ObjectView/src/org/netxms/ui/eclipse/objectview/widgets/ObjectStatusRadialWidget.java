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
import java.util.List;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Transform;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Widget representing object status
 */
public class ObjectStatusRadialWidget extends Canvas implements PaintListener
{
	private AbstractObject object;
	private int maxLvl;
	private int leafObjectCount;
	private float leafObjectSize;   // angular size of single leaf object
	
	private int diameter;
	private int centerX;
	private int centerY;
	private ColorCache cCache;
	private Set<Long> aceptedlist;
	
	private List<ObjLocation> objectMap = new ArrayList<ObjLocation>();
	
	/**
	 * @param parent
	 * @param aceptedlist 
	 */
	public ObjectStatusRadialWidget(Composite parent, AbstractObject object, Set<Long> aceptedlist)
	{
		super(parent, SWT.FILL);
		this.object = object;
		this.aceptedlist = aceptedlist;
      
		cCache = new ColorCache(this);

      setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
      addPaintListener(this);
	}
	
	/**
	 * Check if given object is a container
	 * 
	 * @param object
	 * @return
	 */
	private static boolean isContainerObject(AbstractObject object)
	{
	   return (object instanceof Container) || (object instanceof Cluster) || (object instanceof Rack) || (object instanceof Chassis);
	}
	
	/**
	 * 
	 * @param object
	 */
	private float drawParts(PaintEvent e, AbstractObject object, int lvl, float degree)
	{	   
      e.gc.setAlpha(255);

      float objectSize = 0;
	   for(AbstractObject obj : object.getChildsAsArray())
	   {	      
	      float currObjsize = 0;	      
	      if (isContainerObject(obj))
	      {
	         currObjsize = drawParts(e, obj, lvl + 1, degree); 
	         if (currObjsize == 0)
	         {
	            if (aceptedlist != null)
	               continue;
	            currObjsize = leafObjectSize;
	         }
	      }
	      else
	      {
	         if (aceptedlist != null && !aceptedlist.contains(obj.getObjectId()))            
	            continue;
	         
            currObjsize = leafObjectSize;
	      }

	      // compensate for rounding errors by either extending or shrinking current object by one degree
         int compensation = Integer.signum(Math.round(degree + currObjsize) - (Math.round(degree) + Math.round(currObjsize)));
         
         // white circle before each level
         e.gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
         e.gc.fillArc(centerX - (diameter * lvl) / 2 - 3, centerY - (diameter * lvl) / 2 - 3, 
               diameter * lvl + 6, diameter * lvl + 6, Math.round(degree), Math.round(currObjsize) + compensation);
         
         e.gc.setBackground(StatusDisplayInfo.getStatusColor(obj.getStatus()));
         e.gc.fillArc(centerX - (diameter * lvl) / 2, centerY - (diameter * lvl) / 2, 
               diameter * lvl, diameter * lvl, Math.round(degree), Math.round(currObjsize) - 1 + compensation); 
        
         objectMap.add(new ObjLocation(degree, degree + currObjsize, lvl, obj));
	      
	      Transform oldTransform = new Transform(e.gc.getDevice());  
	      e.gc.getTransform(oldTransform);
	      
         //draw text         
         String text = obj.getObjectName();
               
         Transform tr = new Transform(getDisplay());
         tr.translate(centerX, centerY);
         
         //text centering
         int h = e.gc.textExtent(text).y;
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
         e.gc.setTransform(tr);
         
         // cut text function
         int l = e.gc.textExtent(text).x;
         if (l > diameter / 2)
         {
            String name = obj.getObjectName();
            int nameL = e.gc.textExtent(name).x;
            if(nameL > diameter/2)
            {
               int numOfCharToLeave = (int)(diameter/2/(nameL/name.length()));
               name = name.subSequence(0, numOfCharToLeave-4).toString(); //make best guess
               name += "...";
            }
            text = name;
         }
         
         e.gc.setForeground(ColorConverter.selectTextColorByBackgroundColor(StatusDisplayInfo.getStatusColor(obj.getStatus()), cCache));
            
         if(middle>=90 && middle <=180 || middle>180 && middle < 270)
            e.gc.drawText(text, -(diameter*(lvl))/2+5, -h/2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);
         else
            e.gc.drawText(text, (diameter*(lvl-1))/2+5, -h/2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);

         e.gc.setTransform(oldTransform);    
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
         if (aceptedlist != null)            
         {
            if (isContainerObject(obj))
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
               if(aceptedlist.contains(obj.getObjectId()))
                  objcoutn++;
            }
         }
         else
         {
            if (isContainerObject(obj))
            {
               objcoutn += calculateMaxLVLAndObjCount(obj, lvl+1);    
               contFound++;
            }
            else
               objcoutn++;
         }
      }
      
      if(aceptedlist == null && isContainerObject(object)  && objcoutn == 0)            
      {
         objcoutn++;
      }
         
      if(contFound == 0)
      {
         if(maxLvl < lvl)
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
      e.gc.setAdvanced(true);
      if (!e.gc.getAdvanced()) 
      {
        e.gc.drawText("Advanced graphics not supported", 30, 30, true);
        return;
      } 
	   
	   objectMap.clear();
	   leafObjectCount = calculateMaxLVLAndObjCount(object, 1);
      leafObjectSize = 360 / (float)leafObjectCount;
      
		Rectangle rect = getClientArea();
		rect.width--;
		rect.height--;
		
		e.gc.setAntialias(SWT.ON);
		e.gc.setTextAntialias(SWT.ON);
		e.gc.setForeground(SharedColors.getColor(SharedColors.TEXT_NORMAL, getDisplay()));
		e.gc.setLineWidth(1);
		//calculate values
      int rectSide = Math.min(rect.width, rect.height);
      diameter = rectSide / (maxLvl + 1);
      centerX = rect.x+(rectSide/2);
      centerY = rect.y+(rectSide/2);

		//draw objects
		drawParts(e, object, 2, 0);		
		
		//draw general oval with 	
		//draw white oval
      e.gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
      e.gc.setAlpha(255);
      e.gc.fillOval(centerX-diameter/2-3, centerY-diameter/2-3, diameter+6, diameter+6);
      
		e.gc.setBackground(StatusDisplayInfo.getStatusColor(object.getStatus()));
		e.gc.setAlpha(255);
      e.gc.fillOval(centerX-diameter/2, centerY-diameter/2, diameter, diameter);

		final String text = (object instanceof AbstractNode) ?
		      (object.getObjectName() + "\n" + ((AbstractNode)object).getPrimaryIP().getHostAddress()) : //$NON-NLS-1$
		      object.getObjectName();
		      
		e.gc.setClipping(rect);
		int h = e.gc.textExtent(text).y;
      int l = e.gc.textExtent(text).x;
      e.gc.setForeground(ColorConverter.selectTextColorByBackgroundColor(StatusDisplayInfo.getStatusColor(object.getStatus()), cCache));
		e.gc.drawText(text, rect.x+(rectSide/2) - l/2, rect.y+(rectSide/2)-h/2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);

      objectMap.add(new ObjLocation(0, 360, 1, object));           
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
	   if (maxLvl == 0)
	      calculateMaxLVLAndObjCount(object, 1);
		return new Point(240 * maxLvl, 240 * maxLvl);
	}
	
	/**
	 * @param aceptedlist 
	 * @param node
	 */
	public void updateObject(AbstractObject object, Set<Long> aceptedlist)
	{
		this.object = object;
		this.aceptedlist = aceptedlist;
		redraw();
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
      
      ObjLocation curr = new ObjLocation((float)clickDegree, (float)clickDegree, clickLvl, null);
      
      for(ObjLocation loc : objectMap)
      {
         if(loc.equals(curr))
         {
            object = loc.getObject();
            break;
         }
      }
      
      return object;
   }
   
   /**
    * Object location information
    */
   class ObjLocation
   {
      float startDegree;
      float endDegree;
      int lvl;
      AbstractObject obj;
      
      public ObjLocation(float startDegree, float endDegree, int lvl, AbstractObject obj)
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
         if(((ObjLocation)arg0).lvl != lvl)
         {
            return false;
         }
         if(((ObjLocation)arg0).startDegree >= startDegree && ((ObjLocation)arg0).endDegree <= endDegree)
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
}
