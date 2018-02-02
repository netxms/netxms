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

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Widget showing "heat" map of nodes under given root object
 */
public class ObjectStatusMapRadial extends AbstractObjectStatusMap
{
	private ObjectStatusRadialWidget widget;
	private AbstractObject tooltipObject = null;
   private ObjectPopupDialog tooltipDialog = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectStatusMapRadial(IViewPart viewPart, Composite parent, int style, boolean allowFilterClose)
	{
		super(viewPart, parent, style, allowFilterClose);
	}
	
   /**
    * @param root
    * @param filterByTest
    * @return
    */
   protected Collection<AbstractObject> createFilteredList(AbstractObject root)
   {
      Map<Long, AbstractObject> acceptedlist = new HashMap<Long, AbstractObject>();
      for(AbstractObject obj : root.getAllChilds(-1))
      {
         if ((isContainerObject(obj) || isLeafObject(obj)) && isAcceptedByFilter(obj))
            acceptedlist.put(obj.getObjectId(), obj);
      }
      return acceptedlist.values();
   }
	
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.widgets.AbstractObjectStatusMap#refresh()
    */
   @Override
	public void refresh()
	{
      AbstractObject root = session.findObjectById(rootObjectId);
      Collection<AbstractObject> objects = createFilteredList(root);
      
      if (widget == null)
      {
         widget = new ObjectStatusRadialWidget(dataArea, root, objects);
         widget.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

         widget.addMouseListener(new MouseListener() {
            @Override
            public void mouseUp(MouseEvent e)
            {
            }
            
            @Override
            public void mouseDown(MouseEvent e)
            {
               if ((tooltipDialog != null) && (tooltipDialog.getShell() != null) && !tooltipDialog.getShell().isDisposed() && (e.display.getActiveShell() != tooltipDialog.getShell()))
               {
                  tooltipDialog.close();
                  tooltipDialog = null;
               }
               tooltipObject = null;
               
               AbstractObject object = widget.getObjectFromPoint(e.x, e.y);
               if (object != null)
               {
                  setSelection(new StructuredSelection(object));
                  if (e.button == 1)
                     callDetailsProvider(object);
               }
               else
               {
                  setSelection(new StructuredSelection());
               }
            }
            
            @Override
            public void mouseDoubleClick(MouseEvent e)
            {
            }
         });
         
         WidgetHelper.attachMouseTrackListener(widget, new MouseTrackListener() {
            @Override
            public void mouseHover(MouseEvent e)
            {
               AbstractObject object = widget.getObjectFromPoint(e.x, e.y);
               if ((object != null) && ((object != tooltipObject) || (tooltipDialog == null) || (tooltipDialog.getShell() == null) || tooltipDialog.getShell().isDisposed()))
               {
                  if ((tooltipDialog != null) && (tooltipDialog.getShell() != null) && !tooltipDialog.getShell().isDisposed())
                     tooltipDialog.close();
               
                  tooltipObject = object;
                  tooltipDialog = new ObjectPopupDialog(getShell(), object, widget.toDisplay(e.x, e.y));
                  tooltipDialog.open();
               }
               else if ((object == null) && (tooltipDialog != null) && (tooltipDialog.getShell() != null) && !tooltipDialog.getShell().isDisposed())
               {
                  tooltipDialog.close();
                  tooltipDialog = null;
               }
            }
            
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
            
            @Override
            public void mouseEnter(MouseEvent e)
            {
            }
         });

         // Create popup menu
         Menu menu = menuManager.createContextMenu(widget);
         widget.setMenu(menu);

         // Register menu for extension.
         if (viewPart != null)
            viewPart.getSite().registerContextMenu(menuManager, this);
      }
      else
      {
         widget.updateObjects(root, objects);
         widget.redraw();
      }
		
		dataArea.layout(true, true);
		
		Rectangle r = getClientArea();
		scroller.setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
		
		for(Runnable l : refreshListeners)
		   l.run();
	}

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.widgets.AbstractObjectStatusMap#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if (widget.containsChangedObject(object) || (object.isChildOf(rootObjectId) && isAcceptedByFilter(object)))
         refreshTimer.execute();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.widgets.AbstractObjectStatusMap#onObjectDelete(long)
    */
   @Override
   protected void onObjectDelete(long objectId)
   {
      if (widget.containsObject(objectId))
         refreshTimer.execute();
   }
}
