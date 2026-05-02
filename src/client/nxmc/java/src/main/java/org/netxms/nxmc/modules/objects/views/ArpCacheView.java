/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.base.MacAddress;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.ArpCacheEntry;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.CopyTableRowsAction;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.CreateMultipleNodesDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateNodeDialog;
import org.netxms.nxmc.modules.objects.views.helpers.ArpCacheComparator;
import org.netxms.nxmc.modules.objects.views.helpers.ArpCacheFilter;
import org.netxms.nxmc.modules.objects.views.helpers.ArpCacheLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * ARP cache view
 */
public class ArpCacheView extends ObjectView
{
   private static final Logger logger = LoggerFactory.getLogger(ArpCacheView.class);

   private final I18n i18n = LocalizationHelper.getI18n(ArpCacheView.class);

   public static final int COLUMN_IP_ADDRESS = 0;
   public static final int COLUMN_MAC_ADDRESS = 1;
   public static final int COLUMN_VENDOR = 2;
	public static final int COLUMN_INTERFACE = 3;
   public static final int COLUMN_NODE = 4;

	private SortableTableViewer viewer;
   private boolean refreshPending = true;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionCopyRowToClipboard;
   private Action actionCopyMACToClipboard;
   private Action actionCreateNode;

   /**
    * Default constructor
    */
   public ArpCacheView()
   {
      super(LocalizationHelper.getI18n(ArpCacheView.class).tr("ARP Cache"), ResourceManager.getImageDescriptor("icons/object-views/fdb.gif"), "objects.arp-cache", true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && (((Node)context).hasAgent() || ((Node)context).hasSnmpAgent() || ((Node)context).isManagementServer());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 155;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{	   
		final String[] names = { 
            i18n.tr("IP Address"), i18n.tr("MAC Address"), i18n.tr("NIC Vendor"), i18n.tr("Interface"), i18n.tr("Node")
		   };
      final int[] widths = { 180, 180, 200, 200, 250 };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_IP_ADDRESS, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ArpCacheLabelProvider(viewer));
      viewer.setComparator(new ArpCacheComparator());
      ArpCacheFilter filter = new ArpCacheFilter();
		setFilterClient(viewer, filter);
		viewer.addFilter(filter);

		createActions();
		createPopupMenu();
	}

	/**
	 * Create actions
	 */
   private void createActions()
	{
      actionCopyRowToClipboard = new CopyTableRowsAction(viewer, true);
      actionCopyMACToClipboard = new Action(i18n.tr("Copy MAC address to clipboard")) {
         @Override
         public void run()
         {
            @SuppressWarnings("unchecked")
            final List<ArpCacheEntry> selection = viewer.getStructuredSelection().toList();
            if (selection.size() > 0)
            {
               final StringBuilder sb = new StringBuilder();
               for(int i = 0; i < selection.size(); i++)
               {
                  if (i > 0)
                     sb.append('\t');

                  MacAddress addr = selection.get(i).getMacAddress();
                  sb.append(addr != null ? addr.toString() : "");
               }
               WidgetHelper.copyToClipboard(sb.toString());
            }
         }
      };
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);

      actionCreateNode = new Action(i18n.tr("&Create node..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNodesFromSelection();
         }
      };
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
      @SuppressWarnings("unchecked")
      List<ArpCacheEntry> selection = viewer.getStructuredSelection().toList();
      if (hasUnknownNode(selection))
      {
         actionCreateNode.setText((countUnknownNodes(selection) > 1) ? i18n.tr("&Create nodes...") : i18n.tr("&Create node..."));
         manager.add(actionCreateNode);
         manager.add(new Separator());
      }
      manager.add(actionCopyRowToClipboard);
      manager.add(actionCopyMACToClipboard);
      manager.add(actionExportToCsv);
      manager.add(actionExportAllToCsv);
	}

   /**
    * Check if selection contains at least one entry with no associated node.
    *
    * @param selection list of selected ARP cache entries
    * @return true if any entry has node id == 0
    */
   private static boolean hasUnknownNode(List<ArpCacheEntry> selection)
   {
      for(ArpCacheEntry e : selection)
      {
         if (e.getNodeId() == 0)
            return true;
      }
      return false;
   }

   /**
    * Count entries with no associated node.
    *
    * @param selection list of selected ARP cache entries
    * @return number of entries with node id == 0
    */
   private static int countUnknownNodes(List<ArpCacheEntry> selection)
   {
      int count = 0;
      for(ArpCacheEntry e : selection)
      {
         if (e.getNodeId() == 0)
            count++;
      }
      return count;
   }

   /**
    * Find first non-Subnet parent of given object, or 0 if none found.
    *
    * @param object source object
    * @return parent object id suitable as a container, or 0
    */
   private long findContainerParent(AbstractObject object)
   {
      NXCSession session = Registry.getSession();
      Iterator<Long> it = object.getParents();
      while(it.hasNext())
      {
         long id = it.next();
         AbstractObject parent = session.findObjectById(id);
         if ((parent != null) && ((parent.getObjectClass() == AbstractObject.OBJECT_CONTAINER) || (parent.getObjectClass() == AbstractObject.OBJECT_COLLECTOR) ||
               (parent.getObjectClass() == AbstractObject.OBJECT_SERVICEROOT)))
            return id;
      }
      return 0;
   }

