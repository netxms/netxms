/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.commands.Command;
import org.eclipse.core.commands.State;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.commands.ICommandService;
import org.netxms.base.MacAddress;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfaceListComparator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfaceListLabelProvider;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfacesTabFilter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Interfaces" tab
 */
public class InterfacesTab extends NodeComponentViewerTab
{
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_ALIAS = 2;
   public static final int COLUMN_INDEX = 3;
   public static final int COLUMN_DESCRIPTION = 4;
   public static final int COLUMN_IP_ADDRESS = 5;
   public static final int COLUMN_MAC_ADDRESS = 6;
   public static final int COLUMN_NIC_VENDOR = 7;
   public static final int COLUMN_VLAN = 8;
   public static final int COLUMN_MTU = 9;
   public static final int COLUMN_SPEED = 10;
   public static final int COLUMN_TYPE = 11;
   public static final int COLUMN_PHYSICAL_LOCATION = 12;
   public static final int COLUMN_ADMIN_STATE = 13;
   public static final int COLUMN_OPER_STATE = 14;
   public static final int COLUMN_EXPECTED_STATE = 15;
   public static final int COLUMN_STP_STATE = 16;
   public static final int COLUMN_STATUS = 17;
   public static final int COLUMN_OSPF_AREA = 18;
   public static final int COLUMN_OSPF_TYPE = 19;
   public static final int COLUMN_OSPF_STATE = 20;
   public static final int COLUMN_8021X_PAE_STATE = 21;
   public static final int COLUMN_8021X_BACKEND_STATE = 22;
   public static final int COLUMN_PEER_NODE = 23;
   public static final int COLUMN_PEER_INTERFACE = 24;
   public static final int COLUMN_PEER_MAC_ADDRESS = 25;
   public static final int COLUMN_PEER_NIC_VENDOR = 26;
   public static final int COLUMN_PEER_IP_ADDRESS = 27;
   public static final int COLUMN_PEER_PROTOCOL = 28;

   private InterfaceListLabelProvider labelProvider;
   private Action actionCopyMacAddressToClipboard;
   private Action actionCopyIpAddressToClipboard;
   private Action actionCopyPeerNameToClipboard;
   private Action actionCopyPeerMacToClipboard;
   private Action actionCopyPeerIpToClipboard;

