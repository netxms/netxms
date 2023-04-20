/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Widget showing "heat" map of nodes under given root object as radial ("sunburst") diagram
 */
public class RadialObjectStatusMap extends AbstractObjectStatusMap
{
	private ObjectStatusSunburstDiagram widget;
	private AbstractObject tooltipObject = null;
   private ObjectPopupDialog tooltipDialog = null;

   /**
    * Create "sunburst" status map
    *
    * @param view owning view
    * @param parent parent composite
    * @param style widget's style
    */
   public RadialObjectStatusMap(ObjectView view, Composite parent, int style)
	{
      super(view, parent, style);
	}

   /**
    * @param root
    * @param filterByTest
    * @return
    */
   protected Collection<AbstractObject> createFilteredList(AbstractObject root)
   {
      Map<Long, AbstractObject> acceptedlist = new HashMap<Long, AbstractObject>();
      for(AbstractObject obj : root.getAllChildren(-1))
      {
         if ((isContainerObject(obj) || isLeafObject(obj)) && isAcceptedByFilter(obj))
            acceptedlist.put(obj.getObjectId(), obj);
      }
      return acceptedlist.values();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.widgets.AbstractObjectStatusMap#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Composite createContent(Composite parent)
   {
      widget = new ObjectStatusSunburstDiagram(parent, null, null);

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
                  showObjectDetails(object);
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

      return widget;
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.widgets.AbstractObjectStatusMap#refresh()
    */
   @Override
	public void refresh()
	{
      AbstractObject root = session.findObjectById(rootObjectId);
      if (root == null)
         return;

      Collection<AbstractObject> objects = createFilteredList(root);
      widget.updateObjects(root, objects);
      widget.refresh();
		for(Runnable l : refreshListeners)
		   l.run();
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.widgets.AbstractObjectStatusMap#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if (widget.containsChangedObject(object) || (object.isChildOf(rootObjectId) && isAcceptedByFilter(object)))
         refreshTimer.execute();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.widgets.AbstractObjectStatusMap#onObjectDelete(long)
    */
   @Override
   protected void onObjectDelete(long objectId)
   {
      if (widget.containsObject(objectId))
         refreshTimer.execute();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.widgets.AbstractObjectStatusMap#computeSize()
    */
   @Override
   protected Point computeSize()
   {      
      if (!fitToScreen)
         return widget.computeSize(SWT.DEFAULT, SWT.DEFAULT);

      Rectangle rect = getAvailableClientArea();
      return widget.computeSize(rect.width, rect.height);
   }
}
