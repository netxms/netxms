package org.netxms.ui.eclipse.topology.widgets;

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
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ConnectionPointType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.Messages;
import org.netxms.ui.eclipse.topology.widgets.helpers.ConnectionPointComparator;
import org.netxms.ui.eclipse.topology.widgets.helpers.ConnectionPointLabelProvider;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;
import org.netxms.ui.eclipse.widgets.MessageBar;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

public class SearchResult extends CompositeWithMessageBar
{
   public static final int COLUMN_SEQUENCE = 0;
   public static final int COLUMN_NODE = 1;
   public static final int COLUMN_INTERFACE = 2;
   public static final int COLUMN_MAC_ADDRESS = 3;
   public static final int COLUMN_NIC_VENDOR = 4;
   public static final int COLUMN_IP_ADDRESS = 5;
   public static final int COLUMN_SWITCH = 6;
   public static final int COLUMN_PORT = 7;
   public static final int COLUMN_TYPE = 8;
   
   private ViewPart viewPart;
   private SortableTableViewer viewer;
   private List<ConnectionPoint> results = new ArrayList<ConnectionPoint>();
   private Action actionClearLog;
   private Action actionCopyMAC;
   private Action actionCopyIP;
   private Action actionCopyRecord;

   /**
    * Create serach result widget
    * 
    * @param viewPart owning view part
    * @param parent parent widget
    * @param style style
    * @param configPrefix configuration prefix for saving/restoring viewer settings
    */
   public SearchResult(ViewPart viewPart, Composite parent, int style, final String configPrefix)
   {
      super(parent, style);
      this.viewPart = viewPart;

      final String[] names = { Messages.get().HostSearchResults_ColSeq, Messages.get().HostSearchResults_ColNode, Messages.get().HostSearchResults_ColIface, Messages.get().HostSearchResults_ColMac, "NIC vendor", Messages.get().HostSearchResults_ColIp, Messages.get().HostSearchResults_ColSwitch, Messages.get().HostSearchResults_ColPort, Messages.get().HostSearchResults_ColType };
      final int[] widths = { 70, 120, 120, 180, 90, 90, 120, 120, 60 };
      viewer = new SortableTableViewer(getContent(), names, widths, COLUMN_SEQUENCE, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      ConnectionPointLabelProvider labelProvider = new ConnectionPointLabelProvider(viewer);
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new ConnectionPointComparator(labelProvider));
      WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configPrefix);

      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configPrefix);
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
      actionClearLog = new Action(Messages.get().HostSearchResults_ClearLog) {
         @Override
         public void run()
         {
            results.clear();
            viewer.setInput(results.toArray());
         }
      };
      actionClearLog.setImageDescriptor(SharedIcons.CLEAR_LOG);
      
      actionCopyIP = new Action(Messages.get().HostSearchResults_CopyIp) {
         @Override
         public void run()
         {
            copyToClipboard(COLUMN_IP_ADDRESS);
         }
      };
      
      actionCopyMAC = new Action(Messages.get().HostSearchResults_CopyMac) {
         @Override
         public void run()
         {
            copyMacAddress();
         }
      };
      
      actionCopyRecord = new Action(Messages.get().HostSearchResults_Copy, SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyToClipboard(-1);
         }
      };
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
      viewPart.getSite().setSelectionProvider(viewer);
      viewPart.getSite().registerContextMenu(menuMgr, viewer);
   }
   
   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionCopyRecord);
      manager.add(actionCopyIP);
      manager.add(actionCopyMAC);
      manager.add(new Separator());
      manager.add(actionClearLog);
      manager.add(new Separator());
      manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
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
      manager.add(new Separator());
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
      hideMessage();
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
         showMessage(MessageBar.WARNING, Messages.get().HostSearchResults_NotFound);
         return;
      }

      final NXCSession session = ConsoleSharedData.getSession();
      if (session.areChildrenSynchronized(cp.getNodeId()) && session.areChildrenSynchronized(cp.getLocalNodeId()))
      {
         showConnectionStep2(session, cp);
      }
      else
      {
         new ConsoleJob("Synchronize objects", viewPart, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
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
                  showMessage(MessageBar.INFORMATION, 
                        String.format(Messages.get().HostSearchResults_NodeConnectedToWiFi,
                              host.getObjectName(), bridge.getObjectName(), iface.getObjectName()));
               }
               else
               {
                  if (cp.getLocalIpAddress() != null)
                  {
                     showMessage(MessageBar.INFORMATION, 
                           String.format(Messages.get().HostSearchResults_NodeIpMacConnectedToWiFi,
                                 cp.getLocalIpAddress().getHostAddress(), cp.getLocalMacAddress(),
                                 bridge.getObjectName(), iface.getObjectName()));
                  }
                  else
                  {
                     showMessage(MessageBar.INFORMATION, 
                           String.format(Messages.get().HostSearchResults_NodeMacConnectedToWiFi,
                                 cp.getLocalMacAddress(), bridge.getObjectName(), iface.getObjectName()));
                  }
               }
            }
            else
            {
               if (host != null)
               {
                  showMessage(MessageBar.INFORMATION, 
                        String.format(Messages.get().HostSearchResults_NodeConnected, host.getObjectName(),
                              cp.getType() == ConnectionPointType.DIRECT ? Messages.get().HostSearchResults_ModeDirectly
                                    : Messages.get().HostSearchResults_ModeIndirectly,
                              bridge.getObjectName(), iface.getObjectName()));
               }
               else
               {
                  if (cp.getLocalIpAddress() != null)
                  {
                     showMessage(MessageBar.INFORMATION, 
                           String.format(Messages.get().HostSearchResults_NodeIpMacConnected,
                                 cp.getLocalIpAddress().getHostAddress(), cp.getLocalMacAddress(),
                                 cp.getType() == ConnectionPointType.DIRECT ? Messages.get().HostSearchResults_ModeDirectly
                                       : Messages.get().HostSearchResults_ModeIndirectly,
                                 bridge.getObjectName(), iface.getObjectName()));
                  }
                  else
                  {
                     showMessage(MessageBar.INFORMATION, 
                           String.format(Messages.get().HostSearchResults_NodeMacConnected,
                                 cp.getLocalMacAddress(),
                                 cp.getType() == ConnectionPointType.DIRECT ? Messages.get().HostSearchResults_ModeDirectly : Messages.get().HostSearchResults_ModeIndirectly,
                                 bridge.getObjectName(), iface.getObjectName()));
                  }
               }
            }
            addResult(cp);
         }
         else if ((host != null) && (cp.getType() == ConnectionPointType.UNKNOWN))
         {
            showMessage(MessageBar.WARNING, 
                  String.format("Found node %s but it's connection point is unknown", host.getObjectName()));
            addResult(cp);
         }
         else
         {
            showMessage(MessageBar.WARNING, Messages.get().HostSearchResults_NotFound);
         }
      }
      catch(Exception e)
      {
         Activator.logError("Exception in host search result view", e);
         showMessage(MessageBar.WARNING, 
               String.format(Messages.get().HostSearchResults_ShowError, e.getLocalizedMessage()));         
      }
   } /**
    * Show connection point information
    * 
    * @param cp[] connection point information
    */
   public void showConnection(final List<ConnectionPoint> cps)
   {
      retireResults();
      final NXCSession session = ConsoleSharedData.getSession();
      if (session.areObjectsSynchronized())
      {
         showConnectionStep2(session, cps);
      }
      else
      {
         new ConsoleJob("Synchronize objects", null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
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
            showMessage(MessageBar.WARNING, 
                  Messages.get().HostSearchResults_NotFound + " for " + counter + " interfaces!");    
         }
         
      }
      catch(Exception e)
      {
         showMessage(MessageBar.WARNING, 
               String.format(Messages.get().HostSearchResults_ShowError, e.getLocalizedMessage())); 
      }
   }
}
