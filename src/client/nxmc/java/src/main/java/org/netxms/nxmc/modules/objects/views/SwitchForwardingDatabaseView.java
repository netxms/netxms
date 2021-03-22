/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.FdbEntry;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.FDBComparator;
import org.netxms.nxmc.modules.objects.views.helpers.FDBFilter;
import org.netxms.nxmc.modules.objects.views.helpers.FDBLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Switch forwarding database view
 */
public class SwitchForwardingDatabaseView extends ObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(SwitchForwardingDatabaseView.class);

	public static final int COLUMN_MAC_ADDRESS = 0;
	public static final int COLUMN_PORT = 1;
	public static final int COLUMN_INTERFACE = 2;
   public static final int COLUMN_VLAN = 3;
	public static final int COLUMN_NODE = 4;
   public static final int COLUMN_TYPE = 5;
	
	private NXCSession session;
	private SortableTableViewer viewer;
	private Composite resultArea;
	private FilterText filterText;
	private FDBFilter filter;
   private boolean showFilter = true;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionShowFilter;

   /**
    * Default constructor
    */
   public SwitchForwardingDatabaseView()
   {
      super(i18n.tr("FDB"), ResourceManager.getImageDescriptor("icons/object-views/fdb.gif"), "FDB");
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectTableView#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
      final PreferenceStore settings = PreferenceStore.getInstance();
      showFilter = settings.getAsBoolean("FDB.showFilter", showFilter);

      // Create result area
      resultArea = new Composite(parent, SWT.BORDER);
      FormLayout formLayout = new FormLayout();
      resultArea.setLayout(formLayout);
      // Create filter area
      filterText = new FilterText(resultArea, SWT.BORDER);
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
            actionShowFilter.setChecked(showFilter);
         }
      });
      filterText.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            settings.set("FDB.showFilter", showFilter);
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      ;
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);
	   
		final String[] names = { 
            i18n.tr("MAC"), i18n.tr("Port"), i18n.tr("Interface"), i18n.tr("VLAN"), i18n.tr("Node"), i18n.tr("Type")
		   };
		final int[] widths = { 180, 100, 200, 100, 250, 110 };
		viewer = new SortableTableViewer(resultArea, names, widths, COLUMN_MAC_ADDRESS, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new FDBLabelProvider());
		viewer.setComparator(new FDBComparator());
		filter = new FDBFilter();
		viewer.addFilter(filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "SwitchForwardingDatabase");
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            WidgetHelper.saveTableViewerSettings(viewer, "SwitchForwardingDatabase");
			}
		});
		
		// Setup layout
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);;
      fd.top = new FormAttachment(filterText, 0, SWT.BOTTOM);
      fd.bottom = new FormAttachment(100, 0);
      fd.right = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);

		createActions();
		createPopupMenu();
		
		// Set initial focus to filter input line
      if (showFilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
	}

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      showFilter = enable;
      filterText.setVisible(showFilter);
      FormData fd = (FormData)viewer.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText, 0, SWT.BOTTOM) : new FormAttachment(0, 0);
      resultArea.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText("");
         onFilterModify();
      }

   }

   /**
    * Handler for filter modification
    */
   public void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
		
		actionShowFilter = new Action("Show filter", Action.AS_CHECK_BOX) {
	        @Override
	        public void run()
	        {
            enableFilter(!showFilter);
            actionShowFilter.setChecked(showFilter);
	        }
	      };
      actionShowFilter.setChecked(showFilter);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionExportToCsv);
      manager.add(actionExportAllToCsv);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
	@Override
   public void refresh()
	{
      if (getObject() == null)
         return;

      final long objectId = getObject().getObjectId();
      new Job(i18n.tr("Read switch forwarding database"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<FdbEntry> fdb = session.getSwitchForwardingDatabase(objectId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(fdb.toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot get switch forwarding database for node %s"), getObject().getObjectName());
         }
      }.start();
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && ((Node)context).isBridge();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 200;
   }
}
