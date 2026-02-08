/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.views;

import java.util.UUID;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.TableValueViewer;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Display last value of table DCI
 */
public class TableLastValuesView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(TableLastValuesView.class);

	private long contextId;
   private long ownerId;
	private long dciId;
   private String fullName;
   private TableValueViewer viewer;
   private Action actionExportAllToCsv;

   /**
    * Build view ID
    *
    * @param object context object
    * @param items list of DCIs to show
    * @return view ID
    */
   private static String buildId(AbstractObject object, long dciId)
   {
      StringBuilder sb = new StringBuilder("objects.table-last-values");
      if (object != null)
      {
         sb.append('.');
         sb.append(object.getObjectId());
      }
      sb.append('.');
      sb.append(dciId);
      return sb.toString();
   }  

   /**
    * Create table last value view.
    *
    * @param contextObject context object
    * @param ownerId owning object ID
    * @param dciId DCI ID
    */
   public TableLastValuesView(AbstractObject contextObject, long ownerId, long dciId)
   {
      super(LocalizationHelper.getI18n(TableLastValuesView.class).tr("Table Last Value"), ResourceManager.getImageDescriptor("icons/object-views/table-value.png"), 
            buildId(contextObject, dciId), true);

      contextId = contextObject.getObjectId();
      this.ownerId = ownerId;
      this.dciId = dciId;

      String nodeName = contextObject.getObjectName();
      fullName = nodeName + ": [" + Long.toString(dciId) + "]";
      setName(Long.toString(dciId));
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      TableLastValuesView view = (TableLastValuesView)super.cloneView();
      view.contextId = contextId;
      view.ownerId = ownerId;
      view.dciId = dciId;
      view.fullName = fullName;
      return view;
   }

   /**
    * Default constructor for use by cloneView()
    */
   public TableLastValuesView()
   {
      super(LocalizationHelper.getI18n(TableLastValuesView.class).tr("Table Last Value"), ResourceManager.getImageDescriptor("icons/object-views/table-value.png"), UUID.randomUUID().toString(), true); 
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      return fullName;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
		viewer = new TableValueViewer(parent, SWT.NONE, this, null, false);
		setFilterClient(viewer.getViewer(), viewer.getFilter());
		createActions();

		viewer.setObject(ownerId, dciId);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionExportAllToCsv = new ExportToCsvAction(this, viewer.getViewer(), false);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportAllToCsv);
      super.fillLocalMenu(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
		manager.add(actionExportAllToCsv);
      super.fillLocalToolBar(manager);
	}

	/**
	 * Refresh table
	 */
	@Override
	public void refresh()
	{
	   viewer.resetColumns();
      viewer.refresh(() -> {
         setName(viewer.getTitle());
         fullName = viewer.getObjectName() + ": " + viewer.getTitle();
		});
	}

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject) && (((AbstractObject)context).getObjectId() == contextId);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
      if ((newContext == null) || !(newContext instanceof AbstractObject) || (((AbstractObject)newContext).getObjectId() != contextId))
         return;

      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("contextId", contextId);
      memento.set("ownerId", ownerId);
      memento.set("dciId", dciId);
   }  

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      contextId = memento.getAsLong("contextId", 0);
      ownerId = memento.getAsLong("ownerId", 0);
      dciId = memento.getAsLong("dciId", 0);

      AbstractObject contextObject = session.findObjectById(contextId);
      if (contextObject != null)
      {
         String nodeName = contextObject.getObjectName();
         fullName = nodeName + ": [" + Long.toString(dciId) + "]";
         setName(Long.toString(dciId));
      }
      else
      {
         throw(new ViewNotRestoredException(i18n.tr("Invalid object id")));
      }
   }
}