   /**
    * Create node objects from the currently selected ARP cache entries.
    * Entries that already reference an existing node are skipped.
    */
   private void createNodesFromSelection()
   {
      @SuppressWarnings("unchecked")
      List<ArpCacheEntry> selection = viewer.getStructuredSelection().toList();
      List<ArpCacheEntry> targets = new ArrayList<ArpCacheEntry>(selection.size());
      for(ArpCacheEntry e : selection)
      {
         if ((e.getNodeId() == 0) && (e.getIpAddress() != null))
            targets.add(e);
      }
      if (targets.isEmpty())
         return;

      AbstractObject contextObject = getObject();
      long defaultParentId = (contextObject != null) ? findContainerParent(contextObject) : 0;
      int defaultZoneUIN = (contextObject instanceof Node) ? ((Node)contextObject).getZoneId() : 0;

      if (targets.size() == 1)
      {
         createSingleNode(targets.get(0), defaultParentId, defaultZoneUIN);
      }
      else
      {
         createMultipleNodes(targets, defaultParentId, defaultZoneUIN);
      }
   }

   /**
    * Create a single node using the full create node dialog, pre-filled from the ARP entry.
    *
    * @param entry ARP cache entry
    * @param defaultParentId initial parent id for the dialog
    * @param defaultZoneUIN initial zone UIN for the dialog
    */
   private void createSingleNode(ArpCacheEntry entry, long defaultParentId, int defaultZoneUIN)
   {
      String ipAddress = entry.getIpAddress().getHostAddress();
      CreateNodeDialog dlg = new CreateNodeDialog(getWindow().getShell(), null);
      dlg.setObjectName(ipAddress);
      dlg.setEnableShowAgainFlag(false);
      dlg.setZoneUIN(defaultZoneUIN);
      if (dlg.open() != Window.OK)
         return;

      final NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, dlg.getObjectName(), defaultParentId);
      cd.setCreationFlags(dlg.getCreationFlags());
      cd.setPrimaryName(dlg.getHostName());
      cd.setObjectAlias(dlg.getObjectAlias());
      cd.setAgentPort(dlg.getAgentPort());
      cd.setAgentProxyId(dlg.getAgentProxy());
      cd.setSnmpPort(dlg.getSnmpPort());
      cd.setSnmpProxyId(dlg.getSnmpProxy());
      cd.setEtherNetIpPort(dlg.getEtherNetIpPort());
      cd.setEtherNetIpProxyId(dlg.getEtherNetIpProxy());
      cd.setModbusTcpPort(dlg.getModbusTcpPort());
      cd.setModbusUnitId(dlg.getModbusUnitId());
      cd.setModbusProxyId(dlg.getModbusProxy());
      cd.setIcmpProxyId(dlg.getIcmpProxy());
      cd.setWebServiceProxyId(dlg.getWebServiceProxy());
      cd.setSshPort(dlg.getSshPort());
      cd.setSshProxyId(dlg.getSshProxy());
      cd.setSshLogin(dlg.getSshLogin());
      cd.setSshPassword(dlg.getSshPassword());
      cd.setMqttProxyId(dlg.getMqttProxy());
      cd.setZoneUIN(dlg.getZoneUIN());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Creating node"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.createObject(cd);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot create node object %s"), cd.getName());
         }
      }.start();
   }

   /**
    * Create multiple node objects with default settings using the lightweight batch dialog.
    *
    * @param entries ARP cache entries to create nodes for
    * @param defaultParentId initial parent id for the dialog
    * @param defaultZoneUIN initial zone UIN for the dialog
    */
   private void createMultipleNodes(final List<ArpCacheEntry> entries, long defaultParentId, int defaultZoneUIN)
   {
      CreateMultipleNodesDialog dlg = new CreateMultipleNodesDialog(getWindow().getShell(), defaultParentId, defaultZoneUIN);
      if (dlg.open() != Window.OK)
         return;

      final long parentId = dlg.getParentId();
      final int zoneUIN = dlg.getZoneUIN();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Creating nodes"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(ArpCacheEntry entry : entries)
            {
               String ipAddress = entry.getIpAddress().getHostAddress();
               NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, ipAddress, parentId);
               cd.setPrimaryName(ipAddress);
               cd.setZoneUIN(zoneUIN);
               try
               {
                  session.createObject(cd);
               }
               catch(Exception e)
               {
                  logger.error("Cannot create node object " + ipAddress, e);
               }
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create node objects");
         }
      }.start();
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
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportAllToCsv);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
	@Override
   public void refresh()
	{
      final long objectId = getObjectId();
      if (objectId == 0)
         return;

      new Job(i18n.tr("Reading ARP cache"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<ArpCacheEntry> fdb = session.getArpCache(objectId, true);
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
            return String.format(i18n.tr("Cannot get ARP cache for node %s"), session.getObjectName(objectId));
         }
      }.start();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      if (refreshPending)
      {
         refreshPending = false;
         refresh();
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      viewer.setInput(new Object[0]);
      if (isActive())
         refresh();
      else
         refreshPending = true;
   }
}
