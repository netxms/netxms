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
package org.netxms.nxmc.modules.agentmanagement.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.AgentTunnel;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.TunnelListComparator;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.TunnelListLabelProvider;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.TunnelManagerFilter;
import org.netxms.nxmc.modules.objects.dialogs.CreateNodeDialog;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Tunnel manager view
 */
public class TunnelManager extends ConfigurationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(TunnelManager.class);
   public static final String ID = "TunnelManager";
   
   public static final int COL_ID = 0;
   public static final int COL_STATE = 1;
   public static final int COL_NODE = 2;
   public static final int COL_IP_ADDRESS = 3;
   public static final int COL_CHANNELS = 4;
   public static final int COL_SYSNAME = 5;
   public static final int COL_HOSTNAME = 6;
   public static final int COL_PLATFORM = 7;
   public static final int COL_SYSINFO = 8;
   public static final int COL_HARDWARE_ID = 9;
   public static final int COL_AGENT_VERSION = 10;
   public static final int COL_AGENT_ID = 11;
   public static final int COL_AGENT_PROXY = 12;
   public static final int COL_SNMP_PROXY = 13;
   public static final int COL_SNMP_TRAP_PROXY = 14;
   public static final int COL_SYSLOG_PROXY = 15;
   public static final int COL_USER_AGENT = 16;
   public static final int COL_CERTIFICATE_EXPIRATION = 17;
   public static final int COL_CONNECTION_TIME = 18;
   
   private SortableTableViewer viewer;
   private TunnelManagerFilter filter;
   private Action actionCreateNode;
   private Action actionBind;
   private Action actionUnbind;
   private Action actionHideNonProxy;
   private Action actionHideNonUA;
   
   /**
    * Constructor
    */
   public TunnelManager()
   {
      super(i18n.tr("Agent Tunnels"), ResourceManager.getImageDescriptor("icons/config-views/tunnel_manager.png"), ID, true);
   }
   
   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {     
      final String[] names = 
         { i18n.tr("ID"), i18n.tr("State"), i18n.tr("Node"), i18n.tr("IP address"), i18n.tr("Channels"), i18n.tr("System name"), i18n.tr("Hostname"),
           i18n.tr("Platform"), i18n.tr("System information"), i18n.tr("Hardware ID"), i18n.tr("Agent version"), i18n.tr("Agent ID"),
           i18n.tr("Agent proxy"), i18n.tr("SNMP proxy"), i18n.tr("SNMP trap proxy"), i18n.tr("Syslog proxy"), i18n.tr("User agent"), 
           i18n.tr("Certificate expiration"), i18n.tr("Connection time") };
      final int[] widths = { 80, 80, 140, 150, 80, 150, 150, 250, 300, 180, 150, 150, 80, 80, 80, 80, 80, 130, 130 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new TunnelListLabelProvider());
      viewer.setComparator(new TunnelListComparator());
      filter = new TunnelManagerFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
      
      WidgetHelper.restoreTableViewerSettings(viewer, ID);
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, ID);
         }
      });
      
      createActions();
      createPopupMenu();
      
      refresh();
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
    * Create actions
    */
   private void createActions()
   {      
      actionCreateNode = new Action("&Create node and bind...") {
         @Override
         public void run()
         {
            createNode();
         }
      };
      
      actionBind = new Action("&Bind to...") {
         @Override
         public void run()
         {
            bindTunnel();
         }
      };
      
      actionUnbind = new Action("&Unbind") {
         @Override
         public void run()
         {
            unbindTunnel();
         }
      };

      actionHideNonProxy = new Action(i18n.tr("Hide tunnels without proxy function"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            filter.setHideNonProxy(actionHideNonProxy.isChecked());
         }
      };

      actionHideNonUA = new Action(i18n.tr("Hide tunnels without user agent"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            filter.setHideNonUA(actionHideNonUA.isChecked());
         }
      };
   }

   /**
    * Create pop-up menu for user list
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }
   
   /**
    * Fill context menu
    * @param manager Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection.size() == 1) && !((AgentTunnel)selection.getFirstElement()).isBound())
      {
         manager.add(actionBind);
         manager.add(actionCreateNode);
         manager.add(new Separator());
      }
      else
      {
         for(Object o : selection.toList())
         {
            if (((AgentTunnel)o).isBound())
            {
               manager.add(actionUnbind);
               manager.add(new Separator());
               break;
            }
         }
      }
      manager.add(actionHideNonProxy);
      manager.add(actionHideNonUA);
   }
   
   /**
    * Refresh view
    */
   public void refresh()
   {
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Get list of active agent tunnels"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<AgentTunnel> tunnels = session.getAgentTunnels();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(tunnels);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of active agent tunnels");
         }
      }.start();
   }
   
   /**
    * Create new node and bind tunnel
    */
   private void createNode()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;

      final AgentTunnel tunnel = (AgentTunnel)selection.getFirstElement();
      if (tunnel.isBound())
         return;

      CreateNodeDialog dlg = new CreateNodeDialog(getWindow().getShell(), null);
      dlg.setEnableShowAgainFlag(false);
      dlg.setObjectName(tunnel.getSystemName());
      dlg.setZoneUIN(tunnel.getZoneUIN());
      if (dlg.open() != Window.OK)
         return;

      final NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, dlg.getObjectName(), 2);
      cd.setCreationFlags(dlg.getCreationFlags());
      cd.setPrimaryName(dlg.getHostName());
      cd.setAgentPort(dlg.getAgentPort());
      cd.setSnmpPort(dlg.getSnmpPort());
      cd.setAgentProxyId(dlg.getAgentProxy());
      cd.setSnmpProxyId(dlg.getSnmpProxy());
      cd.setIcmpProxyId(dlg.getIcmpProxy());
      cd.setSshProxyId(dlg.getSshProxy());
      cd.setWebServiceProxyId(dlg.getWebServiceProxy());
      cd.setZoneUIN(dlg.getZoneUIN());
      cd.setSshLogin(dlg.getSshLogin());
      cd.setSshPassword(dlg.getSshPassword());
      cd.setSshPort(dlg.getSshPort());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Create new node and bind tunnel"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            // Always create node as unmanaged to start configuration poll only after tunnel bind
            boolean stayUnmanaged = ((cd.getCreationFlags() & NXCObjectCreationData.CF_CREATE_UNMANAGED) != 0);
            cd.setCreationFlags(cd.getCreationFlags() | NXCObjectCreationData.CF_CREATE_UNMANAGED);

            long nodeId = session.createObject(cd);
            session.bindAgentTunnel(tunnel.getId(), nodeId);

            if (!stayUnmanaged)
            {
               // Wait for tunnel to appear
               session.waitForAgentTunnel(nodeId, 20000);
               session.setObjectManaged(nodeId, true);
            }
            final List<AgentTunnel> tunnels = session.getAgentTunnels();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!viewer.getControl().isDisposed())
                     viewer.setInput(tunnels);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create node and bind tunnel");
         }
      }.start();
   }

   /**
    * Bind tunnel to node
    */
   private void bindTunnel()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      final AgentTunnel tunnel = (AgentTunnel)selection.getFirstElement();
      if (tunnel.isBound())
         return;
      
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell(), ObjectSelectionDialog.createNodeSelectionFilter(false));
      if (dlg.open() != Window.OK)
         return;      
      final long nodeId = dlg.getSelectedObjects().get(0).getObjectId();
      
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Bind tunnels"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.bindAgentTunnel(tunnel.getId(), nodeId);

            final List<AgentTunnel> tunnels = session.getAgentTunnels();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(tunnels);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot bind tunnel");
         }
      }.start();
   }

   /**
    * Unbind tunnel
    */
   private void unbindTunnel()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      
      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Unbind Tunnel"), i18n.tr("Selected tunnels will be unbound. Are you sure?")))
         return;
      
      final Object[] tunnels = selection.toArray();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Unbind tunnels"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : tunnels)
            {
               AgentTunnel t = (AgentTunnel)o;
               if (!t.isBound())
                  continue;
               session.unbindAgentTunnel(t.getNodeId());
            }

            final List<AgentTunnel> tunnels = session.getAgentTunnels();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(tunnels);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot unbind tunnel");
         }
      }.start();
   }
   
   @Override
   public boolean isModified()
   {
      return false;
   }

   @Override
   public void save()
   {
   }
}
