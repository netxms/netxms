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
package org.netxms.ui.eclipse.topology.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
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
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.FdbEntry;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.Messages;
import org.netxms.ui.eclipse.topology.views.helpers.FDBComparator;
import org.netxms.ui.eclipse.topology.views.helpers.FDBLabelProvider;
import org.netxms.ui.eclipse.topology.views.helpers.SwitchForwardingDatabaseFilter;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Switch forwarding database view
 */
public class SwitchForwardingDatabaseView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.SwitchForwardingDatabaseView"; //$NON-NLS-1$
	
	public static final int COLUMN_MAC_ADDRESS = 0;
	public static final int COLUMN_PORT = 1;
	public static final int COLUMN_INTERFACE = 2;
   public static final int COLUMN_VLAN = 3;
	public static final int COLUMN_NODE = 4;
   public static final int COLUMN_TYPE = 5;
	
	private NXCSession session;
	private long rootObject;
	private SortableTableViewer viewer;
	private Action actionRefresh;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;
	private Action actionShowFilter;
	
	private Composite resultArea;
	private FilterText filterText;
	private SwitchForwardingDatabaseFilter filter;
	private boolean initShowFilter = true;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		try
		{
			rootObject = Long.parseLong(site.getSecondaryId());
		}
		catch(NumberFormatException e)
		{
			rootObject = 0;
		}

		session = (NXCSession)ConsoleSharedData.getSession();
		setPartName(String.format(Messages.get().SwitchForwardingDatabaseView_Title, session.getObjectName(rootObject)));
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
      initShowFilter = safeCast(settings.get("SwitchForwardingDatabase.showFilter"), settings.getBoolean("SwitchForwardingDatabase.showFilter"), initShowFilter);
	}
	
	/**
    * @param b
    * @param defval
    * @return
    */
   private static boolean safeCast(String s, boolean b, boolean defval)
   {
      return (s != null) ? b : defval;
   }

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
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
            actionShowFilter.setChecked(initShowFilter);
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
		      Messages.get().SwitchForwardingDatabaseView_ColMacAddr, 
		      Messages.get().SwitchForwardingDatabaseView_ColPort, 
		      Messages.get().SwitchForwardingDatabaseView_ConIface, 
		      Messages.get().SwitchForwardingDatabaseView_ColVlan, 
		      Messages.get().SwitchForwardingDatabaseView_ColNode,
		      Messages.get().SwitchForwardingDatabaseView_ColType
		   };
		final int[] widths = { 180, 100, 200, 100, 250, 110 };
		viewer = new SortableTableViewer(resultArea, names, widths, COLUMN_MAC_ADDRESS, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new FDBLabelProvider());
		viewer.setComparator(new FDBComparator());
		filter = new SwitchForwardingDatabaseFilter();
		viewer.addFilter(filter);
		
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "SwitchForwardingDatabase"); //$NON-NLS-1$
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "SwitchForwardingDatabase"); //$NON-NLS-1$
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
		contributeToActionBars();
		createPopupMenu();
		
		// Set initial focus to filter input line
      if (initShowFilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
      
      activateContext();
		
		refresh();
	}
	
	/**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.topology.context.Topology"); //$NON-NLS-1$
      }
   }
	
   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      initShowFilter = enable;
      filterText.setVisible(initShowFilter);
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

   @Override
   public void dispose()
   {
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      settings.put("SwitchForwardingDatabase.showFilter", initShowFilter);
      super.dispose();
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
	   final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
	   
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refresh();
			}
		};

		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
		
		actionShowFilter = new Action("Show filter", Action.AS_CHECK_BOX) {
	        @Override
	        public void run()
	        {
	           enableFilter(!initShowFilter);
	           actionShowFilter.setChecked(initShowFilter);
	        }
	      };
	      actionShowFilter.setChecked(initShowFilter);
	      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.topology.commands.show_filter"); //$NON-NLS-1$
	      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
	}

	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionExportAllToCsv);
		manager.add(actionShowFilter);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionExportAllToCsv);
		manager.add(actionRefresh);
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

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

	/**
	 * Refresh content
	 */
	private void refresh()
	{
	   new ConsoleJob(Messages.get().SwitchForwardingDatabaseView_JobTitle, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<FdbEntry> fdb = session.getSwitchForwardingDatabase(rootObject);               
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
            return String.format(Messages.get().SwitchForwardingDatabaseView_JobError, session.getObjectName(rootObject));
         }
      }.start();
	}
}
