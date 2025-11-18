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
package org.netxms.nxmc.modules.objects.views;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.VlanInfo;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.networkmaps.views.VlanMapView;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.views.helpers.VlanLabelProvider;
import org.netxms.nxmc.modules.objects.widgets.PortViewWidget;
import org.netxms.nxmc.modules.objects.widgets.SlotViewWidget;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortInfo;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortSelectionListener;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Ports" view
 */
public class PortView extends NodeSubObjectView implements ISelectionProvider
{
   private static enum DisplayMode
   {
      NONE, STATE, STATUS, VLAN
   }

   private final I18n i18n = LocalizationHelper.getI18n(PortView.class);

   public static final int COLUMN_VLAN_ID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_PORTS = 2;
   public static final int COLUMN_INTERFACES = 3;
   
   private List<VlanInfo> vlans = new ArrayList<VlanInfo>(0);
   private VlanLabelProvider labelProvider;
   private DisplayMode displayMode = DisplayMode.NONE;
   private Button buttonState;
   private Button buttonStatus;
   private Button buttonVlan;
   private SortableTableViewer vlanList;
   private boolean objectsFullySync;
   private Action actionShowVlanMap;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
	private ScrolledComposite scroller;
   private PortViewWidget portView;
	private ISelection selection = new StructuredSelection();
	private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();

   /**
    * Create "Ports" view
    */
   public PortView()
   {
      super(LocalizationHelper.getI18n(PortView.class).tr("Ports"), ResourceManager.getImageDescriptor("icons/object-views/ports.png"), "objects.ports", false);
      PreferenceStore store = PreferenceStore.getInstance();
      objectsFullySync = store.getAsBoolean("ObjectBrowser.FullSync", false);
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
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 90;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      scroller.setMinSize(portView.computeSize(SWT.DEFAULT, SWT.DEFAULT));
      setVlans(new ArrayList<VlanInfo>());
      if (object != null)
      {
         portView.setNodeId((object != null) ? object.getObjectId() : 0);
         refresh();
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   protected void createContent(Composite parent)
	{
      super.createContent(parent);
      
     // Composite area = new Composite(mainArea, SWT.NONE);

      mainArea.setLayout(new GridLayout());
      
      Composite controlBar = new Composite(mainArea, SWT.NONE);
      controlBar.setBackground(mainArea.getBackground());
      controlBar.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      controlBar.setLayout(new RowLayout(SWT.HORIZONTAL));

      buttonState = new Button(controlBar, SWT.TOGGLE | SWT.FLAT);
      buttonState.setText(i18n.tr("State"));
      buttonState.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            buttonStatus.setSelection(false);
            buttonVlan.setSelection(false);
            switchMode(DisplayMode.STATE);
         }
      });
      buttonState.setSelection(true);
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonState.setLayoutData(rd);

      buttonStatus = new Button(controlBar, SWT.TOGGLE | SWT.FLAT);
      buttonStatus.setText(i18n.tr("Status"));
      buttonStatus.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            buttonState.setSelection(false);
            buttonVlan.setSelection(false);
            switchMode(DisplayMode.STATUS);
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonStatus.setLayoutData(rd);

      buttonVlan = new Button(controlBar, SWT.TOGGLE | SWT.FLAT);
      buttonVlan.setText(i18n.tr("Vlans"));
      buttonVlan.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            buttonState.setSelection(false);
            buttonStatus.setSelection(false);
            switchMode(DisplayMode.VLAN);
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonVlan.setLayoutData(rd);

