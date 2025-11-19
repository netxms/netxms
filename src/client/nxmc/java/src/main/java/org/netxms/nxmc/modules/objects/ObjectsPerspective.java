/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.ServiceLoader;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MenuEvent;
import org.eclipse.swt.events.MenuListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectFilter;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.views.ViewWithContext;
import org.netxms.nxmc.base.widgets.RoundedLabel;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.AgentConfigurationEditor;
import org.netxms.nxmc.modules.alarms.views.AlarmsView;
import org.netxms.nxmc.modules.assetmanagement.LinkAssetToObjectAction;
import org.netxms.nxmc.modules.assetmanagement.LinkObjectToAssetAction;
import org.netxms.nxmc.modules.assetmanagement.UnlinkAssetFromObjectAction;
import org.netxms.nxmc.modules.assetmanagement.UnlinkObjectFromAssetAction;
import org.netxms.nxmc.modules.assetmanagement.views.AssetSummaryView;
import org.netxms.nxmc.modules.assetmanagement.views.AssetView;
import org.netxms.nxmc.modules.businessservice.views.BusinessServiceAvailabilityView;
import org.netxms.nxmc.modules.businessservice.views.BusinessServiceChecksView;
import org.netxms.nxmc.modules.dashboards.ExportDashboardAction;
import org.netxms.nxmc.modules.dashboards.ImportDashboardAction;
import org.netxms.nxmc.modules.dashboards.views.ContextDashboardView;
import org.netxms.nxmc.modules.dashboards.views.DashboardView;
import org.netxms.nxmc.modules.datacollection.views.DataCollectionView;
import org.netxms.nxmc.modules.datacollection.views.PerformanceView;
import org.netxms.nxmc.modules.datacollection.views.PolicyListView;
import org.netxms.nxmc.modules.datacollection.views.SummaryDataCollectionView;
import org.netxms.nxmc.modules.datacollection.views.ThresholdSummaryView;
import org.netxms.nxmc.modules.filemanager.views.AgentFileManager;
import org.netxms.nxmc.modules.networkmaps.views.ContextPredefinedMapView;
import org.netxms.nxmc.modules.networkmaps.views.PredefinedMapView;
import org.netxms.nxmc.modules.nxsl.views.ScriptExecutorView;
import org.netxms.nxmc.modules.objects.actions.ForcedPolicyDeploymentAction;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.modules.objects.dialogs.ObjectViewManagerDialog;
import org.netxms.nxmc.modules.objects.views.AccessPointsView;
import org.netxms.nxmc.modules.objects.views.AddressMapView;
import org.netxms.nxmc.modules.objects.views.ArpCacheView;
import org.netxms.nxmc.modules.objects.views.ChassisView;
import org.netxms.nxmc.modules.objects.views.CommentsView;
import org.netxms.nxmc.modules.objects.views.DeviceView;
import org.netxms.nxmc.modules.objects.views.Dot1xStatusView;
import org.netxms.nxmc.modules.objects.views.HardwareInventoryView;
import org.netxms.nxmc.modules.objects.views.InterfacesView;
import org.netxms.nxmc.modules.objects.views.MaintenanceJournalView;
import org.netxms.nxmc.modules.objects.views.NetworkServiceView;
import org.netxms.nxmc.modules.objects.views.NodesView;
import org.netxms.nxmc.modules.objects.views.OSPFView;
import org.netxms.nxmc.modules.objects.views.ObjectBrowser;
import org.netxms.nxmc.modules.objects.views.ObjectOverviewView;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.views.PhysicalLinkView;
import org.netxms.nxmc.modules.objects.views.PortView;
import org.netxms.nxmc.modules.objects.views.ProcessesView;
import org.netxms.nxmc.modules.objects.views.RackView;
import org.netxms.nxmc.modules.objects.views.RadioInterfacesAP;
import org.netxms.nxmc.modules.objects.views.RadioInterfacesController;
import org.netxms.nxmc.modules.objects.views.RemoteControlView;
import org.netxms.nxmc.modules.objects.views.ResourcesView;
import org.netxms.nxmc.modules.objects.views.RoutingTableView;
import org.netxms.nxmc.modules.objects.views.ScreenshotView;
import org.netxms.nxmc.modules.objects.views.ServicesView;
import org.netxms.nxmc.modules.objects.views.SoftwareInventoryView;
import org.netxms.nxmc.modules.objects.views.StatusMapView;
import org.netxms.nxmc.modules.objects.views.SwitchForwardingDatabaseView;
import org.netxms.nxmc.modules.objects.views.TemplateTargets;
import org.netxms.nxmc.modules.objects.views.UserSessionsView;
import org.netxms.nxmc.modules.objects.views.VpnView;
import org.netxms.nxmc.modules.objects.views.WirelessStations;
import org.netxms.nxmc.modules.objects.views.elements.Comments;
import org.netxms.nxmc.modules.objects.views.elements.ObjectInfo;
import org.netxms.nxmc.modules.objects.views.elements.PollStates;
import org.netxms.nxmc.modules.snmp.views.MibExplorer;
import org.netxms.nxmc.modules.worldmap.views.ObjectGeoLocationView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.services.ObjectActionDescriptor;
import org.netxms.nxmc.services.ObjectViewDescriptor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Object browser perspective
 */
