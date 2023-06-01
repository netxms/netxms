/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.logviewer.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ColumnFilterSetOperation;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogFilter;
import org.netxms.client.log.OrderingColumn;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.LogRecordDetailsViewer;
import org.netxms.nxmc.modules.logviewer.LogRecordDetailsViewerRegistry;
import org.netxms.nxmc.modules.logviewer.widgets.LogWidget;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Universal log viewer
 */
public class ObjectLogViewer extends AdHocObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectLogViewer.class);
   
   protected NXCSession session = Registry.getSession();
   private LogWidget logWidget;
   private String logName;
   private String filterColumn;
   private String sortingColumn;
   private LogRecordDetailsViewer recordDetailsViewer;

   /**
    * @see org.netxms.nxmc.base.views.View#cloneView()
    */
   @Override
   public View cloneView()
   {
      ObjectLogViewer view = (ObjectLogViewer)super.cloneView();
      
      view.logName = logName;
      view.recordDetailsViewer = recordDetailsViewer;
      return view;
   }
   
   /**
    * Internal constructor used for cloning
    */
   protected ObjectLogViewer()
   {
      super(null, null, null, 0, 0, false); 
   }

   /**
    * Create new log viewer view
    *
    * @param viewName view name
    * @param logName log name
    */
   public ObjectLogViewer(String viewName, String logName, String filterColumn, String sortingColumn, long objectId, long contextId)
   {
      super(viewName, ResourceManager.getImageDescriptor("icons/log-viewer/" + logName + ".png"), "LogViewer." + logName, objectId, contextId,  false);
      this.logName = logName;
      this.filterColumn = filterColumn;
      this.sortingColumn = sortingColumn;      
      recordDetailsViewer = LogRecordDetailsViewerRegistry.get(logName);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   public void createContent(Composite parent)
	{
		parent.setLayout(new FillLayout());		
		logWidget = new LogWidget(this, parent, SWT.NONE, logName);
   }

	
	
   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      ObjectLogViewer origin = (ObjectLogViewer)view;
      logWidget.postClone(origin.logWidget);
      super.postClone(view);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      
      ColumnFilter cf = new ColumnFilter();
      cf.setOperation(ColumnFilterSetOperation.OR);
      cf.addSubFilter(new ColumnFilter((getObject() instanceof AbstractNode) ? ColumnFilterType.EQUALS : ColumnFilterType.CHILDOF, getObjectId()));

      LogFilter filter = new LogFilter();
      filter.setColumnFilter(filterColumn, cf); //$NON-NLS-1$
      List<OrderingColumn> orderingColumns = new ArrayList<OrderingColumn>(1);
      orderingColumns.add(new OrderingColumn(sortingColumn, i18n.tr("Time"), true)); //$NON-NLS-1$
      filter.setOrderingColumns(orderingColumns);
      
      logWidget.queryWithFilter(filter);

      
      logWidget.openServerLog();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
      logWidget.fillLocalMenu(manager);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
      logWidget.fillLocalToolBar(manager);
	}

	/**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
	{
      logWidget.refresh();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
	@Override
	public void setFocus()
	{
	   logWidget.getViewer().getControl().setFocus();
	}
}
