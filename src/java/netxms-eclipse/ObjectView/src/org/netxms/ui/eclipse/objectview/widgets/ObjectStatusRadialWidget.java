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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Container;
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
	private int objCount;
	
	private int deametr;
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
		
		addPaintListener(this);
		setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		cCache = new ColorCache();
	}
	
	/**
	 * 
	 * @param object
	 */
	private int drawParts(PaintEvent e, AbstractObject object, int lvl, float degree)
	{	   
	   int numOfObj = 0;
	   for(AbstractObject obj : object.getChildsAsArray())
	   {	      
	      float currObjsize = 0;	      
	      if(obj instanceof Container)
	      {
	         int parentSize = drawParts(e, obj, lvl+1, degree); 
	         if(aceptedlist != null && parentSize == 0)            
               continue;
	         
	         numOfObj += (parentSize == 0) ? 1 : parentSize;
            currObjsize = (parentSize == 0) ? 1 : parentSize;
            currObjsize*=360/(float)objCount; 	 
            //white circle before each level
            e.gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
            e.gc.setAlpha(255);
            e.gc.fillArc(centerX-(deametr*lvl)/2-3, centerY-(deametr*lvl)/2-3, deametr*lvl+6, deametr*lvl+6,(int)(degree+1),(int)(currObjsize-1));
            
            //draw 
            e.gc.setBackground(StatusDisplayInfo.getStatusColor(obj.getStatus()));
            e.gc.setAlpha(255);
            e.gc.fillArc(centerX-(deametr*lvl)/2, centerY-(deametr*lvl)/2, deametr*lvl, deametr*lvl,(int)(degree+1),(int)(currObjsize -1));   
            
            objectMap.add(new ObjLocation(degree, degree+currObjsize, lvl, obj));  
	      }
	      else
	      {
	         if(aceptedlist != null && !aceptedlist.contains(obj.getObjectId()))            
	            continue;
	         
	         numOfObj++;
            currObjsize = 360/(float)objCount;
            
            //white circle before each level
            e.gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
            e.gc.setAlpha(255);
            e.gc.fillArc(centerX-(deametr*lvl)/2-3, centerY-(deametr*lvl)/2-3, deametr*lvl+6, deametr*lvl+6,(int)(degree+1),(int)(currObjsize-1));
            
	         //System.out.println(obj.getObjectName() + " - name, status: "+obj.getStatus());
	         e.gc.setBackground(StatusDisplayInfo.getStatusColor(obj.getStatus()));
	         e.gc.setAlpha(255);
	         e.gc.fillArc(centerX-(deametr*lvl)/2, centerY-(deametr*lvl)/2, deametr*lvl, deametr*lvl,(int)(degree+1),(int)(currObjsize-1)); 
           
	         objectMap.add(new ObjLocation(degree, degree+currObjsize, lvl, obj));
	      }

         //test
         e.gc.setAdvanced(true);
         if (!e.gc.getAdvanced()) 
         {
           e.gc.drawText("Advanced graphics not supported", 30, 30, true);  
         } 
         
	      Transform oldTransform = new Transform(e.gc.getDevice());  
	      e.gc.getTransform(oldTransform);
	      
         //draw text         
         String text = (obj instanceof AbstractNode) ?
               (obj.getObjectName() + "\n" + ((AbstractNode)obj).getPrimaryIP().getHostAddress()) : //$NON-NLS-1$
               obj.getObjectName();
               
         Transform tr = new Transform(getDisplay());
         tr.translate(centerX, centerY);
         
         //text centring
         int h = e.gc.textExtent(text, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER).y;
         float rotate = 0;
         float middle = degree+currObjsize/2;
         if(middle>=90 && middle <180)
            rotate = 90 - (middle - 90);
         if(middle>=270 && middle <= 360)
            rotate = 90 - (middle - 270);
         if(middle>=0 && middle <90)
            rotate = -middle;
         if(middle>=180 && middle < 270)
            rotate = -(middle-180);
         
         tr.rotate(rotate);
         e.gc.setTransform(tr);
         
         //cut text function
         int l = e.gc.textExtent(text, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER).x;
         if(l > deametr/2)
         {
            String name = obj.getObjectName();
            String ipAddress = (obj instanceof AbstractNode) ? ((AbstractNode)obj).getPrimaryIP().getHostAddress() : "";
            int nameL = e.gc.textExtent(name, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER).x;
            if(nameL > deametr/2)
            {
               int numOfCharToLeave = (int)(deametr/2/(nameL/name.length()));
               name = name.subSequence(0, numOfCharToLeave-4).toString();//make best gues
               name+="...";
            }
            if(!ipAddress.isEmpty())
            {   
               int ipL = e.gc.textExtent(ipAddress, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER).x;
               if(ipL > deametr/2)
               {
                  int numOfCharToLeave = (int)(deametr/2/(ipL/ipAddress.length()));
                  ipAddress = ipAddress.subSequence(0, numOfCharToLeave-4).toString();//make best gues
                  ipAddress+="...";
               }
               text=name+"\n"+ipAddress;
            }
            else
               text=name;
         }
         
         e.gc.setForeground(ColorConverter.selectTextColorByBackgroundColor(StatusDisplayInfo.getStatusColor(obj.getStatus()), cCache));
            
         if(middle>=90 && middle <=180 || middle>180 && middle < 270)
            e.gc.drawText(text, -(deametr*(lvl))/2+5, -h/2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);
         else
            e.gc.drawText(text, (deametr*(lvl-1))/2+5, -h/2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);

         e.gc.setTransform(oldTransform);    
         tr.dispose();
         oldTransform.dispose();
         degree+=currObjsize;  
	   }   
      return numOfObj;
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
         if(aceptedlist != null)            
         {
            if(obj instanceof Container)
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
            if(obj instanceof Container)
            {
               objcoutn += calculateMaxLVLAndObjCount(obj, lvl+1);    
               contFound++;
            }
            else
               objcoutn++;
         }
      }
      
      if(aceptedlist == null && (object.getChildIdList().length - contFound) == 0)            
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
	   objectMap.clear();
      //find number of layers
	   objCount = calculateMaxLVLAndObjCount(object, 1);
      
		Rectangle rect = getClientArea();
		rect.width--;
		rect.height--;
		
		e.gc.setAntialias(SWT.ON);
		e.gc.setTextAntialias(SWT.ON);
		e.gc.setForeground(SharedColors.getColor(SharedColors.TEXT_NORMAL, getDisplay()));
		e.gc.setLineWidth(1);
		//calculate values
      int rectSide = Math.min(rect.width, rect.height);
      deametr = rectSide / (maxLvl+1);
      centerX = rect.x+(rectSide/2);
      centerY = rect.y+(rectSide/2);

		//draw objects
		drawParts(e, object, 2, 0);		
		
		//draw general oval with 	
		//draw white oval
      e.gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));
      e.gc.setAlpha(255);
      e.gc.fillOval(centerX-deametr/2-3, centerY-deametr/2-3, deametr+6, deametr+6);
      
		e.gc.setBackground(StatusDisplayInfo.getStatusColor(object.getStatus()));
		e.gc.setAlpha(255);
      e.gc.fillOval(centerX-deametr/2, centerY-deametr/2, deametr, deametr);

		final String text = (object instanceof AbstractNode) ?
		      (object.getObjectName() + "\n" + ((AbstractNode)object).getPrimaryIP().getHostAddress()) : //$NON-NLS-1$
		      object.getObjectName();
		      
		e.gc.setClipping(rect);
		int h = e.gc.textExtent(text, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER).y;
      int l = e.gc.textExtent(text, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER).x;
		e.gc.drawText(text, rect.x+(rectSide/2) - l/2, rect.y+(rectSide/2)-h/2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);

      objectMap.add(new ObjLocation(0, 360, 1, object));      
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
	   if(maxLvl == 0)
	      objCount = calculateMaxLVLAndObjCount(object, 1);
		return new Point(240*maxLvl, 240*maxLvl);
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

   public AbstractObject getObjectByPoint(int x, int y)
   {
      AbstractObject object = null;      
      int clickLvl = (int)(Math.sqrt(Math.pow((x-centerX),2)+Math.pow((y-centerY),2))/(deametr/2)+1);      
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
         return Float.hashCode(startDegree)+lvl;
      }
   }
}
