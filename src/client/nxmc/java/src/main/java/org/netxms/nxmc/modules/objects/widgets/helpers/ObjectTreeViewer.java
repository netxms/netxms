/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import java.util.Collection;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.IElementComparer;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.events.TreeEvent;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Circuit;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;

/**
 * Custom extension of tree viewer for object tree
 */
public class ObjectTreeViewer extends TreeViewer
{
   private NXCSession session;
   private boolean objectsFullySync;

	/**
	 * @param parent
	 * @param style
	 */
	public ObjectTreeViewer(Composite parent, int style, boolean objectsFullySync)
	{
		super(parent, style);

      session = Registry.getSession();
		this.objectsFullySync = objectsFullySync;

      setUseHashlookup(true);

		setComparer(new IElementComparer() {
         @Override
         public int hashCode(Object element)
         {
            return (element instanceof AbstractObject) ? (int)((AbstractObject)element).getObjectId() : element.hashCode();
         }

         @Override
         public boolean equals(Object a, Object b)
         {
            if ((a instanceof AbstractObject) && (b instanceof AbstractObject))
               return ((AbstractObject)a).getObjectId() == ((AbstractObject)b).getObjectId();
            return a.equals(b);
         }
      });
	}

   /**
    * Refresh objects form given collection.
    *
    * @param objects updated objects
    */
   public void refreshObjects(Collection<AbstractObject> objects)
   {
      for(AbstractObject o : objects)
      {
         refresh(o);
      }
   }

   /**
    * @param item
    * @return
    */
	public ViewerRow getTreeViewerRow(TreeItem item)
	{
		return getViewerRowFromItem(item);
	}

	/**
	 * Toggle item's expanded/collapsed state
	 * 
	 * @param item
	 */
	public void toggleItemExpandState(TreeItem item)
	{
		if (item.getExpanded())
		{
			item.setExpanded(false);
		}
		else
		{
	      checkAndSyncChildren((AbstractObject)item.getData());
			createChildren(item);
			item.setExpanded(true);
		}
	}

   /**
    * @see org.eclipse.jface.viewers.TreeViewer#handleTreeExpand(org.eclipse.swt.events.TreeEvent)
    */
   @Override
   protected void handleTreeExpand(TreeEvent event) 
   {
      checkAndSyncChildren((AbstractObject)event.item.getData());
      super.handleTreeExpand(event);
   }
   
   /**
    * Check is child objects are synchronized and synchronize if needed
    * 
    * @param object current object
    */
   private void checkAndSyncChildren(AbstractObject object)
   {
      if (!objectsFullySync && ((object instanceof Node) || (object instanceof Circuit)) && object.hasChildren() && !session.areChildrenSynchronized(object.getObjectId()))
      {
         syncChildren(object);
      }
   }

   /**
    * Sync object children form server
    * 
    * @param object 
    */
   private void syncChildren(final AbstractObject object)
   {
      Job job = new Job("Synchronizing node components", null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.syncChildren(object);
            runInUIThread(() -> refresh(object));
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot synchronize node components";
         }
      };
      job.setUser(false);
      job.start();  
   }
}
