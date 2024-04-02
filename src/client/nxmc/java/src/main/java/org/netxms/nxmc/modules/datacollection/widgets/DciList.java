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
package org.netxms.nxmc.modules.datacollection.widgets;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DciListComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DciListFilter;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DciListLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Show DCI on given node
 */
public class DciList extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(DciList.class);

	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_PARAMETER = 1;
	public static final int COLUMN_DESCRIPTION = 2;
	
   private boolean filterEnabled = true;
   private DciListFilter filter;
	private FilterText filterText;
   private View view;
	private AbstractObject node;
	private NXCSession session;
	private SortableTableViewer viewer;
	private int dcObjectType;	// DC object type filter; -1 allows all object types
	private boolean allowNoValueObjects = false;
	
	/**
    * Create DCI list widget
    * 
    * @param view owning view
    * @param parent parent widget
    * @param style style
    * @param _node node to display data for
    * @param configPrefix configuration prefix for saving/restoring viewer settings
    */
   public DciList(View view, Composite parent, int style, AbstractNode _node, final String configPrefix, int dcObjectType,
         int selectionType, boolean allowNoValueObjects)
	{
		super(parent, style);
      session = Registry.getSession();
      this.view = view;
		this.node = _node;
		this.dcObjectType = dcObjectType;
		this.allowNoValueObjects = allowNoValueObjects;

      FormLayout formLayout = new FormLayout();
      setLayout(formLayout);

      // Create filter area
      filterText = new FilterText(this, SWT.NONE, null, false);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
         }
      });

		// Setup table columns
      final String[] names = { i18n.tr("ID"), i18n.tr("Metric"), i18n.tr("Display name") };
		final int[] widths = { 70, 150, 250 };
		viewer = new SortableTableViewer(this, names, widths, 2, SWT.DOWN, selectionType | SWT.FULL_SELECTION);

		viewer.setLabelProvider(new DciListLabelProvider());
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setComparator(new DciListComparator());
		filter = new DciListFilter();
		viewer.addFilter(filter);
      WidgetHelper.restoreTableViewerSettings(viewer, configPrefix);

		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            WidgetHelper.saveTableViewerSettings(viewer, configPrefix);
			}
		});

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      // Set initial focus to filter input line
      if (filterEnabled)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

		getDataFromServer();
	}
	
	/**
	 * Get data from server
	 */
	private void getDataFromServer()
	{
		if (node == null)
		{
			viewer.setInput(new DciValue[0]);
			return;
		}

      Job job = new Job(String.format(i18n.tr("Get DCI values for node %s"), node.getObjectName()), view) {
			@Override
			protected String getErrorMessage()
			{
            return String.format(i18n.tr("Cannot get DCI list for node %s"), node.getObjectName());
			}

			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final List<DciValue> data = 
						(dcObjectType == -1) ? Arrays.asList(session.getDataCollectionSummary(node.getObjectId(), false, false, allowNoValueObjects)) 
								: new ArrayList<DciValue>(Arrays.asList(session.getDataCollectionSummary(node.getObjectId(), false, false, allowNoValueObjects)));
				if (dcObjectType != -1)
				{
					Iterator<DciValue> it = data.iterator();
					while(it.hasNext())
					{
						DciValue dci = it.next();
						if (dci.getDcObjectType() != dcObjectType)
							it.remove();
					}
				}
						
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(data.toArray());
					}
				});
			}
		};
		job.setUser(false);
		job.start();
	}
	
	/**
	 * Change node object
	 * 
	 * @param node new node or mobile device object
	 */
	public void setNode(AbstractObject node)
	{
		this.node = node;
		getDataFromServer();
	}
	
	/**
	 * Refresh view
	 */
	public void refresh()
	{
		getDataFromServer();
	}
	
	/**
	 * Get selected DCI
	 * 
	 * @return selected DCI
	 */
	@SuppressWarnings("unchecked")
   public List<DciValue> getSelection()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		// selection.toList may return list which is actual backing for this selection
		// so we make a copy for safety
		return new ArrayList<DciValue>(selection.toList());
	}

	/**
	 * @return the dcObjectType
	 */
	public int getDcObjectType()
	{
		return dcObjectType;
	}

	/**
	 * @param dcObjectType the dcObjectType to set
	 */
	public void setDcObjectType(int dcObjectType)
	{
		this.dcObjectType = dcObjectType;
		getDataFromServer();
	}
	
	/**
	 * @param listener
	 */
	public void addDoubleClickListener(IDoubleClickListener listener)
	{
		viewer.addDoubleClickListener(listener);
	}

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      filterEnabled = enable;
      filterText.setVisible(filterEnabled);
      FormData fd = (FormData)viewer.getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      layout();
      if (enable)
         filterText.setFocus();
      else
         setFilter(""); //$NON-NLS-1$
   }

   /**
    * @return the filterEnabled
    */
   public boolean isFilterEnabled()
   {
      return filterEnabled;
   }
   
   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   public void setFilter(final String text)
   {
      filterText.setText(text);
      onFilterModify();
   }

   /**
    * Get filter text
    * 
    * @return Current filter text
    */
   public String getFilter()
   {
      return filterText.getText();
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }
}