      SashForm splitter = new SashForm(mainArea, SWT.VERTICAL);
      splitter.setSashWidth(3);
      splitter.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      Composite deviceViewArea = new Composite(splitter, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      deviceViewArea.setLayout(layout);

      scroller = new ScrolledComposite(deviceViewArea, SWT.H_SCROLL | SWT.V_SCROLL);
      scroller.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      portView = new PortViewWidget(scroller, SWT.NONE);
      portView.setPortDisplayMode(SlotViewWidget.DISPLAY_MODE_STATE);
      portView.setNodeId(getObjectId());
      portView.addSelectionListener(new PortSelectionListener() {
          @Override
          public void portSelected(PortInfo port)
          {
             if (port != null)
             {
               vlanList.setSelection(null);
               labelProvider.setSelectedPort(port);
               vlanList.refresh();
                Interface iface = session.findObjectById(port.getInterfaceObjectId(), Interface.class);
                if (iface != null)
                {
                   selection = new StructuredSelection(iface);
                }
                else
                {
                   selection = new StructuredSelection();
                }
             }
             else
             {
                selection = new StructuredSelection();
             }
             
             for(ISelectionChangedListener listener : selectionListeners)
                listener.selectionChanged(new SelectionChangedEvent(PortView.this, selection));
          }
      });
       

      scroller.setContent(portView);
      scroller.setBackground(portView.getBackground());
      scroller.setExpandVertical(true);
      scroller.setExpandHorizontal(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.HORIZONTAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            scroller.setMinSize(portView.computeSize(SWT.DEFAULT, SWT.DEFAULT));
         }
      });

      new Label(deviceViewArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      
      Composite vlanListArea = new Composite(splitter, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      vlanListArea.setLayout(layout);

      final String[] names = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Ports"), "Interfaces" };
      final int[] widths = { 80, 180, 400, 400 };
      vlanList = new SortableTableViewer(vlanListArea, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
      vlanList.setContentProvider(new ArrayContentProvider());      
      labelProvider = new VlanLabelProvider();
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
               portView.setHighlight(vlan.getPorts());
               actionShowVlanMap.setEnabled(true);
            }
            else
            {
               portView.clearHighlight(true);
               actionShowVlanMap.setEnabled(false);
            }

            if (labelProvider.setSelectedPort(null))
            {
               vlanList.refresh();
            }
         }
      });

      WidgetHelper.restoreTableViewerSettings(vlanList, "VlanList");
      vlanList.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(vlanList, "VlanList");
         }
      });
      
      new Label(vlanListArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      
      splitter.setWeights(new int[] { 60, 40 });
      
		createContextMenu();
      createActions();
      createPopupMenu();
	}

   /**
    * Create actions
    */
   private void createActions()
   {     
      actionShowVlanMap = new Action(i18n.tr("Show VLAN &map")) {
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
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportAllToCsv);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportAllToCsv);
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
   }

   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionShowVlanMap);
      manager.add(actionExportToCsv);
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
      for(final Object o : selection.toList())
      {
         final VlanInfo vlan = (VlanInfo)o;
         openView(new VlanMapView(getObject(), vlan.getVlanId()));
      }
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
   @Override
   public void refresh()
   {
      portView.setNodeId(getObjectId());
      scroller.setMinSize(portView.computeSize(SWT.DEFAULT, SWT.DEFAULT));
      labelProvider.setSelectedPort(null);

      final String objectName = session.getObjectName(getObjectId());
      final boolean syncChildren = !objectsFullySync && !session.areChildrenSynchronized(getObjectId());
      new Job(i18n.tr("Reading VLAN list from node"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (syncChildren)
            {
               AbstractObject nodeObject = session.findObjectById(getObjectId());
               if (nodeObject != null)
               {
                  session.syncChildren(nodeObject);
                  runInUIThread(() -> {
                     portView.refresh();
                  });
               }
            }
            final List<VlanInfo> vlans = session.getVlans(getObjectId());
            runInUIThread(() -> {
               setVlans(vlans);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot get VLAN list from node %s"), objectName);
         }
      }.start();
   }

   /**
    * Switch display mode
    *
    * @param newMode new display mode
    */
   private void switchMode(DisplayMode newMode)
   {
      if (newMode == displayMode)
         return;

      displayMode = newMode;
      switch(displayMode)
      {
         case STATE:
            portView.setPortDisplayMode(SlotViewWidget.DISPLAY_MODE_STATE);
            break;
         case VLAN:
            portView.setPortDisplayMode(SlotViewWidget.DISPLAY_MODE_NONE);
            break;
         default:
            portView.setPortDisplayMode(SlotViewWidget.DISPLAY_MODE_STATUS);
            break;
      }

      refresh();
   }

	/**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#needRefreshOnObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean needRefreshOnObjectChange(AbstractObject object)
   {
      return (object instanceof Interface) && object.isDirectChildOf(getObjectId());
   }

   /**
    * Create pop-up menu
    */
	private void createContextMenu()
	{
		// Create menu manager.
      MenuManager menuMgr = new ObjectContextMenuManager(this, this, null);
      portView.setMenu(menuMgr.createContextMenu(portView));
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
	@Override
	public void addSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.add(listener);
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
    */
	@Override
	public ISelection getSelection()
	{
		return selection;
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
	@Override
	public void removeSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.remove(listener);
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
	@Override
	public void setSelection(ISelection selection)
	{
		this.selection = selection;
	}
}
