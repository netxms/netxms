/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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

import org.eclipse.core.commands.Command;
import org.eclipse.core.commands.State;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionProvider;
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
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.commands.ICommandService;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfaceListComparator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfaceListLabelProvider;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfacesTabFilter;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Interfaces" tab
 */
public class InterfacesTab extends ObjectTab
{
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
   public static final int COLUMN_ALIAS = 2;
	public static final int COLUMN_TYPE = 3;
	public static final int COLUMN_INDEX = 4;
	public static final int COLUMN_SLOT = 5;
	public static final int COLUMN_PORT = 6;
   public static final int COLUMN_MTU = 7;
   public static final int COLUMN_SPEED = 8;
	public static final int COLUMN_DESCRIPTION = 9;
	public static final int COLUMN_MAC_ADDRESS = 10;
	public static final int COLUMN_IP_ADDRESS = 11;
   public static final int COLUMN_VLAN = 12;
	public static final int COLUMN_PEER_NAME = 13;
	public static final int COLUMN_PEER_MAC_ADDRESS = 14;
	public static final int COLUMN_PEER_IP_ADDRESS = 15;
   public static final int COLUMN_PEER_PROTOCOL = 16;
	public static final int COLUMN_ADMIN_STATE = 17;
	public static final int COLUMN_OPER_STATE = 18;
	public static final int COLUMN_EXPECTED_STATE = 19;
	public static final int COLUMN_STATUS = 20;
	public static final int COLUMN_8021X_PAE_STATE = 21;
	public static final int COLUMN_8021X_BACKEND_STATE = 22;

	private SortableTableViewer viewer;
	private InterfaceListLabelProvider labelProvider;
	private SessionListener sessionListener = null;
	private Action actionCopyToClipboard;
	private Action actionCopyMacAddressToClipboard;
	private Action actionCopyIpAddressToClipboard;
	private Action actionCopyPeerNameToClipboard;
	private Action actionCopyPeerMacToClipboard;
	private Action actionCopyPeerIpToClipboard;
	private Action actionExportToCsv;
	
	private boolean showFilter = true;
	private boolean hideSubInterfaces = false;
	private FilterText filterText;
	private InterfacesTabFilter filter;
   private Composite interfacesArea;

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
	   final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      showFilter = safeCast(settings.get("InterfacesTab.showFilter"), settings.getBoolean("InterfacesTab.showFilter"), showFilter); //$NON-NLS-1$ //$NON-NLS-2$
      hideSubInterfaces = safeCast(settings.get("InterfacesTab.hideSubInterfaces"), settings.getBoolean("InterfacesTab.hideSubInterfaces"), hideSubInterfaces); //$NON-NLS-1$ //$NON-NLS-2$

      // Create interface area
      interfacesArea = new Composite(parent, SWT.BORDER);
      FormLayout formLayout = new FormLayout();
      interfacesArea.setLayout(formLayout);

