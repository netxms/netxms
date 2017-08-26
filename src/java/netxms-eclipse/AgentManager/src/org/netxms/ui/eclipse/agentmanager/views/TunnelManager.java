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
package org.netxms.ui.eclipse.agentmanager.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.AgentTunnel;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.views.helpers.TunnelListComparator;
import org.netxms.ui.eclipse.agentmanager.views.helpers.TunnelListLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectmanager.dialogs.CreateNodeDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Tunnel manager view
 */
public class TunnelManager extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.TunnelManager";
   
   public static final int COL_ID = 0;
   public static final int COL_STATE = 1;
   public static final int COL_NODE = 2;
   public static final int COL_IP_ADDRESS = 3;
   public static final int COL_CHANNELS = 4;
   public static final int COL_SYSNAME = 5;
   public static final int COL_PLATFORM = 6;
   public static final int COL_SYSINFO = 7;
   public static final int COL_AGENT_VERSION = 8;
   
   private SortableTableViewer viewer;
   private Action actionRefresh;
   private Action actionCreateNode;
   private Action actionBind;
   private Action actionUnbind;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      final String[] names = { "ID", "State", "Node", "IP address", "Channels", "System name", "Platform", "System information", "Agent version" };
      final int[] widths = { 80, 80, 140, 150, 80, 150, 150, 300, 150 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new TunnelListLabelProvider());
      viewer.setComparator(new TunnelListComparator());
      
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      WidgetHelper.restoreTableViewerSettings(viewer, settings, "TunnelManager");
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, settings, "TunnelManager");
         }
      });
      
      createActions();
      contributeToActionBars();
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
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };
      
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
      manager.add(actionRefresh);
   }

   /**
    * Fill local toolbar
    * @param manager menu manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
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
    * @param manager Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection.size() == 1) && !((AgentTunnel)selection.getFirstElement()).isBound())
      {
         manager.add(actionBind);
         manager.add(actionCreateNode);
      }
      else
      {
         for(Object o : selection.toList())
         {
            if (((AgentTunnel)o).isBound())
            {
               manager.add(actionUnbind);
               break;
            }
         }
      }
   }
   
   /**
    * Refresh view
    */
   private void refresh()
   {
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Get list of active agent tunnels", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return "Cannot get list of active agent tunnels";
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
      
      CreateNodeDialog dlg = new CreateNodeDialog(getSite().getShell(), null);
      dlg.setEnableShowAgainFlag(false);
      dlg.setObjectName(tunnel.getSystemName());
      dlg.setZoneId(tunnel.getZoneId());
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
      cd.setZoneId(dlg.getZoneId());
      cd.setSshLogin(dlg.getSshLogin());
      cd.setSshPassword(dlg.getSshPassword());
      
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Create new node and bind tunnel", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            long nodeId = session.createObject(cd);
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
            return "Cannot create node and bind tunnel";
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
      
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), null, ObjectSelectionDialog.createNodeSelectionFilter(false));
      if (dlg.open() != Window.OK)
         return;      
      final long nodeId = dlg.getSelectedObjects().get(0).getObjectId();
      
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Bind tunnels", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return "Cannot bind tunnel";
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
      
      if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Unbind Tunnel", "Selected tunnels will be unbound. Are you sure?"))
         return;
      
      final Object[] tunnels = selection.toArray();
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Unbind tunnels", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return "Cannot unbind tunnel";
         }
      }.start();
   }
}
