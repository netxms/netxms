/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Pattern;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CheckboxTableViewer;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
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
import org.eclipse.swt.widgets.TableColumn;
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
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectFilter;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.NetworkService;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.VPNConnector;
import org.netxms.client.objects.Zone;
import org.netxms.client.objects.interfaces.ZoneMember;
import org.netxms.client.objects.queries.ObjectQuery;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectbrowser.views.helpers.ObjectSearchResultComparator;
import org.netxms.ui.eclipse.objectbrowser.views.helpers.ObjectSearchResultLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.CommandBridge;
import org.netxms.ui.eclipse.tools.ComparatorHelper;
import org.netxms.ui.eclipse.tools.FilteringMenuManager;
import org.netxms.ui.eclipse.tools.IntermediateSelectionProvider;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.SelectionTransformation;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.MessageBar;
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
   
   private static final String[] OBJECT_ATTRIBUTES = 
      {
         "adminState",
         "agentVersion",
         "alarms", 
         "alias",
         "bootTime",
         "bridgeBaseAddress",
         "bridgePortNumber",
         "city",
         "comments",
         "components",
         "country",
         "customAttributes",
         "description",
         "dot1xBackendAuthState",
         "dot1xPaeAuthState",
         "driver",
         "expectedState",
         "flags",
         "geolocation",
         "guid",
         "id",
         "ifIndex",
         "ifType",
         "ipAddr",
         "ipAddressList",
         "ipNetMask",
         "isAgent",
         "isBridge",
         "isCDP",
         "isExcludedFromTopology",
         "isLLDP",
         "isLocalManagement",
         "isLoopback",
         "isManuallyCreated",
         "isPAE",
         "isPhysicalPort",
         "isPrinter",
         "isRouter",
         "isSMCLP",
         "isSNMP",
         "isSONMP",
         "isSTP",
         "macAddr",
         "mapImage",
         "mtu",
         "name",
         "node",
         "nodes",
         "operState",
         "peerInterface",
         "peerNode",
         "platformName",
         "port",
         "postcode",
         "proxyNode",
         "proxyNodeId",
         "responsibleUsers",
         "runtimeFlags",
         "snmpOID",
         "snmpSysContact",
         "snmpSysLocation",
         "snmpSysName",
         "snmpVersion",
         "slot",
         "speed",
         "status",
         "streetAddress",
         "sysDescription",
         "type",
         "uin",
         "zone",
         "zoneUIN",
      };
   private static final String[] OBJECT_CONSTANTS =
      {
         "ACCESSPOINT",
         "AGENTPOLICY",
         "AGENTPOLICY_CONFIG",
         "AGENTPOLICY_LOGPARSER",
         "BUSINESSSERVICE",
         "BUSINESSSERVICEROOT",
         "CHASSIS",
         "CLUSTER",
         "CONDITION",
         "CONTAINER",
         "DASHBOARD",
         "DASHBOARDGROUP",
         "DASHBOARDROOT",
         "INTERFACE",
         "MOBILEDEVICE",
         "NETWORK",
         "NETWORKMAP",
         "NETWORKMAPGROUP",
         "NETWORKMAPROOT",
         "NETWORKSERVICE",
         "NODE",
         "NODELINK",
         "POLICYGROUP",
         "POLICYROOT",
         "RACK",
         "SENSOR",
         "SERVICEROOT",
         "SLMCHECK",
         "SUBNET",
         "TEMPLATE",
         "TEMPLATEGROUP",
         "TEMPLATEROOT",
         "VPNCONNECTOR",
         "ZONE"
      };

   private static final List<ObjectClass> OBJECT_CLASSES;

   static
   {
      OBJECT_CLASSES = new ArrayList<ObjectClass>();
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_ACCESSPOINT, "Access Point"));
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
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_RACK, "Rack"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_SENSOR, "Sensor"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_SERVICEROOT, "Service Root"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_SUBNET, "Subnet"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_TEMPLATE, "Template"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_TEMPLATEGROUP, "Template Group"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_TEMPLATEROOT, "Template Root"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_VPNCONNECTOR, "VPN Connector"));
      OBJECT_CLASSES.add(new ObjectClass(AbstractObject.OBJECT_ZONE, "Zone"));
   }

   private NXCSession session = ConsoleSharedData.getSession();
   private SortableTableViewer results;
   private IntermediateSelectionProvider resultSelectionProvider;
   private CTabFolder tabFolder;
   private LabeledText text;
   private Button radioPlainText;
   private Button radioPattern;
   private Button radioRegularExpression;
   private CheckboxTableViewer classList;
   private CheckboxTableViewer zoneList;
   private Text ipRangeStart;
   private Text ipRangeEnd;
   private CompositeWithMessageBar queryEditorMessage;
   private ScriptEditor queryEditor;
   private boolean queryModified = false;
   private TableViewer queryList;
   private Action actionStartSearch;
   private Action actionShowObjectDetails;
   private Action actionSaveAs;
   private Action actionExportToCSV;
   private Action actionExportAllToCSV;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      SashForm splitter = new SashForm(parent, SWT.VERTICAL);

      Composite searchArea = new Composite(splitter, SWT.NONE);
      searchArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      searchArea.setLayout(layout);

      tabFolder = new CTabFolder(searchArea, SWT.TOP | SWT.FLAT | SWT.MULTI);
      tabFolder.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      CTabItem filterTab = new CTabItem(tabFolder, SWT.NONE);
      filterTab.setText("Filter");
      filterTab.setImage(SharedIcons.IMG_FILTER);

      Composite conditionGroup = new Composite(tabFolder, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = session.isZoningEnabled() ? 3 : 2;
      conditionGroup.setLayout(layout);
      filterTab.setControl(conditionGroup);
      
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
      
      /*** Zone filter ***/
      if (session.isZoningEnabled())
      {      
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

      /*** IP filter ***/
      Group ipFilterGroup = new Group(conditionGroup, SWT.NONE);
      ipFilterGroup.setText("IP Range");
      ipFilterGroup.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
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
      Button searchButtonFilter = new Button(conditionGroup, SWT.PUSH);
      searchButtonFilter.setText("&Search");
      gd = new GridData(SWT.LEFT, SWT.BOTTOM, true, false);
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      searchButtonFilter.setLayoutData(gd);
      searchButtonFilter.addSelectionListener(new SelectionListener() {
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

      /*** Query ***/
      CTabItem queryTab = new CTabItem(tabFolder, SWT.NONE);
      queryTab.setText("Query");
      queryTab.setImage(SharedIcons.IMG_FIND);

      Composite queryArea = new Composite(tabFolder, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      queryArea.setLayout(layout);
      queryTab.setControl(queryArea);

      Label labelQueryEditor = new Label(queryArea, SWT.LEFT);
      labelQueryEditor.setText("Query");
      gd = new GridData(SWT.FILL, SWT.BOTTOM, true, false);
      gd.horizontalSpan = 2;
      labelQueryEditor.setLayoutData(gd);

      Label labelQueryList = new Label(queryArea, SWT.LEFT);
      labelQueryList.setText("Saved queries");
      labelQueryList.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      queryEditorMessage = new CompositeWithMessageBar(queryArea, SWT.BORDER);
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 2;
      queryEditorMessage.setLayoutData(gd);

      queryEditor = new ScriptEditor(queryEditorMessage, SWT.NONE, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL, true);
      queryEditorMessage.setContent(queryEditor);
      queryEditor.addVariables(Arrays.asList(OBJECT_ATTRIBUTES));
      queryEditor.addConstants(Arrays.asList(OBJECT_CONSTANTS));
      queryEditor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            queryModified = true;
         }
      });

      queryList = new TableViewer(queryArea, SWT.BORDER | SWT.FULL_SELECTION);
      queryList.setContentProvider(new ArrayContentProvider());
      queryList.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((ObjectQuery)element).getName();
         }
      });
      queryList.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((ObjectQuery)e1).getName().compareToIgnoreCase(((ObjectQuery)e2).getName());
         }
      });
      queryList.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      queryList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            IStructuredSelection selection = queryList.getStructuredSelection();
            if (selection.isEmpty())
               return;

            if (queryModified)
            {
               if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Warning", "You are about to replace current query source which is not saved. All changes will be lost. Are you sure?"))
                  return;
            }

            queryEditor.setText(((ObjectQuery)selection.getFirstElement()).getSource());
            queryEditorMessage.hideMessage();
            queryModified = false;
         }
      });
      loadQueries();

      Button searchButtonQuery = new Button(queryArea, SWT.PUSH);
      searchButtonQuery.setText("&Search");
      gd = new GridData(SWT.LEFT, SWT.BOTTOM, true, false);
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      searchButtonQuery.setLayoutData(gd);
      searchButtonQuery.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            startQuery();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
      Label separator = new Label(searchArea, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      separator.setLayoutData(gd);
      
      
      /*** Result view ***/

      Composite resultArea = new Composite(splitter, SWT.NONE); 
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      resultArea.setLayout(layout);

      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      try
      {
         int[] weights = new int[2];
         weights[0] = settings.getInt("ObjectFinder.weight1"); //$NON-NLS-1$
         weights[1] = settings.getInt("ObjectFinder.weight2"); //$NON-NLS-1$
         splitter.setWeights(weights);
      }
      catch(NumberFormatException e)
      {
         splitter.setWeights(new int[] { 25, 70 });
      }
      
      separator = new Label(resultArea, SWT.SEPARATOR | SWT.HORIZONTAL);      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      separator.setLayoutData(gd);
      
      final String[] names = { "ID", "Class", "Name", "IP Address", "Parent", "Zone" };
      final int[] widths = { 90, 120, 300, 250, 300, 200 };
      results = new SortableTableViewer(resultArea, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      if (!ConsoleSharedData.getSession().isZoningEnabled())
         results.removeColumnById(COL_ZONE);
      results.setContentProvider(new ArrayContentProvider());
      results.setLabelProvider(new ObjectSearchResultLabelProvider(results.getTable()));
      results.setComparator(new ObjectSearchResultComparator());
      results.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      WidgetHelper.restoreTableViewerSettings(results, settings, "ResultTable");
      
      parent.addDisposeListener(new DisposeListener() {
         
         @Override
         public void widgetDisposed(DisposeEvent e)
         {       
            WidgetHelper.saveTableViewerSettings(results, settings, "ResultTable");
            
            int[] weights = splitter.getWeights();
            settings.put("ObjectFinder.weight1", weights[0]); //$NON-NLS-1$
            settings.put("ObjectFinder.weight2", weights[1]); //$NON-NLS-1$
         }
      });

      resultSelectionProvider = new IntermediateSelectionProvider(results, new SelectionTransformation() {
         @Override
         public ISelection transform(ISelection input)
         {
            if (!(input instanceof IStructuredSelection) || input.isEmpty() || !(((IStructuredSelection)input).getFirstElement() instanceof ObjectQueryResult))
               return input;
            List<AbstractObject> objects = new ArrayList<>(((IStructuredSelection)input).size());
            for(Object o : ((IStructuredSelection)input).toList())
               objects.add(((ObjectQueryResult)o).getObject());
            return new StructuredSelection(objects);
         }
      });
      getSite().setSelectionProvider(resultSelectionProvider);
      createResultsContextMenu();

      activateContext();
      createActions();
      contributeToActionBars();
      
      tabFolder.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            int index = tabFolder.getSelectionIndex();
            switch(index)
            {
               case 0:
                  text.setFocus();
                  break;
               case 1:
                  queryEditor.setFocus();
                  break;
            }
            settings.put("ObjectFinder.selectedTab", index);
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });

      try
      {
         tabFolder.setSelection(settings.getInt("ObjectFinder.selectedTab"));
      }
      catch(NumberFormatException e)
      {
         tabFolder.setSelection(0);
      }
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

      actionStartSearch = new Action("&Search", Activator.getImageDescriptor("icons/find.png")) {
         @Override
         public void run()
         {
            int i = tabFolder.getSelectionIndex();
            if (i == 0)
               startSearch();
            else if (i == 1)
               startQuery();
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

      actionSaveAs = new Action("&Save as predefined query...", SharedIcons.SAVE_AS) {
         @Override
         public void run()
         {
            saveQuery();
         }
      };

      actionExportToCSV = new ExportToCsvAction(this, results, true);
      actionExportAllToCSV = new ExportToCsvAction(this, results, false);
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
      manager.add(actionSaveAs);
      manager.add(new Separator());
      manager.add(actionExportAllToCSV);
   }

   /**
    * Fill local tool bar
    * @param manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionStartSearch);
      manager.add(actionSaveAs);
      manager.add(new Separator());
      manager.add(actionExportAllToCSV);
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
            ObjectContextMenu.fill(mgr, getSite(), resultSelectionProvider);
            if (((IStructuredSelection)results.getSelection()).size() == 1)
            {
               mgr.insertAfter(GroupMarkers.MB_PROPERTIES, actionShowObjectDetails);
            }
            manager.add(new Separator());
            manager.add(actionExportToCSV);
            manager.add(actionExportAllToCSV);
         }
      });

      // Create menu.
      Menu menu = manager.createContextMenu(results.getTable());
      results.getTable().setMenu(menu);

      // Register menu for extension.
      getSite().registerContextMenu(manager, resultSelectionProvider);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      switch(tabFolder.getSelectionIndex())
      {
         case 0:
            text.setFocus();
            break;
         case 1:
            queryEditor.setFocus();
            break;
      }
   }

   /**
    * Start object query
    */
   private void startQuery()
   {
      final String query = queryEditor.getText();
      new ConsoleJob("Query objects", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            if (!session.areObjectsSynchronized())
               session.syncObjects();

            try
            {
               final List<ObjectQueryResult> objects = session.queryObjectDetails(query, null, null, null, 0, true, 0);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     queryEditorMessage.hideMessage();
                     updateResultTable(objects);
                     queryEditor.setFocus();
                  }
               });
            }
            catch(NXCException e)
            {
               if ((e.getErrorCode() == RCC.NXSL_COMPILATION_ERROR) || (e.getErrorCode() == RCC.NXSL_EXECUTION_ERROR))
               {
                  final String errorMessage = e.getAdditionalInfo();
                  runInUIThread(new Runnable() {
                     @Override
                     public void run()
                     {
                        queryEditorMessage.showMessage(MessageBar.WARNING, errorMessage);
                     }
                  });
               }
               throw e;
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Object query failed";
         }
      }.start();
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
      
      List<Integer> zoneFilter = new ArrayList<Integer>();
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
   private void doSearch(final String searchString, final int mode, final List<Integer> classFilter, final List<Integer> zoneFilter, final InetAddress addrStart, final InetAddress addrEnd)
   {
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Find objects", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            if (!session.areObjectsSynchronized())
               session.syncObjects();

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
                         if (!zoneFilter.contains(node.getZoneId()))
                        	 return false;
                     }
                     if (object instanceof Sensor)
                     {
                        AbstractNode gateway = session.findObjectById(((Sensor)object).getGatewayId(), AbstractNode.class);
                        if (gateway != null && zoneFilter.contains(gateway.getZoneId()))
                           return false;
                     }
                     else if (object instanceof Interface)
                     {
                        AbstractNode parent = ((Interface)object).getParentNode();
                        if (parent != null && !zoneFilter.contains(parent.getZoneId()))
                           return false;
                     }
                     else if (object instanceof NetworkService)
                     {
                        AbstractNode parent = ((NetworkService)object).getParentNode();
                        if (parent != null && !zoneFilter.contains(parent.getZoneId()))
                           return false;
                     }  
                     else if (object instanceof VPNConnector)
                     {
                        AbstractNode parent = ((VPNConnector)object).getParentNode();
                        if (parent != null && !zoneFilter.contains(parent.getZoneId()))
                           return false;
                     }
                     else if (object instanceof AccessPoint)
                     {
                    	 AbstractNode parent = ((AccessPoint)object).getParentNode();
                         if (parent != null && !zoneFilter.contains(parent.getZoneId()))
                            return false;
                     }
                     else if (object instanceof GenericObject  && !zoneFilter.isEmpty())
                     {
                        AbstractObject children[] = ((GenericObject)object).getChildrenAsArray();
                        boolean match = false;
                        for(AbstractObject o : children)
                        {
                           if (o instanceof ZoneMember && zoneFilter.contains(((ZoneMember)o).getZoneId()))
                              match = true;
                        }
                        if (!match)
                        	return false;
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
                  resetResultTable();
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
         for(AbstractObject o : object.getAllChildren(AbstractObject.OBJECT_INTERFACE))
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
      IStructuredSelection selection = results.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final Object selectedElement = selection.getFirstElement();
      final AbstractObject object = (selectedElement instanceof ObjectQueryResult) ? ((ObjectQueryResult)selectedElement).getObject() : (AbstractObject)selectedElement;
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
    * Remove all columns except standard
    */
   private void resetResultTable()
   {
      TableColumn[] columns = results.getTable().getColumns();
      for(int i = ConsoleSharedData.getSession().isZoningEnabled() ? 6 : 5; i < columns.length; i++)
         columns[i].dispose();
   }

   /**
    * Update result table
    *
    * @param objects query result
    */
   private void updateResultTable(List<ObjectQueryResult> objects)
   {
      resetResultTable();

      Set<String> registeredProperties = new HashSet<>();
      for(ObjectQueryResult r : objects)
      {
         for(String n : r.getPropertyNames())
         {
            if (!registeredProperties.contains(n))
            {
               TableColumn tc = results.addColumn(n, 200);
               tc.setData("propertyName", n);
               registeredProperties.add(n);
            }
         }
      }

      results.setInput(objects);
   }

   /**
    * Save query as predefined
    */
   private void saveQuery()
   {
      if (tabFolder.getSelectionIndex() != 1)
         return;

      InputDialog dlg = new InputDialog(getSite().getShell(), "Save Object Query", "Query name", "", new IInputValidator() {
         @Override
         public String isValid(String newText)
         {
            if (newText.trim().isEmpty())
               return "Query name must not be empty";
            return null;
         }
      });
      if (dlg.open() != Window.OK)
         return;

      final ObjectQuery query = new ObjectQuery(dlg.getValue(), "", queryEditor.getText());
      new ConsoleJob("Save object query", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            boolean found = false;
            for(ObjectQuery q : session.getObjectQueries())
               if (q.getName().equalsIgnoreCase(query.getName()))
               {
                  query.setId(q.getId());
                  found = true;
                  break;
               }
            if (found)
            {
               final boolean[] overwrite = new boolean[1];
               getDisplay().syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     overwrite[0] = MessageDialogHelper.openQuestion(getSite().getShell(), "Confirm Overwrite",
                           String.format("Object query named \"%s\" already exists. Do you want to overwrite it?", query.getName()));
                  }
               });
               if (!overwrite[0])
                  return;
            }
            session.modifyObjectQuery(query);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  loadQueries();
                  queryModified = false;
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot save object query";
         }
      }.start();
   }

   /**
    * Load saved object queries from server
    */
   private void loadQueries()
   {
      ConsoleJob job = new ConsoleJob("Load saved queries", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<ObjectQuery> queries = session.getObjectQueries();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  queryList.setInput(queries);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot load saved object queries";
         }
      };
      job.setUser(false);
      job.start();
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