      // Create filter
      filterText = new FilterText(interfacesArea, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            settings.put("InterfacesTab.showFilter", showFilter); //$NON-NLS-1$
            settings.put("InterfacesTab.hideSubInterfaces", hideSubInterfaces); //$NON-NLS-1$
         }
      });
      
      Action action = new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
            Command command = service.getCommand("org.netxms.ui.eclipse.objectview.commands.show_filter"); //$NON-NLS-1$
            State state = command.getState("org.netxms.ui.eclipse.objectview.commands.show_filter.state"); //$NON-NLS-1$
            state.setValue(false);
            service.refreshElements(command.getId(), null);
         }
      };
      setFilterCloseAction(action);

      // Check/uncheck menu items
      ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);

      Command command = service.getCommand("org.netxms.ui.eclipse.objectview.commands.show_filter"); //$NON-NLS-1$
      State state = command.getState("org.netxms.ui.eclipse.objectview.commands.show_filter.state"); //$NON-NLS-1$
      state.setValue(showFilter);
      service.refreshElements(command.getId(), null);

      command = service.getCommand("org.netxms.ui.eclipse.objectview.commands.hideSubInterfaces"); //$NON-NLS-1$
      state = command.getState("org.netxms.ui.eclipse.objectview.commands.hideSubInterfaces.state"); //$NON-NLS-1$
      state.setValue(hideSubInterfaces);
      service.refreshElements(command.getId(), null);
      
      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);
	   
		final String[] names = { 
	      Messages.get().InterfacesTab_ColId, 
	      Messages.get().InterfacesTab_ColName,
	      Messages.get().InterfacesTab_Alias,
	      Messages.get().InterfacesTab_ColIfType, 
	      Messages.get().InterfacesTab_ColIfIndex, 
	      Messages.get().InterfacesTab_ColSlot, 
	      Messages.get().InterfacesTab_ColPort,
	      Messages.get().InterfacesTab_MTU,
         Messages.get().InterfacesTab_Speed,
	      Messages.get().InterfacesTab_ColDescription, 
	      Messages.get().InterfacesTab_ColMacAddr,
	      Messages.get().InterfacesTab_ColIpAddr, 
	      "VLAN",
	      Messages.get().InterfacesTab_ColPeerNode, 
	      Messages.get().InterfacesTab_ColPeerMAC, 
	      Messages.get().InterfacesTab_ColPeerIP,
	      Messages.get().InterfacesTab_PeerDiscoveryProtocol,
	      Messages.get().InterfacesTab_ColAdminState, 
	      Messages.get().InterfacesTab_ColOperState, 
	      Messages.get().InterfacesTab_ColExpState, 
	      Messages.get().InterfacesTab_ColStatus, 
	      Messages.get().InterfacesTab_Col8021xPAE, 
	      Messages.get().InterfacesTab_Col8021xBackend 
		};
		
		final int[] widths = { 60, 150, 150, 150, 70, 70, 70, 70, 90, 150, 100, 90, 80, 150, 100, 90, 80, 80, 80, 80, 80, 80, 80 };
		viewer = new SortableTableViewer(interfacesArea, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		labelProvider = new InterfaceListLabelProvider();
		viewer.setLabelProvider(labelProvider);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setComparator(new InterfaceListComparator());
		viewer.getTable().setHeaderVisible(true);
		viewer.getTable().setLinesVisible(true);
		filter = new InterfacesTabFilter();
		filter.setHideSubInterfaces(hideSubInterfaces);
		viewer.addFilter(filter);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "InterfaceTable.V5"); //$NON-NLS-1$
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "InterfaceTable.V5"); //$NON-NLS-1$
			}
		});
		
      // Setup layout
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
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
      
      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_CHANGED)
            {
               AbstractObject object = (AbstractObject)n.getObject();
               if ((object != null) && (object instanceof Interface) && (getObject() != null) && object.isDirectChildOf(getObject().getObjectId()))
               {
                  viewer.getControl().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        refresh();
                     }
                  });
               }
            }
         }
      };
      ConsoleSharedData.getSession().addListener(sessionListener);
   }
	
	/* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#dispose()
    */
   @Override
   public void dispose()
   {
      ConsoleSharedData.getSession().removeListener(sessionListener);
      super.dispose();
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

   /**
    * @param action
    */
   private void setFilterCloseAction(Action action)
   {
      filterText.setCloseAction(action);
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
      interfacesArea.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText(""); //$NON-NLS-1$
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
		actionCopyToClipboard = new Action(Messages.get().InterfacesTab_ActionCopy, SharedIcons.COPY) {
			@Override
			public void run()
			{
				copyToClipboard(-1);
			}
		};	

		actionCopyMacAddressToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyMAC) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_MAC_ADDRESS);
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
				copyToClipboard(COLUMN_PEER_NAME);
			}
		};	

		actionCopyPeerMacToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyPeerMAC) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_PEER_MAC_ADDRESS);
			}
		};	

		actionCopyPeerIpToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyPeerIP) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_PEER_IP_ADDRESS);
			}
		};	
		
		actionExportToCsv = new ExportToCsvAction(getViewPart(), viewer, true);
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
			public void menuAboutToShow(IMenuManager manager)
			{
				fillContextMenu(manager);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		if (getViewPart() != null)
			getViewPart().getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
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

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#currentObjectUpdated(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void currentObjectUpdated(AbstractObject object)
	{
		objectChanged(object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
	 */
	@Override
	public void refresh()
	{
		if (getObject() != null)
			viewer.setInput(getObject().getAllChilds(AbstractObject.OBJECT_INTERFACE).toArray());
		else
			viewer.setInput(new Interface[0]);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void objectChanged(final AbstractObject object)
	{
		labelProvider.setNode((AbstractNode)object);
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean showForObject(AbstractObject object)
	{
		return (object instanceof AbstractNode);
	}

	/* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#getSelectionProvider()
    */
   @Override
   public ISelectionProvider getSelectionProvider()
   {
      return viewer;
   }

   /**
	 * Copy content to clipboard
	 * 
	 * @param column column number or -1 to copy all columns
	 */
	private void copyToClipboard(int column)
	{
		final TableItem[] selection = viewer.getTable().getSelection();
		if (selection.length > 0)
		{
			final String newLine = Platform.getOS().equals(Platform.OS_WIN32) ? "\r\n" : "\n"; //$NON-NLS-1$ //$NON-NLS-2$
			final StringBuilder sb = new StringBuilder();
			for(int i = 0; i < selection.length; i++)
			{
				if (i > 0)
					sb.append(newLine);
				if (column == -1)
				{
					for(int j = 0; j < viewer.getTable().getColumnCount(); j++)
					{
						if (j > 0)
							sb.append('\t');
						sb.append(selection[i].getText(j));
					}
				}
				else
				{
					sb.append(selection[i].getText(column));
				}
			}
			WidgetHelper.copyToClipboard(sb.toString());
		}
	}
	
	/* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      super.selected();
      refresh();
   }

   /**
    * @param hide
    */
   public void hideSubInterfaces(boolean hide)
   {
      hideSubInterfaces = hide;
      filter.setHideSubInterfaces(hide);
      viewer.refresh();
   }
}
