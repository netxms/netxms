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
package org.netxms.ui.eclipse.topology.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.topology.VlanInfo;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.Messages;
import org.netxms.ui.eclipse.topology.views.helpers.VlanLabelProvider;
import org.netxms.ui.eclipse.topology.widgets.DeviceView;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortInfo;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortSelectionListener;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Display VLAN configuration on a node
 */
public class VlanView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.VlanView"; //$NON-NLS-1$

	public static final int COLUMN_VLAN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_PORTS = 2;
   public static final int COLUMN_INTERFACES = 3;

	private long nodeId;
	private List<VlanInfo> vlans = new ArrayList<VlanInfo>(0);
	private NXCSession session;
	private SortableTableViewer vlanList;
	private ScrolledComposite scroller;
	private DeviceView deviceView;
	private boolean refreshOnStartup = false;

	private Action actionRefresh; 
	private Action actionShowVlanMap;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		nodeId = Long.parseLong(site.getSecondaryId());
		session = (NXCSession)ConsoleSharedData.getSession();
		AbstractObject object = session.findObjectById(nodeId);
		setPartName(String.format(Messages.get().VlanView_PartName, (object != null) ? object.getObjectName() : "<" + site.getSecondaryId() + ">")); //$NON-NLS-1$ //$NON-NLS-2$
	}

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
    */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		super.init(site, memento);
		if (memento != null)
			refreshOnStartup = true;
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createPartControl(Composite parent)
	{
      SashForm splitter = new SashForm(parent, SWT.VERTICAL);
      splitter.setSashWidth(3);

      Composite vlanListArea = new Composite(splitter, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      vlanListArea.setLayout(layout);

		final String[] names = { Messages.get().VlanView_ColumnID, Messages.get().VlanView_ColumnName, Messages.get().VlanView_ColumnPorts, "Interfaces" };
		final int[] widths = { 80, 180, 400, 400 };
      vlanList = new SortableTableViewer(vlanListArea, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		vlanList.setContentProvider(new ArrayContentProvider());
		final VlanLabelProvider labelProvider = new VlanLabelProvider();
		vlanList.setLabelProvider(labelProvider);
      vlanList.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		vlanList.setInput(vlans.toArray());
		vlanList.addSelectionChangedListener(new ISelectionChangedListener() {
		   @Override
		   public void selectionChanged(SelectionChangedEvent event)
		   {
            IStructuredSelection selection = vlanList.getStructuredSelection();

		      VlanInfo vlan = (VlanInfo)selection.getFirstElement();
		      if (vlan != null)
		      {
		         deviceView.setHighlight(vlan.getPorts());
		         actionShowVlanMap.setEnabled(true);
		      }
		      else
		      {
		         deviceView.clearHighlight(true);
		         actionShowVlanMap.setEnabled(false);
		      }
            
		      if (labelProvider.setSelectedPort(null))
            {
               vlanList.refresh();
            }
		   }
		});

		WidgetHelper.restoreTableViewerSettings(vlanList, Activator.getDefault().getDialogSettings(), "VlanList");
		vlanList.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(vlanList, Activator.getDefault().getDialogSettings(), "VlanList");
         }
      });

      new Label(vlanListArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Composite deviceViewArea = new Composite(splitter, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      deviceViewArea.setLayout(layout);

      new Label(deviceViewArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

		scroller = new ScrolledComposite(deviceViewArea, SWT.H_SCROLL | SWT.V_SCROLL);
      scroller.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		deviceView = new DeviceView(scroller, SWT.NONE);
		deviceView.setPortStatusVisible(false);
		deviceView.setNodeId(nodeId);
		deviceView.addSelectionListener(new PortSelectionListener() {
         @Override
         public void portSelected(PortInfo port)
         {
            vlanList.setSelection(null);
            labelProvider.setSelectedPort(port);
            vlanList.refresh();
         }
      });

		scroller.setContent(deviceView);
      scroller.setBackground(deviceView.getBackground());
		scroller.setExpandVertical(true);
		scroller.setExpandHorizontal(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.HORIZONTAL, 20);
		scroller.addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				scroller.setMinSize(deviceView.computeSize(SWT.DEFAULT, SWT.DEFAULT));
			}
		});

      splitter.setWeights(new int[] { 60, 40 });

		createActions();
		contributeToActionBars();
		createPopupMenu();

		if (refreshOnStartup)
			refreshVlanList();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refreshVlanList();
			}
		};
		
		actionShowVlanMap = new Action(Messages.get().VlanView_ShowVlanMap) {
			@Override
			public void run()
			{
				showVlanMap();
			}
		};
		
		actionExportToCsv = new ExportToCsvAction(this, vlanList, true);
		actionExportAllToCsv = new ExportToCsvAction(this, vlanList, false);
	}

	/**
	 * Contribute actions to action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pulldown menu
	 * @param manager menu manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionShowVlanMap);
		manager.add(actionExportAllToCsv);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local toolbar
	 * @param manager menu manager
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
		Menu menu = menuMgr.createContextMenu(vlanList.getControl());
		vlanList.getControl().setMenu(menu);

		// Register menu for extension.
      getSite().setSelectionProvider(vlanList);
		getSite().registerContextMenu(menuMgr, vlanList);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionShowVlanMap);
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
	@Override
	public void setFocus()
	{
		vlanList.getControl().setFocus();
	}

	/**
	 * @param vlans the vlans to set
	 */
	public void setVlans(List<VlanInfo> vlans)
	{
		this.vlans = vlans;
		vlanList.setInput(vlans.toArray());
	}

	/**
	 * Show map for currently selected VLAN
	 */
	private void showVlanMap()
	{
      IStructuredSelection selection = vlanList.getStructuredSelection();
		IWorkbenchPage page = getSite().getWorkbenchWindow().getActivePage();
		for(final Object o : selection.toList())
		{
			final VlanInfo vlan = (VlanInfo)o;
			try
			{
				page.showView("org.netxms.ui.eclipse.networkmaps.views.VlanMap", Long.toString(nodeId) + "&" + Integer.toString(vlan.getVlanId()), IWorkbenchPage.VIEW_ACTIVATE); //$NON-NLS-1$ //$NON-NLS-2$
			}
			catch(PartInitException e)
			{
				MessageDialogHelper.openError(getSite().getShell(), Messages.get().VlanView_Error, String.format(Messages.get().VlanView_OpenMapError, vlan.getVlanId(), e.getLocalizedMessage()));
			}
		}
	}

	/**
	 * Reload list of VLANs from server and update table view
	 */
	private void refreshVlanList()
	{
      final boolean syncChildren = !ConsoleSharedData.getSettings().getBoolean("ObjectsFullSync") && !session.areChildrenSynchronized(nodeId);
		new ConsoleJob(Messages.get().VlanView_JobTitle, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            if (syncChildren)
            {
               AbstractObject nodeObject = session.findObjectById(nodeId);
               if (nodeObject != null)
               {
                  session.syncChildren(nodeObject);
                  runInUIThread(() -> {
                     deviceView.refresh();
                  });
               }
            }
				final List<VlanInfo> vlans = session.getVlans(nodeId);
            runInUIThread(() -> {
               setVlans(vlans);
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().VlanView_JobError;
			}
		}.start();
	}
}