   private boolean hideSubInterfaces = false;

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected void createTabContent(Composite parent)
	{
	   super.createTabContent(parent);
	   
	   final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      hideSubInterfaces = safeCast(settings.get("InterfacesTab.hideSubInterfaces"), settings.getBoolean("InterfacesTab.hideSubInterfaces"), hideSubInterfaces); //$NON-NLS-1$ //$NON-NLS-2$

      filterText.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            settings.put("InterfacesTab.hideSubInterfaces", hideSubInterfaces); //$NON-NLS-1$
         }
      });

      // Check/uncheck menu items
      ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);

      Command command = service.getCommand("org.netxms.ui.eclipse.objectview.commands.hideSubInterfaces"); //$NON-NLS-1$
      State state = command.getState("org.netxms.ui.eclipse.objectview.commands.hideSubInterfaces.state"); //$NON-NLS-1$
      state.setValue(hideSubInterfaces);
      service.refreshElements(command.getId(), null);
   }

	/**
	 * Create actions
	 */
	protected void createActions()
	{
	   super.createActions();

		actionCopyMacAddressToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyMAC) {
			@Override
			public void run()
			{
			   copyMacAddress(false);
			}
		};	

		actionCopyIpAddressToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyIP) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_IP_ADDRESS);
			}
		};	

		actionCopyPeerNameToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyPeerName) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_PEER_NODE);
			}
		};	

		actionCopyPeerMacToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyPeerMAC) {
			@Override
			public void run()
			{
            copyMacAddress(true);
			}
		};	

		actionCopyPeerIpToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyPeerIP) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_PEER_IP_ADDRESS);
			}
		};	
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	@Override
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionCopyToClipboard);
		manager.add(actionCopyMacAddressToClipboard);
		manager.add(actionCopyIpAddressToClipboard);
		manager.add(actionCopyPeerNameToClipboard);
		manager.add(actionCopyPeerMacToClipboard);
		manager.add(actionCopyPeerIpToClipboard);
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		ObjectContextMenu.fill(manager, getViewPart().getSite(), viewer);
	}

   /**
    * Copy mac address
    */
   private void copyMacAddress(boolean peerMacAddress)
   {
      @SuppressWarnings("unchecked")
      final List<Interface> selection = viewer.getStructuredSelection().toList();
      if (selection.size() > 0)
      {
         final String newLine = WidgetHelper.getNewLineCharacters();
         final StringBuilder sb = new StringBuilder();
         for(int i = 0; i < selection.size(); i++)
         {
            if (i > 0)
               sb.append(newLine);

            MacAddress addr = null;
            if (peerMacAddress)
            {
               addr = labelProvider.getPeerMacAddress(selection.get(i));
            }
            else
            {
               addr = selection.get(i).getMacAddress();
            }
            sb.append(addr != null ? addr.toString() : "");
         }
         WidgetHelper.copyToClipboard(sb.toString());
      }      
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
	@Override
	public void refresh()
	{
		if (getObject() != null)
      {
			viewer.setInput(getObject().getAllChildren(AbstractObject.OBJECT_INTERFACE).toArray());
         viewer.packColumns();
      }
		else
      {
			viewer.setInput(new Interface[0]);
      }
	}

   /**
    * @param hide
    */
   public void hideSubInterfaces(boolean hide)
   {
      hideSubInterfaces = hide;
      ((InterfacesTabFilter)filter).setHideSubInterfaces(hide);
      viewer.refresh();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.NodeComponentViewerTab#createViewer()
    */
   @Override
   protected void createViewer()
   {
      final String[] names = { 
         Messages.get().InterfacesTab_ColId, 
         Messages.get().InterfacesTab_ColName,
         Messages.get().InterfacesTab_Alias,
         Messages.get().InterfacesTab_ColIfIndex, 
         Messages.get().InterfacesTab_ColDescription, 
         Messages.get().InterfacesTab_ColIpAddr, 
         Messages.get().InterfacesTab_ColMacAddr,
         "NIC vendor",
         "VLAN",
         Messages.get().InterfacesTab_MTU,
         Messages.get().InterfacesTab_Speed,
         Messages.get().InterfacesTab_ColIfType, 
         "Location",
         Messages.get().InterfacesTab_ColAdminState, 
         Messages.get().InterfacesTab_ColOperState, 
         Messages.get().InterfacesTab_ColExpState, 
         "STP state",
         Messages.get().InterfacesTab_ColStatus, 
         "OSPF area",
         "OSPF type",
         "OSPF state",
         Messages.get().InterfacesTab_Col8021xPAE, 
         Messages.get().InterfacesTab_Col8021xBackend, 
         Messages.get().InterfacesTab_ColPeerNode,
         "Peer interface",
         Messages.get().InterfacesTab_ColPeerMAC, 
         "Peer NIC vendor",
         Messages.get().InterfacesTab_ColPeerIP,
         Messages.get().InterfacesTab_PeerDiscoveryProtocol
      };
      final int[] widths = { 60, 150, 150, 70, 150, 100, 70, 90, 150, 100, 90, 200, 80, 80, 80, 80, 150, 150, 100, 120, 90, 80, 80, 80, 80, 80, 80, 80, 80 };
      viewer = new SortableTableViewer(mainArea, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      labelProvider = new InterfaceListLabelProvider(viewer);
      viewer.setLabelProvider(labelProvider);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new InterfaceListComparator());
      viewer.getTable().setHeaderVisible(true);
      viewer.getTable().setLinesVisible(true);
      filter = new InterfacesTabFilter();
      ((InterfacesTabFilter)filter).setHideSubInterfaces(hideSubInterfaces);
      viewer.addFilter(filter);
      WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "InterfaceTable.V8"); //$NON-NLS-1$
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "InterfaceTable.V8"); //$NON-NLS-1$
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText, 0, SWT.BOTTOM);
      fd.bottom = new FormAttachment(100, 0);
      fd.right = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public void objectChanged(final AbstractObject object)
   {
      labelProvider.setNode((AbstractNode)object);
      super.objectChanged(object);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.NodeComponentViewerTab#getFilterSettingName()
    */
   @Override
   public String getFilterSettingName()
   {
      return "InterfacesTab.showFilter";
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.NodeComponentViewerTab#needRefreshOnObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean needRefreshOnObjectChange(AbstractObject object)
   {
      return object instanceof Interface;
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.NodeComponentTab#syncAdditionalObjects(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void syncAdditionalObjects(AbstractObject object) throws IOException, NXCException
   {
      List<Long> additionalSyncInterfaces = new ArrayList<Long>();
      for(AbstractObject obj : object.getAllChildren(AbstractObject.OBJECT_INTERFACE))
      {
         long id = ((Interface)obj).getPeerInterfaceId();
         if (id != 0)
            additionalSyncInterfaces.add(id);
      }

      if (!additionalSyncInterfaces.isEmpty())
         session.syncMissingObjects(additionalSyncInterfaces, NXCSession.OBJECT_SYNC_WAIT);
   }
}
