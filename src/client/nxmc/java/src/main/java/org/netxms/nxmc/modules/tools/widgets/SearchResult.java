/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.tools.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ConnectionPointType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.widgets.DashboardControl;
import org.netxms.nxmc.modules.tools.widgets.helpers.ConnectionPointComparator;
import org.netxms.nxmc.modules.tools.widgets.helpers.ConnectionPointLabelProvider;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

public class SearchResult extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(SearchResult.class);
   private static final Logger logger = LoggerFactory.getLogger(DashboardControl.class);
   
   public static final int COLUMN_SEQUENCE = 0;
   public static final int COLUMN_NODE = 1;
   public static final int COLUMN_INTERFACE = 2;
   public static final int COLUMN_MAC_ADDRESS = 3;
   public static final int COLUMN_NIC_VENDOR = 4;
   public static final int COLUMN_IP_ADDRESS = 5;
   public static final int COLUMN_SWITCH = 6;
   public static final int COLUMN_PORT = 7;
   public static final int COLUMN_INDEX = 8;
   public static final int COLUMN_TYPE = 9;
   
   private View view;
   private SortableTableViewer viewer;
   private List<ConnectionPoint> results = new ArrayList<ConnectionPoint>();
   private Action actionClearLog;
   private Action actionCopyMAC;
   private Action actionCopyIP;
   private Action actionCopyRecord;

   /**
    * Create serach result widget
    * 
    * @param view owning view part
    * @param parent parent widget
    * @param style style
    * @param configPrefix configuration prefix for saving/restoring viewer settings
    */
   public SearchResult(View view, Composite parent, int style, final String configPrefix)
   {
      super(parent, style);
      this.view = view;
      setLayout(new FillLayout());

      final String[] names = { i18n.tr("Seq."), i18n.tr("Node"), i18n.tr("Interface"), i18n.tr("MAC"), i18n.tr("NIC vendor"), i18n.tr("IP"), i18n.tr("Switch"), i18n.tr("Port"), i18n.tr("Index"), i18n.tr("Type") };
      final int[] widths = { 70, 120, 120, 180, 90, 90, 120, 120, 60, 60 };
      viewer = new SortableTableViewer(this, names, widths, COLUMN_SEQUENCE, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      ConnectionPointLabelProvider labelProvider = new ConnectionPointLabelProvider(viewer);
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new ConnectionPointComparator(labelProvider));
      WidgetHelper.restoreTableViewerSettings(viewer, configPrefix);

      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, configPrefix);
         }
      });

      createActions();
      createPopupMenu();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionClearLog = new Action(i18n.tr("Clear search log")) {
         @Override
         public void run()
         {
            results.clear();
            viewer.setInput(results.toArray());
         }
      };
      actionClearLog.setImageDescriptor(SharedIcons.CLEAR_LOG);
      view.addKeyBinding("M1+L", actionClearLog);

      actionCopyIP = new Action(i18n.tr("Copy IP address to clipboard")) {
         @Override
         public void run()
         {
            copyToClipboard(COLUMN_IP_ADDRESS);
         }
      };

      actionCopyMAC = new Action(i18n.tr("Copy MAC address to clipboard")) {
         @Override
         public void run()
         {
            copyMacAddress();
         }
      };

      actionCopyRecord = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyToClipboard(-1);
         }
      };
      view.addKeyBinding("M1+C", actionCopyRecord);
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {      
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }
   
   /**
    * Fill context menu
    * @param nodeMenuManager 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionCopyRecord);
      manager.add(actionCopyIP);
      manager.add(actionCopyMAC);
      manager.add(new Separator());
      manager.add(actionClearLog);
   }
   
   /**
    * Copy mac address
    */
   private void copyMacAddress()
   {
      @SuppressWarnings("unchecked")
      final List<ConnectionPoint> selection = viewer.getStructuredSelection().toList();
      if (selection.size() > 0)
      {
         final String newLine = WidgetHelper.getNewLineCharacters();
         final StringBuilder sb = new StringBuilder();
         for(int i = 0; i < selection.size(); i++)
         {
            if (i > 0)
               sb.append(newLine);
            
            sb.append(selection.get(i).getLocalMacAddress().toString());
         }
         WidgetHelper.copyToClipboard(sb.toString());
      }      
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
         final String newLine = WidgetHelper.getNewLineCharacters();
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

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public boolean setFocus()
   {
      return viewer.getTable().setFocus();
   }

   /**
    * Fill local pulldown menu
    * @param manager menu manager
    */
   public void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionCopyRecord);
      manager.add(actionCopyIP);
      manager.add(actionCopyMAC);
      manager.add(new Separator());
      manager.add(actionClearLog);
   }

   /**
    * Fill local toolbar
    * @param manager menu manager
    */
   public void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCopyRecord);
      manager.add(actionClearLog);
   }

   /**
    * Add search result to view
    * 
    * @param cp connection point information
    */
   private void addResult(ConnectionPoint cp)
   {
      cp.setData(results.size());
      results.add(cp);
      viewer.setInput(results.toArray());
   }
   
   /**
    * Add search result to view
    * 
    * @param cp connection point information
    */
   private void retireResults()
   {
      for (ConnectionPoint r : results)
      {
         r.setHistorical(true);
      }
      view.clearMessages();
   }

   /**
    * Show connection point information.
    *
    * @param cp connection point information
    */
   public void showConnection(final ConnectionPoint cp)
   {
      retireResults();
      if (cp == null)
      {
         view.addMessage(MessageArea.WARNING, i18n.tr("Connection point information cannot be found"));
         return;
      }

      final NXCSession session = Registry.getSession();
      if (session.areChildrenSynchronized(cp.getNodeId()) && session.areChildrenSynchronized(cp.getLocalNodeId()))
      {
         showConnectionStep2(session, cp);
      }
      else
      {
         new Job("Synchronize objects", view) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               AbstractObject object = session.findObjectById(cp.getNodeId());
               if (object != null)
                  session.syncChildren(object);
               object = session.findObjectById(cp.getLocalNodeId());
               if (object != null)
                  session.syncChildren(object);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     showConnectionStep2(session, cp);
                  }
               });
            }
            
            @Override
            protected String getErrorMessage()
            {
               return "Cannot synchronize objects";
            }
         }.start();
      }
   }

   /**
    * Show connection point information - step 2.
    * 
    * @param session communication session
    * @param cp connection point information
    */
   private void showConnectionStep2(NXCSession session, ConnectionPoint cp)
   {
      try
      {
         Node host = (Node)session.findObjectById(cp.getLocalNodeId());
         Node bridge = (Node)session.findObjectById(cp.getNodeId());
         AbstractObject iface = session.findObjectById(cp.getInterfaceId());
         if ((bridge != null) && (iface != null))
         {
            if (cp.getType() == ConnectionPointType.WIRELESS)
            {
               if (host != null)
               {
                  view.addMessage(MessageArea.INFORMATION, 
                        String.format(i18n.tr("Node %1$s is connected to wireless access point %2$s/%3$s"),
                              host.getObjectName(), bridge.getObjectName(), iface.getObjectName()));
               }
               else
               {
                  if (cp.getLocalIpAddress() != null)
                  {
                     view.addMessage(MessageArea.INFORMATION, 
                           String.format(i18n.tr("Node with IP address %1$s and MAC address %2$s is connected to wireless access point %3$s/%4$s"),
                                 cp.getLocalIpAddress().getHostAddress(), cp.getLocalMacAddress(),
                                 bridge.getObjectName(), iface.getObjectName()));
                  }
                  else
                  {
                     view.addMessage(MessageArea.INFORMATION, 
                           String.format(i18n.tr("Node with MAC address %1$s is connected to wireless access point %2$s/%3$s"),
                                 cp.getLocalMacAddress(), bridge.getObjectName(), iface.getObjectName()));
                  }
               }
            }
            else
            {
               if (host != null)
               {
                  view.addMessage(MessageArea.INFORMATION, 
                        String.format(i18n.tr("Node %1$s is %2$s connected to network switch %3$s port %4$s"), host.getObjectName(),
                              cp.getType() == ConnectionPointType.DIRECT ? i18n.tr("directly")
                                    : i18n.tr("indirectly"),
                              bridge.getObjectName(), iface.getObjectName()));
               }
               else
               {
                  if (cp.getLocalIpAddress() != null)
                  {
                     view.addMessage(MessageArea.INFORMATION, 
                           String.format(i18n.tr("Node with IP address %1$s and MAC address %2$s is %3$s connected to network switch %4$s port %5$s"),
                                 cp.getLocalIpAddress().getHostAddress(), cp.getLocalMacAddress(),
                                 cp.getType() == ConnectionPointType.DIRECT ? i18n.tr("directly")
                                       : i18n.tr("indirectly"),
                                 bridge.getObjectName(), iface.getObjectName()));
                  }
                  else
                  {
                     view.addMessage(MessageArea.INFORMATION, 
                           String.format(i18n.tr("Node with MAC address %1$s is %2$s connected to network switch %3$s port %4$s"),
                                 cp.getLocalMacAddress(),
                                 cp.getType() == ConnectionPointType.DIRECT ? i18n.tr("directly") : i18n.tr("indirectly"),
                                 bridge.getObjectName(), iface.getObjectName()));
                  }
               }
            }
            addResult(cp);
         }
         else if ((host != null) && (cp.getType() == ConnectionPointType.UNKNOWN))
         {
            view.addMessage(MessageArea.WARNING, 
                  String.format("Found node %s but it's connection point is unknown", host.getObjectName()));
            addResult(cp);
         }
         else
         {
            view.addMessage(MessageArea.WARNING, i18n.tr("Connection point information cannot be found"));
         }
      }
      catch(Exception e)
      {
         logger.error("Exception in host search result view", e);
         view.addMessage(MessageArea.WARNING, 
               String.format(i18n.tr("Connection point information cannot be shown: %s"), e.getLocalizedMessage()));         
      }
   }

   /**
    * Show connection point information
    * 
    * @param cp[] connection point information
    */
   public void showConnection(final List<ConnectionPoint> cps)
   {
      retireResults();
      final NXCSession session = Registry.getSession();
      if (session.areObjectsSynchronized())
      {
         showConnectionStep2(session, cps);
      }
      else
      {
         new Job("Synchronize objects", view) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               for(ConnectionPoint cp : cps)
               {
                  AbstractObject object = session.findObjectById(cp.getNodeId());
                  if (object != null)
                     session.syncChildren(object);
                  object = session.findObjectById(cp.getLocalNodeId());
                  if (object != null)
                     session.syncChildren(object);
               }
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     showConnectionStep2(session, cps);
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot synchronize objects";
            }
         }.start();
      }
   }

   /**
    * Show connection point information
    * 
    * @param cp[] connection point information
    */
   public void showConnectionStep2(NXCSession session, List<ConnectionPoint> cps)
   {
      int counter = 0;
      try
      {
         for (ConnectionPoint p : cps)
         {                        
            if (p.getType() == ConnectionPointType.NOT_FOUND)
               counter++;
            
            addResult(p);
         }
         if (counter > 0)
         {
            view.addMessage(MessageArea.WARNING, 
                  i18n.tr("Connection point information cannot be found") + " for " + counter + " interfaces!");    
         }
         
      }
      catch(Exception e)
      {
         view.addMessage(MessageArea.WARNING, 
               String.format(i18n.tr("Connection point information cannot be shown: %s"), e.getLocalizedMessage())); 
      }
   }

   /**
    * Copy results between views 
    * 
    * @param serachResultWidget source widget
    */
   public void copyResults(SearchResult serachResultWidget)
   {
      results = new ArrayList<ConnectionPoint>(serachResultWidget.results);
      viewer.setInput(results.toArray());
   }
}