public abstract class ObjectsPerspective extends Perspective implements ISelectionProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectsPerspective.class);

   private ObjectBrowser objectBrowser;
   private SubtreeType subtreeType;
   private ObjectFilter objectFilter;
   private StructuredSelection selection = new StructuredSelection();
   private Composite headerArea;
   private Label objectName;
   private RoundedLabel objectStatus;
   private Composite objectDetails;
   private ObjectInfo objectGeneralInfo;
   private PollStates objectPollState;
   private Comments objectComments;
   private ToolBar objectToolBar;
   private ToolBar objectMenuBar;
   private ToolBar perspectiveToolBar;
   private Label expandButton;
   private Image imageEditConfig;
   private Image imageExecuteScript;
   private Image imageOpenMibExplorer;
   private Image imageTakeScreenshot;
   private Image imageRemoteControl;
   private Image imageManageViews;
   private List<ObjectAction<?>> actionContributions = new ArrayList<>();
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<>();

   /**
    * Create new object perspective. If object filter is provided, as top level objects will be selected objects that passed filter
    * themselves and does not have accessible parents at all or none of their parents passed filter.
    *
    * @param id perspective ID
    * @param name perspective name
    * @param image perspective icon
    * @param subtreeType object subtree type
    * @param objectFilter additional filter for top level objects (optional, can be null)
    */
   protected ObjectsPerspective(String id, String name, Image image, SubtreeType subtreeType, ObjectFilter objectFilter)
   {
      super(id, name, image);
      this.subtreeType = subtreeType;
      this.objectFilter = objectFilter;
      imageEditConfig = ResourceManager.getImage("icons/object-views/agent-config.png");
      imageExecuteScript = ResourceManager.getImage("icons/object-views/script-executor.png");
      imageTakeScreenshot = ResourceManager.getImage("icons/screenshot.png");
      imageRemoteControl = ResourceManager.getImage("icons/object-views/remote-desktop.png");
      imageManageViews = ResourceManager.getImage("icons/perspective-config.png");
      imageOpenMibExplorer = ResourceManager.getImage("icons/object-views/mibexplorer.gif");
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = true;
      configuration.enableNavigationHistory = true;
      configuration.hasSupplementalArea = false;
      configuration.multiViewNavigationArea = false;
      configuration.multiViewMainArea = true;
      configuration.hasHeaderArea = true;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      objectBrowser = new ObjectBrowser(getName(), null, subtreeType, objectFilter);
      addNavigationView(objectBrowser);

      addMainView(new AccessPointsView());
      addMainView(new AddressMapView());
      addMainView(new AgentFileManager());
      addMainView(new AlarmsView());
      addMainView(new ArpCacheView());
      addMainView(new AssetSummaryView());
      addMainView(new AssetView());      
      addMainView(new BusinessServiceAvailabilityView());
      addMainView(new BusinessServiceChecksView());
      addMainView(new ChassisView());
      addMainView(new CommentsView());
      addMainView(new DashboardView());
      addMainView(new DataCollectionView());
      addMainView(new DeviceView());
      addMainView(new Dot1xStatusView());
      addMainView(new HardwareInventoryView());
      addMainView(new InterfacesView());
      addMainView(new MaintenanceJournalView());
      addMainView(new NetworkServiceView());
      addMainView(new NodesView());
      addMainView(new ObjectGeoLocationView());
      addMainView(new ObjectOverviewView());
      addMainView(new OSPFView());
      addMainView(new PerformanceView());
      addMainView(new PhysicalLinkView());
      addMainView(new PolicyListView());
      addMainView(new PortView());
      addMainView(new PredefinedMapView());
      addMainView(new ProcessesView());
      addMainView(new RackView());
      addMainView(new RadioInterfacesAP());
      addMainView(new RadioInterfacesController());
      addMainView(new ResourcesView());
      addMainView(new RoutingTableView());
      addMainView(new ServicesView());
      addMainView(new SoftwareInventoryView());
      addMainView(new StatusMapView());
      addMainView(new SummaryDataCollectionView());
      addMainView(new SwitchForwardingDatabaseView());
      addMainView(new TemplateTargets());      
      addMainView(new ThresholdSummaryView());
      addMainView(new UserSessionsView());
      addMainView(new VpnView());
      addMainView(new WirelessStations());

      NXCSession session = Registry.getSession();

      ServiceLoader<ObjectViewDescriptor> viewLoader = ServiceLoader.load(ObjectViewDescriptor.class, getClass().getClassLoader());
      for(ObjectViewDescriptor v : viewLoader)
      {
         String componentId = v.getRequiredComponentId();
         if ((componentId == null) || session.isServerComponentRegistered(componentId))
            addMainView(v.createView());
      }

      // Standard action contributions
      actionContributions.add(new ForcedPolicyDeploymentAction(new ViewPlacement(this), this));
      actionContributions.add(new LinkAssetToObjectAction(new ViewPlacement(this), this));
      actionContributions.add(new UnlinkAssetFromObjectAction(new ViewPlacement(this), this));
      actionContributions.add(new LinkObjectToAssetAction(new ViewPlacement(this), this));
      actionContributions.add(new UnlinkObjectFromAssetAction(new ViewPlacement(this), this));
      actionContributions.add(new ExportDashboardAction(new ViewPlacement(this), this));
      actionContributions.add(new ImportDashboardAction(new ViewPlacement(this), this));

      ViewPlacement viewPlacement = new ViewPlacement(this);
      ServiceLoader<ObjectActionDescriptor> actionLoader = ServiceLoader.load(ObjectActionDescriptor.class, getClass().getClassLoader());
      for(ObjectActionDescriptor a : actionLoader)
      {
         String componentId = a.getRequiredComponentId();
         if ((componentId == null) || session.isServerComponentRegistered(componentId))
            actionContributions.add(a.createAction(viewPlacement, this));
      }

      // Add session listener
      session.addListener((n) -> {
         if (n.getCode() != SessionNotification.OBJECT_CHANGED)
            return;
         AbstractObject context = (AbstractObject)getContext();
         if ((context != null) && (n.getSubCode() == context.getObjectId()))
         {
            getWindow().getShell().getDisplay().asyncExec(() -> updateContext(n.getObject()));
         }
      });
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#createHeaderArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createHeaderArea(Composite parent)
   {
      headerArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 9;
      layout.verticalSpacing = 0;
      layout.marginHeight = 0;
      layout.marginTop = 3;
      headerArea.setLayout(layout);

      objectName = new Label(headerArea, SWT.LEFT);
      objectName.setFont(JFaceResources.getBannerFont());
      Menu objectNameMenu = new Menu(objectName);
      MenuItem menuItem = new MenuItem(objectNameMenu, SWT.PUSH);
      menuItem.setText("&Copy");
      menuItem.setImage(SharedIcons.IMG_COPY);
      menuItem.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            WidgetHelper.copyToClipboard(objectName.getText());
         }
      });
      objectName.setMenu(objectNameMenu);

      Label separator = new Label(headerArea, SWT.SEPARATOR | SWT.VERTICAL);
      GridData gd = new GridData(SWT.CENTER, SWT.FILL, false, true);
      gd.heightHint = 28;
      separator.setLayoutData(gd);

      objectStatus = new RoundedLabel(headerArea);
      objectStatus.setToolTipText(i18n.tr("Object status"));
      gd = new GridData(SWT.FILL, SWT.FILL, false, true);
      gd.widthHint = WidgetHelper.getTextWidth(objectStatus, StatusDisplayInfo.getStatusText(ObjectStatus.UNMANAGED) + 10);
      objectStatus.setLayoutData(gd);

      separator = new Label(headerArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData(SWT.CENTER, SWT.FILL, false, true);
      gd.heightHint = 28;
      separator.setLayoutData(gd);

      objectToolBar = new ToolBar(headerArea, SWT.FLAT | SWT.HORIZONTAL | SWT.RIGHT);

      separator = new Label(headerArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData(SWT.CENTER, SWT.FILL, false, true);
      gd.heightHint = 28;
      separator.setLayoutData(gd);

      objectMenuBar = new ToolBar(headerArea, SWT.FLAT | SWT.HORIZONTAL | SWT.RIGHT);

      perspectiveToolBar = new ToolBar(headerArea, SWT.FLAT | SWT.HORIZONTAL | SWT.RIGHT);
      gd = new GridData(SWT.RIGHT, SWT.CENTER, true, false);
      perspectiveToolBar.setLayoutData(gd);

      ToolItem item = new ToolItem(perspectiveToolBar, SWT.DROP_DOWN);
      item.setImage(imageManageViews);
      item.setToolTipText("Manage views");
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if (e.detail == SWT.ARROW)
            {
               ToolItem item = (ToolItem)e.widget;
               Rectangle rect = item.getBounds();
               Point pt = item.getParent().toDisplay(new Point(rect.x, rect.y));
               final Menu closedViewMenu = createClosedViewMenu();
               closedViewMenu.setLocation(pt.x, pt.y + rect.height);
               closedViewMenu.setVisible(true);
            }
            else
            {
               openViewManager();
            }
         }
      });

      expandButton = new Label(headerArea, SWT.NONE);
      expandButton.setBackground(headerArea.getBackground());
      expandButton.setImage(SharedIcons.IMG_EXPAND);
      expandButton.setToolTipText(i18n.tr("Show additional object information"));
      expandButton.setCursor(parent.getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      expandButton.addMouseListener(new MouseAdapter() {
         @Override
         public void mouseUp(MouseEvent e)
         {
            boolean show = PreferenceStore.getInstance().getAsBoolean("ObjectPerspective.showObjectDetails", false);
            PreferenceStore.getInstance().set("ObjectPerspective.showObjectDetails", !show);
         }
      });
      gd = new GridData(SWT.RIGHT, SWT.CENTER, false, false);
      expandButton.setLayoutData(gd);

      PreferenceStore.getInstance().addPropertyChangeListener((event) -> {
         if (event.getProperty().equals("ObjectPerspective.showObjectDetails"))
         {
            showObjectDetails(Boolean.parseBoolean((String)event.getNewValue()));
         }
      });

      if (PreferenceStore.getInstance().getAsBoolean("ObjectPerspective.showObjectDetails", false))
         showObjectDetails(true);
   }

   /**
    * Create view pinning menu
    *
    * @return view pinning menu
    */
   private Menu createClosedViewMenu()
   {
      Menu menu = new Menu(perspectiveToolBar);
      menu.addMenuListener(new MenuListener() {
         @Override
         public void menuShown(MenuEvent e)
         {
            createViewMenuItem(menu);
         }
         
         @Override
         public void menuHidden(MenuEvent e)
         {
         }
      });
      return menu;
   }

   /**
    * Create item for view pinning menu
    *
    * @param menu menu
    * @param location pin location
    * @param name menu item name
    */
   private void createViewMenuItem(Menu menu)
   {
      List<View> objectViews = new ArrayList<>();
      for(View v : getAllMainViews())
      {
         if (!v.isCloseable() && (v instanceof ObjectView) && 
               ((ObjectView)v).isHidden() && (((ViewWithContext)v).isValidForContext(selection.getFirstElement())))
            objectViews.add(v);
      }
      objectViews.sort((v1, v2) -> v1.getName().compareToIgnoreCase(v2.getName()));
      for (View v : objectViews)
      {
         MenuItem item = new MenuItem(menu, SWT.PUSH);
         item.setText(v.getName());
         item.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               PreferenceStore preferenceStore = PreferenceStore.getInstance();
               preferenceStore.set("HideView." + v.getBaseId(), false);
               updateViewSet();
            }
         });
      }
   }

   /**
    * Show/hide object details
    *
    * @param show true to show
    */
   private void showObjectDetails(boolean show)
   {
      if (show && (objectDetails == null))
      {
         objectDetails = new Composite(headerArea, SWT.NONE);
         GridData gd = new GridData(SWT.FILL, SWT.FILL, true, false);
         gd.horizontalSpan = ((GridLayout)headerArea.getLayout()).numColumns;
         objectDetails.setLayoutData(gd);

         GridLayout layout = new GridLayout();
         layout.numColumns = 3;
         layout.marginHeight = 0;
         layout.marginWidth = 0;
         objectDetails.setLayout(layout);

         objectGeneralInfo = new ObjectInfo(objectDetails, null, null);
         objectGeneralInfo.setVerticalAlignment(SWT.FILL);

         objectPollState = new PollStates(objectDetails, objectGeneralInfo, null);
         objectPollState.setVerticalAlignment(SWT.FILL);

         objectComments = new Comments(objectDetails, objectPollState, null);
         objectComments.setVerticalAlignment(SWT.FILL);

         AbstractObject object = (AbstractObject)selection.getFirstElement();
         if (object != null)
         {
            objectGeneralInfo.setObject(object);
            if (objectPollState.isApplicableForObject(object))
               objectPollState.setObject(object);
            objectComments.setObject(object);
         }
      }
      else if (!show && (objectDetails != null))
      {
         objectDetails.dispose();
         objectDetails = null;
      }
      expandButton.setImage(show ? SharedIcons.IMG_COLLAPSE : SharedIcons.IMG_EXPAND);
      expandButton.setToolTipText(show ? i18n.tr("Hide additional object information") : i18n.tr("Show additional object information"));
      layoutMainArea();
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#navigationSelectionChanged(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   protected void navigationSelectionChanged(IStructuredSelection selection)
   {
      super.navigationSelectionChanged(selection);

      if (selection.getFirstElement() instanceof AbstractObject)
      {
         AbstractObject object = (AbstractObject)selection.getFirstElement();
         this.selection = new StructuredSelection(object);
         objectName.setText(object.getNameWithAlias());

         if (object.isInMaintenanceMode())
            objectStatus.setText(StatusDisplayInfo.getStatusText(object.getStatus()) + i18n.tr(" (maintenance)"));
         else
            objectStatus.setText(StatusDisplayInfo.getStatusText(object.getStatus()));
         objectStatus.setLabelBackground(StatusDisplayInfo.getStatusBackgroundColor(object.getStatus()));

         if (objectDetails != null)
            updateObjectDetails(object);

         updateObjectToolBar(object);
         updateObjectMenuBar(object);
         updateContextDashboardsAndMaps(object);
      }
      else
      {
         this.selection = new StructuredSelection();
         objectName.setText("");
         objectStatus.setText("");
         objectStatus.setLabelBackground(null);
      }
      headerArea.layout();

      if (!selectionListeners.isEmpty())
      {
         SelectionChangedEvent e = new SelectionChangedEvent(this, selection);
         for(ISelectionChangedListener l : selectionListeners)
            l.selectionChanged(e);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#updateContext(java.lang.Object)
    */
   @Override
   protected void updateContext(Object context)
   {
      super.updateContext(context);

      AbstractObject object = (AbstractObject)context;
      objectName.setText(object.getNameWithAlias());

      if (object.isInMaintenanceMode())
         objectStatus.setText(StatusDisplayInfo.getStatusText(object.getStatus()) + i18n.tr(" (maintenance)"));
      else
         objectStatus.setText(StatusDisplayInfo.getStatusText(object.getStatus()));
      objectStatus.setLabelBackground(StatusDisplayInfo.getStatusBackgroundColor(object.getStatus()));

      updateObjectToolBar(object);
      updateObjectMenuBar(object);
      updateContextDashboardsAndMaps(object);

      if (objectDetails != null)
         updateObjectDetails(object);
      else
         headerArea.layout();
   }

   /**
    * Update object details.
    *
    * @param object currently selected object
    */
   private void updateObjectDetails(AbstractObject object)
   {
      objectGeneralInfo.setObject(object);
      if (objectPollState.isApplicableForObject(object))
      {
         objectPollState.setObject(object);
         objectPollState.fixPlacement();
      }
      else
      {
         objectPollState.dispose();
         objectComments.fixPlacement();
      }
      objectComments.setObject(object);
      layoutMainArea();
   }

   /**
    * Update object toolbar
    *
    * @param object selected object
    */
   private void updateObjectToolBar(final AbstractObject object)
   {
      for(ToolItem item : objectToolBar.getItems())
         item.dispose();

      addObjectToolBarItem(i18n.tr("Properties"), SharedIcons.IMG_PROPERTIES, () -> ObjectPropertiesManager.openObjectPropertiesDialog(object, getWindow().getShell(), getMessageArea()));
      addObjectToolBarItem(i18n.tr("Execute script"), imageExecuteScript, () -> addMainView(new ScriptExecutorView(object.getObjectId(), object.getObjectId()), true, false));

      if ((object instanceof Node) && ((Node)object).hasAgent())
      {
         addObjectToolBarItem(i18n.tr("Edit agent configuration"), imageEditConfig, () -> addMainView(new AgentConfigurationEditor((Node)object, object.getObjectId()), true, false));
         if (((Node)object).getPlatformName().startsWith("windows-"))
         {
            addObjectToolBarItem(i18n.tr("Take screenshot"), imageTakeScreenshot, () -> addMainView(new ScreenshotView((Node)object, null, null, 0), true, false));
         }
      }

      if ((object instanceof Node) && ((Node)object).hasVNC())
      {
         addObjectToolBarItem(i18n.tr("Remote control"), imageRemoteControl, () -> addMainView(new RemoteControlView((Node)object, 0), true, false));
      }

      if ((object instanceof Node) && ((Node)object).hasSnmpAgent())
      {
         addObjectToolBarItem(i18n.tr("&MIB Explorer"), imageOpenMibExplorer, () -> addMainView(new MibExplorer(object.getObjectId(), object.getObjectId(), false), true, false));
      }

      for(ObjectAction<?> a : actionContributions)
      {
         if (a.isValidForSelection(selection))
            addObjectToolBarItem(a);
      }
   }

   /**
    * Add object toolbar item.
    *
    * @param name tooltip text
    * @param icon icon
    * @param handler selection handler
    */
   private void addObjectToolBarItem(String name, Image icon, final Runnable handler)
   {
      ToolItem item = new ToolItem(objectToolBar, SWT.PUSH);
      item.setImage(icon);
      item.setToolTipText(name);
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            handler.run();
         }
      });
   }

   /**
    * Add object toolbar item.
    *
    * @param metric tooltip text
    * @param icon icon
    * @param handler selection handler
    */
   private void addObjectToolBarItem(final Action action)
   {
      ToolItem item = new ToolItem(objectToolBar, SWT.PUSH);
      final Image icon = (action.getImageDescriptor() != null) ? action.getImageDescriptor().createImage() : ResourceManager.getImage("icons/empty.png");
      item.setImage(icon);
      item.setToolTipText(action.getText());
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            action.run();
         }
      });
      item.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            icon.dispose();
         }
      });
   }

   /**
    * Update object menu bar
    *
    * @param object selected object
    */
   private void updateObjectMenuBar(final AbstractObject object)
   {
      for(ToolItem item : objectMenuBar.getItems())
         item.dispose();

      addObjectMenu(i18n.tr("Tools"), ObjectMenuFactory.createToolsMenu(new StructuredSelection(object), object.getObjectId(), null, objectToolBar, new ViewPlacement(this)));
      addObjectMenu(i18n.tr("Graphs"), ObjectMenuFactory.createGraphTemplatesMenu(new StructuredSelection(object), object.getObjectId(), null, objectToolBar, new ViewPlacement(this)));
      addObjectMenu(i18n.tr("Summary Tables"), ObjectMenuFactory.createSummaryTableMenu(new StructuredSelection(object), object.getObjectId(), null, objectToolBar, new ViewPlacement(this)));
      addObjectMenu(i18n.tr("Poll"), ObjectMenuFactory.createPollMenu(new StructuredSelection(object), object.getObjectId(), null, objectToolBar, new ViewPlacement(this)));
      addObjectMenu(i18n.tr("Dashboards"), ObjectMenuFactory.createDashboardsMenu(new StructuredSelection(object), object.getObjectId(), null, objectToolBar, new ViewPlacement(this)));
      addObjectMenu(i18n.tr("Create"), new ObjectCreateMenuManager(getWindow().getShell(), null, object));
      addObjectMenu(i18n.tr("Logs"), new ObjectLogMenuManager(object, 0, new ViewPlacement(this)));
   }

   /**
    * Add drop-down menu for current object
    *
    * @param name menu name
    * @param menu menu
    */
   private void addObjectMenu(String name, final Menu menu)
   {
      if ((menu != null) && (menu.getItemCount() != 0))
         createMenuToolItem(name, null, menu);
   }

   /**
    * Add drop-down menu for current object
    *
    * @param name menu name
    * @param menu menu
    */
   private void addObjectMenu(String name, final MenuManager menuManager)
   {
      if ((menuManager != null) && !menuManager.isEmpty())
         createMenuToolItem(name, menuManager, null);
   }

   /**
    * Create toolbar item that will show drop down menu.
    *
    * @param name item name
    * @param menuManager menu manager that will provide menu or null if menu provided directly
    * @param menu menu to show or null if menu provided by manager
    */
   private void createMenuToolItem(String name, final MenuManager menuManager, final Menu menu)
   {
      ToolItem item = new ToolItem(objectMenuBar, SWT.PUSH);
      item.setText("  " + name + " \u25BE  ");
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            Rectangle rect = item.getBounds();
            Point pt = new Point(rect.x, rect.y + rect.height);
            pt = objectMenuBar.toDisplay(pt);
            Menu m = (menuManager != null) ? menuManager.createContextMenu(objectToolBar) : menu;
            m.setLocation(pt.x, pt.y);
            m.setVisible(true);
         }
      });
   }

   /**
    * Update context dashboard and map views after object selection change.
    *
    * @param object selected object
    */
   private void updateContextDashboardsAndMaps(AbstractObject object)
   {
      if (!((object instanceof DataCollectionTarget) || (object instanceof Container) || (object instanceof Rack) || (object instanceof EntireNetwork) || (object instanceof ServiceRoot) ||
            (object instanceof Subnet) || (object instanceof Zone) || (object instanceof Condition)))
         return;

      for(AbstractObject d : object.getDashboards(true))
      {
         if ((((Dashboard)d).getFlags() & Dashboard.SHOW_AS_OBJECT_VIEW) == 0)
         {
            String viewId = "objects.context.dashboard." + d.getObjectId();
            removeMainView(viewId);
            continue;
         }

         String viewId = "objects.context.dashboard." + d.getObjectId();
         if (findMainView(viewId) == null)
         {
            addMainView(new ContextDashboardView((Dashboard)d));
         }
      }

      for(AbstractObject m : object.getNetworkMaps(true))
      {
         if ((((NetworkMap)m).getFlags() & NetworkMap.MF_SHOW_AS_OBJECT_VIEW) == 0)
         {
            String viewId = "objects.context.network-map." + m.getObjectId();
            removeMainView(viewId);
            continue;
         }

         String viewId = "objects.context.network-map." + m.getObjectId();
         if (findMainView(viewId) == null)
         {
            addMainView(new ContextPredefinedMapView((NetworkMap)m));
         }
      }
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
   }

   /**
    * Show object object 
    * 
    * @param object object to be shown 
    * @param dciId DCI id or 0 if object only should be showed 
    * @return true if object found in the view
    */
   public boolean showObject(AbstractObject object, long dciId)
   {
      if (!ObjectBrowser.calculateClassFilter(subtreeType).contains(object.getObjectClass()))
         return false;

      Registry.getMainWindow().switchToPerspective(getId());
      if (!objectBrowser.selectObject(object))
         return false;

      if ((dciId != 0) && showMainView(DataCollectionView.VIEW_ID))
      {
         View dataCollectionView = findMainView(DataCollectionView.VIEW_ID);
         ((DataCollectionView)dataCollectionView).selectDci(dciId);
      }
      return true;
   }

   /**
    * Open view manager
    */
   private void openViewManager()
   {
      List<View> objectViews = new ArrayList<>();
      for(View v : getAllMainViews())
      {
         if (!v.isCloseable() && (v instanceof ObjectView))
            objectViews.add(v);
      }
      objectViews.sort((v1, v2) -> v1.getName().compareToIgnoreCase(v2.getName()));
      ObjectViewManagerDialog dlg = new ObjectViewManagerDialog(getWindow().getShell(), objectViews);
      if (dlg.open() == Window.OK)
      {
         updateViewSet();
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      if (objectBrowser == null)
         return;

      IStructuredSelection selection = (IStructuredSelection)objectBrowser.getSelectionProvider().getSelection();
      if (!selection.isEmpty())
         memento.set("ObjectPerspective." + getId() + ".SelectedObject", ((AbstractObject)selection.getFirstElement()).getObjectId());
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento)
   {
      long id = memento.getAsLong("ObjectPerspective." + getId() + ".SelectedObject", 0);
      if (id != 0)
      {
         NXCSession session = Registry.getSession();
         AbstractObject object = session.findObjectById(id, false);
         if (object != null)
            objectBrowser.selectObject(object);
      }
   }
}
