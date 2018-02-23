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
package org.netxms.ui.eclipse.objectbrowser.views;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CheckboxTableViewer;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.events.TraverseEvent;
import org.eclipse.swt.events.TraverseListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.part.ViewPart;
import org.netxms.base.Glob;
import org.netxms.base.InetAddressEx;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectFilter;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.NetworkService;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.VPNConnector;
import org.netxms.client.objects.Zone;
import org.netxms.client.objects.ZoneMember;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectbrowser.views.helpers.ObjectSearchResultComparator;
import org.netxms.ui.eclipse.objectbrowser.views.helpers.ObjectSearchResultLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.CommandBridge;
import org.netxms.ui.eclipse.tools.ComparatorHelper;
import org.netxms.ui.eclipse.tools.FilteringMenuManager;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Object finder view
 */
public class ObjectFinder extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.objectbrowser.views.ObjectFinder";
   
   public static final int COL_ID = 0;
   public static final int COL_CLASS = 1;
   public static final int COL_NAME = 2;
   public static final int COL_IP_ADDRESS = 3;
   public static final int COL_PARENT = 4;
   public static final int COL_ZONE = 5;
   
   private static final int SEARCH_MODE_NORMAL = 0;
   private static final int SEARCH_MODE_PATTERN = 1;
   private static final int SEARCH_MODE_REGEXP = 2;
   
   private static final List<ObjectClass> OBJECT_CLASSES;
   
   static
   {
      OBJECT_CLASSES = new ArrayList<ObjectClass>();
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_ACCESSPOINT, "Access Point"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_AGENTPOLICY_CONFIG, "Agent Policy (Config)"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_AGENTPOLICY_LOGPARSER, "Agent Policy (Log Parser)"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_BUSINESSSERVICE, "Business Service"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_BUSINESSSERVICEROOT, "Business Service Root"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_CHASSIS, "Chassis"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_CLUSTER, "Cluster"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_CONDITION, "Condition"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_CONTAINER, "Container"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_DASHBOARD, "Dashboard"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_DASHBOARDROOT, "Dashboard Root"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_INTERFACE, "Interface"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_MOBILEDEVICE, "Mobile Device"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_NETWORK, "Network"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_NETWORKMAP, "Network Map"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_NETWORKMAPGROUP, "Network Map Group"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_NETWORKMAPROOT, "Network Map Root"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_NETWORKSERVICE, "Network Service"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_NODE, "Node"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_NODELINK, "Node Link"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_POLICYGROUP, "Policy Group"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_POLICYROOT, "Policy Root"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_RACK, "Rack"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_SENSOR, "Sensor"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_SERVICEROOT, "Service Root"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_SLMCHECK, "Service Check"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_SUBNET, "Subnet"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_TEMPLATE, "Template"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_TEMPLATEGROUP, "Template Group"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_TEMPLATEROOT, "Template Root"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_VPNCONNECTOR, "VPN Connector"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_ZONE, "Zone"));
   }
   
   private SortableTableViewer results;
   private LabeledText text;
   private Button radioPlainText;
   private Button radioPattern;
   private Button radioRegularExpression;
   private CheckboxTableViewer classList;
   private CheckboxTableViewer zoneList;
   private Text ipRangeStart;
   private Text ipRangeEnd;
   private Action actionStartSearch;
   private Action actionShowObjectDetails;
   private NXCSession session;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      session = ConsoleSharedData.getSession();
      
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      parent.setLayout(layout);

      Composite conditionGroup = new Composite(parent, SWT.NONE);
      conditionGroup.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      layout = new GridLayout();
      layout.numColumns = 2;
      conditionGroup.setLayout(layout);
      
      /*** Full text search ***/
      Composite fullTextSearchGroup = new Composite(conditionGroup, SWT.NONE);
      fullTextSearchGroup.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      layout = new GridLayout();
      fullTextSearchGroup.setLayout(layout);
      
      text = new LabeledText(fullTextSearchGroup, SWT.NONE);
      text.setLabel("Search string");
      text.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
      TraverseListener traverseListener = new TraverseListener() {
         @Override
         public void keyTraversed(TraverseEvent e)
         {
            if (e.detail == SWT.TRAVERSE_RETURN)
               startSearch();
         }
      };
      text.addTraverseListener(traverseListener);
      
      Group searchModeGroup = new Group(fullTextSearchGroup, SWT.NONE);
      searchModeGroup.setText("Search mode");
      layout = new GridLayout();
      searchModeGroup.setLayout(layout);
      
      radioPlainText = new Button(searchModeGroup, SWT.RADIO);
      radioPlainText.setText("&Normal");
      radioPlainText.setSelection(true);

      radioPattern = new Button(searchModeGroup, SWT.RADIO);
      radioPattern.setText("&Pattern (* = any string, ? = any character)");

      radioRegularExpression = new Button(searchModeGroup, SWT.RADIO);
      radioRegularExpression.setText("&Regular expression");

      /*** Class filter ***/
      Composite classFilterGroup = new Composite(conditionGroup, SWT.NONE);
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.verticalSpan = 2;
      classFilterGroup.setLayoutData(gd);
      layout = new GridLayout();
      classFilterGroup.setLayout(layout);
      
      Label classFilterTitle = new Label(classFilterGroup, SWT.NONE);
      classFilterTitle.setText("Class filter");
      
      classList = CheckboxTableViewer.newCheckList(classFilterGroup, SWT.BORDER | SWT.CHECK);
      classList.setContentProvider(new ArrayContentProvider());
      classList.setInput(OBJECT_CLASSES);
      classList.setAllChecked(true);
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 100;
      classList.getTable().setLayoutData(gd);
      
      Composite classListButtons = new Composite(classFilterGroup, SWT.NONE);
      RowLayout rlayout = new RowLayout();
      rlayout.marginLeft = 0;
      classListButtons.setLayout(rlayout);
      
      Button selectAll = new Button(classListButtons, SWT.PUSH);
      selectAll.setText("Select &all");
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      selectAll.setLayoutData(rd);
      selectAll.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            classList.setAllChecked(true);
         }
      });
      
      Button clearAll = new Button(classListButtons, SWT.PUSH);
      clearAll.setText("&Clear all");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      clearAll.setLayoutData(rd);
      clearAll.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            classList.setAllChecked(false);
         }
      });
      
      /*** IP filter ***/
      Group ipFilterGroup = new Group(conditionGroup, SWT.NONE);
      ipFilterGroup.setText("IP Range");
      ipFilterGroup.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      layout = new GridLayout();
      layout.numColumns = 3;
      ipFilterGroup.setLayout(layout);
      
      ipRangeStart = new Text(ipFilterGroup, SWT.BORDER);
      ipRangeStart.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      ipRangeStart.addTraverseListener(traverseListener);
      
      new Label(ipFilterGroup, SWT.NONE).setText(" - ");

      ipRangeEnd = new Text(ipFilterGroup, SWT.BORDER);
      ipRangeEnd.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      ipRangeEnd.addTraverseListener(traverseListener);
      
      /*** Search button ***/
      Button searchButton = new Button(conditionGroup, SWT.PUSH);
      searchButton.setText("&Search");
      gd = new GridData(SWT.LEFT, SWT.BOTTOM, true, false);
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      searchButton.setLayoutData(gd);
      searchButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            startSearch();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
      final String[] names = { "ID", "Class", "Name", "IP Address", "Parent", "Zone" };
      final int[] widths = { 90, 120, 300, 250, 300, 200 };
      results = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      if (!ConsoleSharedData.getSession().isZoningEnabled())
         results.removeColumnById(COL_ZONE);
      results.setContentProvider(new ArrayContentProvider());
      results.setLabelProvider(new ObjectSearchResultLabelProvider());
      results.setComparator(new ObjectSearchResultComparator());
      results.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      WidgetHelper.restoreTableViewerSettings(results, settings, "ResultTable");
      results.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(results, settings, "ResultTable");
         }
      });
      
      if (session.isZoningEnabled())
      {      
         /*** Zone filter ***/
         Composite zoneFilterGroup = new Composite(conditionGroup, SWT.NONE);
         gd = new GridData(SWT.FILL, SWT.FILL, true, true);
         gd.verticalSpan = 2;
         zoneFilterGroup.setLayoutData(gd);
         layout = new GridLayout();
         zoneFilterGroup.setLayout(layout);
         
         Label zoneFilterTitle = new Label(zoneFilterGroup, SWT.NONE);
         zoneFilterTitle.setText("Zone filter");
         
         zoneList = CheckboxTableViewer.newCheckList(zoneFilterGroup, SWT.BORDER | SWT.CHECK);
         zoneList.setContentProvider(new ArrayContentProvider());
         List<Zone> zones = session.getAllZones();
         zoneList.setLabelProvider(new WorkbenchLabelProvider());
         zoneList.setInput(zones);
         zoneList.setAllChecked(true);
         gd = new GridData(SWT.FILL, SWT.FILL, true, true);
         gd.heightHint = 100;
         zoneList.getTable().setLayoutData(gd);
         
         Composite zoneListButtons = new Composite(zoneFilterGroup, SWT.NONE);
         rlayout = new RowLayout();
         rlayout.marginLeft = 0;
         zoneListButtons.setLayout(rlayout);
         
         selectAll = new Button(zoneListButtons, SWT.PUSH);
         selectAll.setText("Select &all");
         rd = new RowData();
         rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
         selectAll.setLayoutData(rd);
         selectAll.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               zoneList.setAllChecked(true);
            }
         });
         
         clearAll = new Button(zoneListButtons, SWT.PUSH);
         clearAll.setText("&Clear all");
         rd = new RowData();
         rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
         clearAll.setLayoutData(rd);
         clearAll.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               zoneList.setAllChecked(false);
            }
         });
      }
      
      getSite().setSelectionProvider(results);
      createResultsContextMenu();
      
      activateContext();
      createActions();
      contributeToActionBars();
   }
   
   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.objectbrowser.context.ObjectFinder"); //$NON-NLS-1$
      }
   }
   
   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);

      actionStartSearch = new Action("&Search", Activator.getImageDescriptor("icons/find.gif")) {
         @Override
         public void run()
         {
            startSearch();
         }
      };
      actionStartSearch.setId("org.netxms.ui.eclipse.objectbrowser.actions.startSearch"); //$NON-NLS-1$
      actionStartSearch.setActionDefinitionId("org.netxms.ui.eclipse.objectbrowser.commands.start_search"); //$NON-NLS-1$
      final ActionHandler showFilterHandler = new ActionHandler(actionStartSearch);
      handlerService.activateHandler(actionStartSearch.getActionDefinitionId(), showFilterHandler);

      actionShowObjectDetails = new Action("Show object details") {
         @Override
         public void run()
         {
            showObjectDetails();
         }
      };
   }
   
   /**
    * Fill action bars
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * @param manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionStartSearch);
   }

   /**
    * Fill local tool bar
    * @param manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionStartSearch);
   }
   
   /**
    * Create context menu for results
    */
   private void createResultsContextMenu()
   {
      // Create menu manager.
      MenuManager manager = new FilteringMenuManager(Activator.PLUGIN_ID);
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            ObjectContextMenu.fill(mgr, getSite(), results);
            if (((IStructuredSelection)results.getSelection()).size() == 1)
            {
               mgr.insertAfter(GroupMarkers.MB_PROPERTIES, actionShowObjectDetails);
            }
         }
      });

      // Create menu.
      Menu menu = manager.createContextMenu(results.getTable());
      results.getTable().setMenu(menu);

      // Register menu for extension.
      getSite().registerContextMenu(manager, results);
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      text.setFocus();
   }
   
   /**
    * Start search 
    */
   private void startSearch()
   {
      InetAddress addrStart = null, addrEnd = null;
      if (!ipRangeStart.getText().trim().isEmpty() && !ipRangeEnd.getText().trim().isEmpty())
      {
         try
         {
            addrStart = InetAddress.getByName(ipRangeStart.getText().trim());
         }
         catch(UnknownHostException e)
         {
            MessageDialogHelper.openWarning(getSite().getShell(), "Warning", "IP address range start is invalid");
         }
         try
         {
            addrEnd = InetAddress.getByName(ipRangeEnd.getText().trim());
         }
         catch(UnknownHostException e)
         {
            MessageDialogHelper.openWarning(getSite().getShell(), "Warning", "IP address range end is invalid");
         }
      }
      
      List<Integer> classFilter = new ArrayList<Integer>(); 
      for(Object o : classList.getCheckedElements())
         classFilter.add(((ObjectClass)o).classId);
      
      List<Long> zoneFilter = new ArrayList<Long>();
      if (session.isZoningEnabled())
      {
         for(Object o : zoneList.getCheckedElements())
            zoneFilter.add(((Zone)o).getUIN());
      }
      
      if (radioRegularExpression.getSelection())
         doSearch(text.getText().trim(), SEARCH_MODE_REGEXP, classFilter, zoneFilter, addrStart, addrEnd);
      else
         doSearch(text.getText().trim().toLowerCase(), radioPattern.getSelection() ? SEARCH_MODE_PATTERN : SEARCH_MODE_NORMAL, classFilter, zoneFilter, addrStart, addrEnd);
   }
   
   /**
    * Do object search
    * 
    * @param searchString search string
    * @param classFilter
    * @param zoneFilter
    * @param addrEnd 
    * @param addrStart 
    */
   private void doSearch(final String searchString, final int mode, final List<Integer> classFilter, final List<Long> zoneFilter, final InetAddress addrStart, final InetAddress addrEnd)
   {
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Find objects", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final Pattern regexp = (mode == SEARCH_MODE_REGEXP) ? Pattern.compile(searchString, Pattern.CASE_INSENSITIVE | Pattern.DOTALL | Pattern.UNICODE_CASE) : null;
            final List<AbstractObject> objects = session.filterObjects(new ObjectFilter() {
               @Override
               public boolean filter(AbstractObject object)
               {
                  if (session.isZoningEnabled() && session.getAllZones().size() != zoneFilter.size())
                  {
                     if (object instanceof ZoneMember)
                     {
                         ZoneMember node = (ZoneMember)object;
                         return zoneFilter.contains(node.getZoneId());
                     }
                     if (object instanceof Sensor)
                     {
                        AbstractNode proxy = session.findObjectById(((Sensor)object).getProxyId(), AbstractNode.class);
                        if (proxy != null)
                           return zoneFilter.contains(proxy.getZoneId());
                     }
                     else if (object instanceof Interface)
                     {
                        AbstractNode parent = ((Interface)object).getParentNode();
                        if (parent != null)
                           return zoneFilter.contains(parent.getZoneId());
                     }
                     else if (object instanceof NetworkService)
                     {
                        AbstractNode parent = ((NetworkService)object).getParentNode();
                        if (parent != null)
                           return zoneFilter.contains(parent.getZoneId());
                     }  
                     else if (object instanceof VPNConnector)
                     {
                        AbstractNode parent = ((VPNConnector)object).getParentNode();
                        if (parent != null)
                           return zoneFilter.contains(parent.getZoneId());
                     }
                     else if (object instanceof GenericObject)
                     {
                        AbstractObject children[] = ((GenericObject)object).getChildsAsArray();
                        boolean match = false;
                        for(AbstractObject o : children)
                        {
                           if (o instanceof ZoneMember && zoneFilter.contains(((ZoneMember)o).getZoneId()))
                              match = true;
                        }
                        return match;
                     }
                  }
                  
                  if (!classFilter.contains(object.getObjectClass()))
                     return false;
                  
                  if ((addrStart != null) && (addrEnd != null))
                  {
                     if (!checkAddrRange(addrStart, addrEnd, object))
                        return false;
                  }
                  
                  if (searchString.isEmpty())
                     return true;
                  
                  for(String s : object.getStrings())
                  {
                     switch(mode)
                     {
                        case SEARCH_MODE_NORMAL:
                           if (s.toLowerCase().contains(searchString))
                              return true;
                           break;
                        case SEARCH_MODE_PATTERN:
                           if (Glob.match(searchString, s.toLowerCase()))
                              return true;
                           break;
                        case SEARCH_MODE_REGEXP:
                           if (regexp.matcher(s).find())
                              return true;
                           break;
                     }
                  }
                  return false;
               }
            });
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  results.setInput(objects);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Object search failure";
         }
      }.start();
   }
   
   /**
    * Check if object is within given address range
    * 
    * @param start
    * @param end
    * @param object
    * @return
    */
   private static boolean checkAddrRange(InetAddress start, InetAddress end, AbstractObject object)
   {
      List<InetAddressEx> addrList = new ArrayList<InetAddressEx>(16);
      if (object instanceof AbstractNode)
      {
         addrList.add(((AbstractNode)object).getPrimaryIP());
         for(AbstractObject o : object.getAllChilds(AbstractObject.OBJECT_INTERFACE))
         {
            addrList.addAll(((Interface)o).getIpAddressList());
         }
      }
      else if (object instanceof Interface)
      {
         addrList.addAll(((Interface)object).getIpAddressList());
      }
      else if (object instanceof AccessPoint)
      {
         addrList.add(((AccessPoint)object).getIpAddress());
      }
      
      for(InetAddressEx a : addrList)
      {
         if ((ComparatorHelper.compareInetAddresses(a.getAddress(), start) >= 0) &&
             (ComparatorHelper.compareInetAddresses(a.getAddress(), end) <= 0))
            return true;
      }
      
      return false;
   }

   /**
    * Show details for selected object
    */
   private void showObjectDetails()
   {
      IStructuredSelection selection = (IStructuredSelection)results.getSelection();
      if (selection.size() != 1)
         return;

      AbstractObject object = (AbstractObject)selection.getFirstElement();
      try
      {
         PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage()
               .showView("org.netxms.ui.eclipse.objectview.view.tabbed_object_view"); //$NON-NLS-1$
         CommandBridge.getInstance().execute("TabbedObjectView/changeObject", object.getObjectId());
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(getSite().getShell(), "Error",
               String.format("Cannot open object details view (%s)", e.getLocalizedMessage()));
      }
   }
   
   /**
    * Object class ID to name mapping
    */
   private static class ObjectClass
   {
      int classId;
      String className;
      
      ObjectClass(int classId, String className)
      {
         this.classId = classId;
         this.className = className;
      }

      @Override
      public String toString()
      {
         return className;
      }
   }
}
