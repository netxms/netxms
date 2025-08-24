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
package org.netxms.nxmc.modules.networkmaps.views;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.util.LocalSelectionTransfer;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.ViewerDropAdapter;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.dnd.DND;
import org.eclipse.swt.dnd.DropTargetEvent;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.dnd.TransferData;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.maps.MapLayoutAlgorithm;
import org.netxms.client.maps.MapType;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.maps.elements.NetworkMapTextBox;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyDialog;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.modules.imagelibrary.ImageUpdateListener;
import org.netxms.nxmc.modules.imagelibrary.dialogs.ImageSelectionDialog;
import org.netxms.nxmc.modules.networkmaps.algorithms.ManualLayout;
import org.netxms.nxmc.modules.networkmaps.dialogs.EditGroupBoxDialog;
import org.netxms.nxmc.modules.networkmaps.propertypages.DCIContainerDataSources;
import org.netxms.nxmc.modules.networkmaps.propertypages.DCIContainerGeneral;
import org.netxms.nxmc.modules.networkmaps.propertypages.DCIImageGeneral;
import org.netxms.nxmc.modules.networkmaps.propertypages.DCIImageRules;
import org.netxms.nxmc.modules.networkmaps.propertypages.LinkColor;
import org.netxms.nxmc.modules.networkmaps.propertypages.LinkDataSources;
import org.netxms.nxmc.modules.networkmaps.propertypages.LinkGeneral;
import org.netxms.nxmc.modules.networkmaps.propertypages.TextBoxGeneral;
import org.netxms.nxmc.modules.networkmaps.views.helpers.LinkEditor;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.MapObjectSyncer;
import org.netxms.nxmc.modules.objects.ObjectPropertiesManager;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * View for predefined map
 */
public class PredefinedMapView extends AbstractNetworkMapView implements ImageUpdateListener
{
   private final I18n i18n = LocalizationHelper.getI18n(PredefinedMapView.class);

	private Action actionAddObject;
   private Action actionAddObjectMenu;
	private Action actionAddDCIContainer;
	private Action actionLinkObjects;
	private Action actionAddGroupBox;
	private Action actionAddImage;
	private Action actionRemove;
	private Action actionDCIContainerProperties;
	private Action actionDCIImageProperties;
	private Action actionMapProperties;
	private Action actionLinkProperties;
	private Action actionAddDCIImage;
	private Action actionAddTextBox;
	private Action actionTextBoxProperties;
	private Action actionGroupBoxProperties;
   private Action actionImageProperties;
   private Action actionAutoLinkNodes;
	private Color defaultLinkColor = null;
   private boolean disableGeolocationBackground = false;
   private boolean disableLinkTextAutoUpdate = true;
   private Map<Long, Boolean> readOnlyFlagsCache = new HashMap<>();
   private Set<NetworkMapElement> movedElementList = new HashSet<>();
   private Set<NetworkMapLink> changedLinkList = new HashSet<>();
   private MapObjectSyncer mapObjectSyncer = null;
   private int mapWidth;
   private int mapHeight;
   private int updateWarningId = 0;

	/**
	 * Create predefined map view
	 */
	public PredefinedMapView()
	{
      super(LocalizationHelper.getI18n(PredefinedMapView.class).tr("Map"), ResourceManager.getImageDescriptor("icons/object-views/netmap.png"), "objects.predefined-map");
	}

   /**
    * Create new map view with ID extended by given sub ID. Intended for use by subclasses implementing ad-hoc views.
    * 
    * @param id view ID
    */
   protected PredefinedMapView(String id)
   {
      super(LocalizationHelper.getI18n(PredefinedMapView.class).tr("Map"), ResourceManager.getImageDescriptor("icons/object-views/netmap.png"), id);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof NetworkMap);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 1;
   }
   
   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {  
      if (bendpointEditor != null)
         bendpointEditor.stop();
      viewer.resetRefreshBlock();
      saveObjectPositions();

      if (oldContext != null)
      {
         saveZoom((AbstractObject)oldContext);
      }

      updateWarningId = 0; // messages are cleared on context change

      super.contextChanged(oldContext, newContext);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(final AbstractObject object)
   {
      Boolean cachedFlag = readOnlyFlagsCache.get(object.getObjectId());
      actionLock.setChecked(true);
      lockObjectMove(true);
      
      if (cachedFlag != null)
      {
         readOnly = cachedFlag;
         reconfigureViewer();
         syncObjects();
      }
      else
      {
         readOnly = true;
         Job job = new Job("Get map effective rights", this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final Boolean readOnly = ((object.getEffectiveRights() & UserAccessRights.OBJECT_ACCESS_MODIFY) == 0);
               runInUIThread(() -> {
                  readOnlyFlagsCache.put(object.getObjectId(), readOnly);
                  PredefinedMapView.this.readOnly = readOnly;
                  reconfigureViewer();
                  updateToolBar();
                  updateMenu();
                  syncObjects();
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot get effective rights for map");
            }
         };
         job.setUser(false);
         job.start();
      }
      loadZoom(object);
      updateWarningMessage(object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      if (viewer.isRefreshBlocked())
         return;

      saveObjectPositions();
      super.onObjectUpdate(object);
      reconfigureViewer((NetworkMap)object);
      syncObjects();
      updateWarningMessage(object);
   }

   /**
    * Update synchronization warning message.
    *
    * @param object current object
    */
   private void updateWarningMessage(AbstractObject object)
   {
      if (((NetworkMap)object).isUpdateFailed())
      {
         if (updateWarningId == 0)
         {
            updateWarningId = addMessage(MessageArea.WARNING, i18n.tr("Automatic map updates temporary disabled because topology information is not available for at least one of the seeds"), true);
         }
      }
      else if (updateWarningId != 0)
      {
         deleteMessage(updateWarningId);
         updateWarningId = 0;
      }
   }

   /**
    * Reconfigure viewer for given map object
    * 
    * @param mapObject map object
    */
   private void reconfigureViewer(NetworkMap mapObject)
   {      
      mapWidth = mapObject.getWidth();
      mapHeight = mapObject.getHeight();
      int width = mapWidth == 0 ? session.getNetworkMapDefaultWidth() : mapWidth;
      int height = mapHeight == 0 ? session.getNetworkMapDefaultHeight() : mapHeight;

      if (mapObject.isFitToScreen())
      {
         setMapSize(-1, -1);         
      }
      else
      {
         setMapSize(width, height);
      }

      if ((mapObject.getBackground() != null) && (mapObject.getBackground().compareTo(NXCommon.EMPTY_GUID) != 0))
      {
         if (mapObject.getBackground().equals(org.netxms.client.objects.NetworkMap.GEOMAP_BACKGROUND))
         {
            if (!disableGeolocationBackground)
               viewer.setBackgroundImage(mapObject.getBackgroundLocation(), mapObject.getBackgroundZoom());
         }
         else
         {
            Image backgroundImage = ImageProvider.getInstance().getImage(mapObject.getBackground());
            if ((backgroundImage != null) && !mapObject.isFitBackgroundImage())
            {
               Rectangle bounds = backgroundImage.getBounds();
               if ((mapWidth == 0) && (width < bounds.width))
                  width = bounds.width;
               if ((mapHeight == 0) && (height < bounds.height))
                  height = bounds.height;
            }
            viewer.setBackgroundImage(backgroundImage, mapObject.isCenterBackgroundImage(), mapObject.isFitBackgroundImage());
         }
      }
      else
      {
         viewer.setBackgroundImage(null, false, false);
      }

      viewer.setBackgroundColor(ColorConverter.rgbFromInt(mapObject.getBackgroundColor()));

      setConnectionRouter(mapObject.getDefaultLinkRouting(), false);

      if (defaultLinkColor != null)
         defaultLinkColor.dispose();
      if (mapObject.getDefaultLinkColor() >= 0)
         defaultLinkColor = new Color(viewer.getControl().getDisplay(), ColorConverter.rgbFromInt(mapObject.getDefaultLinkColor()));
      else
         defaultLinkColor = null;
      labelProvider.setDefaultLinkColor(defaultLinkColor);

      if (mapObject.getLayout() == MapLayoutAlgorithm.MANUAL)
      {
         automaticLayoutEnabled = false;
         viewer.setLayoutAlgorithm(new ManualLayout());
      }
      else
      {
         automaticLayoutEnabled = true;
         layoutAlgorithm = mapObject.getLayout();
         setLayoutAlgorithm(layoutAlgorithm, true);
      }

      setObjectDisplayMode(mapObject.getObjectDisplayMode(), false);
      labelProvider.setShowStatusBackground(mapObject.isShowStatusBackground());
      labelProvider.setShowStatusFrame(mapObject.isShowStatusFrame());
      labelProvider.setShowStatusIcons(mapObject.isShowStatusIcon());
      labelProvider.setShowLinkDirection(mapObject.isShowLinkDirection());
      labelProvider.setTranslucentLabelBackground(mapObject.isTranslucentLabelBackground());
      labelProvider.setDefaultLinkStyle(mapObject.getDefaultLinkStyle());
      labelProvider.setDefaultLinkWidth(mapObject.getDefaultLinkWidth());

      actionShowStatusBackground.setChecked(labelProvider.isShowStatusBackground());
      actionShowStatusFrame.setChecked(labelProvider.isShowStatusFrame());
      actionShowStatusIcon.setChecked(labelProvider.isShowStatusIcons());
      actionShowLinkDirection.setChecked(labelProvider.isShowLinkDirection());
      actionTranslucentLabelBkgnd.setChecked(labelProvider.isTranslucentLabelBackground());

      for(int i = 0; i < actionSetAlgorithm.length; i++)
         actionSetAlgorithm[i].setEnabled(automaticLayoutEnabled);
      actionEnableAutomaticLayout.setChecked(automaticLayoutEnabled);
   }

   /**
    * Reconfigure viewer after object change
    */
   private void reconfigureViewer()
   {
      allowManualLayout = !readOnly;
      viewer.setDraggingEnabled(!readOnly && !objectMoveLocked);

      NetworkMap mapObject = getMapObject();
      if (mapObject == null)
         return;

      reconfigureViewer(mapObject);
      labelProvider.updateMapId(getMapObject().getObjectId());
   }

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#setupMapControl()
    */
	@Override
	public void setupMapControl()
	{
      if (mapObjectSyncer == null)
         mapObjectSyncer = MapObjectSyncer.getInstance();

      viewer.addPostSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            actionLinkObjects.setEnabled(!readOnly && (((IStructuredSelection)event.getSelection()).size() == 2));
            actionAutoLinkNodes.setEnabled(!readOnly && (getMapObject().getMapType() == MapType.CUSTOM) && (((IStructuredSelection)event.getSelection()).size() >= 2));
         }
      });

      addDropSupport();
		ImageProvider.getInstance().addUpdateListener(this);
      reconfigureViewer();
	}

	/**
	 * Add drop support
	 */
	private void addDropSupport()
	{
		final Transfer[] transfers = new Transfer[] { LocalSelectionTransfer.getTransfer() };
		viewer.addDropSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new ViewerDropAdapter(viewer) {
			private int x;
			private int y;

			@SuppressWarnings("rawtypes")
			@Override
			public boolean validateDrop(Object target, int operation, TransferData transferType)
			{
            if (readOnly || !LocalSelectionTransfer.getTransfer().isSupportedType(transferType))
					return false;

				IStructuredSelection selection = (IStructuredSelection)LocalSelectionTransfer.getTransfer().getSelection();
				Iterator it = selection.iterator();
				while(it.hasNext())
				{
					Object object = it.next();
					if (!((object instanceof AbstractObject) && ((AbstractObject)object).isAllowedOnMap())) 
						return false;
				}

				return true;
			}

			@SuppressWarnings("unchecked")
			@Override
			public boolean performDrop(Object data)
			{
				IStructuredSelection selection = (IStructuredSelection)LocalSelectionTransfer.getTransfer().getSelection();
            Point p = viewer.getControl().toControl(x + viewer.getHorizontalBarSelection(), y + viewer.getVerticalBarSelection()); 
            p.x = (int) Math.floor(p.x * 1/viewer.getZoom());
            p.y = (int) Math.floor(p.y * 1/viewer.getZoom());
				addObjectsFromList(selection.toList(),  p); 	
				
				return true;
			}

			@Override
			public void dropAccept(DropTargetEvent event)
			{
				x = event.x;
				y = event.y;
				super.dropAccept(event);
			}
		});
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#buildMapPage()
    */
	@Override
	protected void buildMapPage(NetworkMapPage oldMapPage)
	{
	   NetworkMap mapObject = getMapObject();
	   if (mapObject != null)
	   {
	      disableLinkTextAutoUpdate = mapObject.isDontUpdateLinkText();
	      mapPage = mapObject.createMapPage();	      
	   }
	   else
	   {
         mapPage = new NetworkMapPage("EMPTY");   	      
	   }

      refreshDciRequestList(oldMapPage, true);
	}

	/**
	 * Synchronize objects, required when interface objects are placed on the map
	 */
	private void syncObjects()
	{
      NetworkMap mapObject = getMapObject();
      NetworkMapPage page = mapObject.createMapPage(); //mapPage field should be updated together with DCI and object sync
      final Set<Long> mapObjectIds = new HashSet<Long>(page.getObjectIds());
      final Set<Long> mapUtilizationObjets = new HashSet<Long>();
	   page.getAllLinkStatusAndUtilizationObjects(mapObjectIds, mapUtilizationObjets);
	   removeResyncNodes();

      Job job = new Job(String.format(i18n.tr("Synchronize objects for network map %s"), getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.syncMissingObjects(mapObjectIds, mapObject.getObjectId(), NXCSession.OBJECT_SYNC_WAIT | NXCSession.OBJECT_SYNC_ALLOW_PARTIAL);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!viewer.getControl().isDisposed())
                     refresh();
                  mapObjectSyncer.addObjects(mapObject.getObjectId(), mapObjectIds, mapUtilizationObjets);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot synchronize objects");
         }
      };
      job.setUser(false);
      job.start();
	}

	/**
	 * Remove previous map objects
	 */
	private void removeResyncNodes()
	{
	   if (mapPage == null)
	      return;
      final Set<Long> mapObjectIds = new HashSet<Long>(mapPage.getObjectIds());
      mapObjectIds.addAll(mapPage.getAllLinkStatusObjects());
      if (mapObjectSyncer != null)
         mapObjectSyncer.removeObjects(mapPage.getMapObjectId(), mapObjectIds);
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#createActions()
    */
	@Override
	protected void createActions()
	{
		super.createActions();
		
      actionAddObject = new Action(i18n.tr("&Add object..."), SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				addObjectToMap(false);
			}
		};
      addKeyBinding("M1+M3+A", actionAddObject);

      actionAddObjectMenu = new Action(i18n.tr("&Add object..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            addObjectToMap(true);
         }
      };

      actionAddDCIContainer = new Action(i18n.tr("Add DCI &container...")) {
         @Override
         public void run()
         {
            addDCIContainer();
         }
      };
      addKeyBinding("M1+M3+C", actionAddDCIContainer);
      
      actionAddDCIImage = new Action(i18n.tr("Add DCI &image...")) {
         @Override
         public void run()
         {
            addDCIImage();
         }
      };
		
      actionAddGroupBox = new Action(i18n.tr("&Group box...")) {
			@Override
			public void run()
			{
				addGroupBox();
			}
		};

      actionGroupBoxProperties = new Action(i18n.tr("&Properties"), SharedIcons.PROPERTIES) {
         @Override
         public void run()
         {
            editGroupBox();
         }
      };

      actionAddImage = new Action(i18n.tr("&Image...")) {
			@Override
			public void run()
			{
				addImageDecoration();
			}
		};

      actionImageProperties = new Action(i18n.tr("&Properties"), SharedIcons.PROPERTIES) {
         @Override
         public void run()
         {
            editImageDecoration();
         }
      };

      actionLinkObjects = new Action(i18n.tr("&Link selected objects"), ResourceManager.getImageDescriptor("icons/netmap/add_link.png")) {
			@Override
			public void run()
			{
				linkSelectedObjects();
			}
		};
      addKeyBinding("M1+L", actionLinkObjects);

      actionAutoLinkNodes = new Action(i18n.tr("&Auto-link nodes"), ResourceManager.getImageDescriptor("icons/netmap/add_link.png")) {
         @Override
         public void run()
         {
            if (getMapObject().getMapType() == MapType.CUSTOM)
            {
               autoLinkSelectedObjects();
            }
         }
      };

      actionRemove = new Action(i18n.tr("&Remove from map"), SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				removeSelectedElements();
			}
		};
      addKeyBinding("M1+R", actionRemove);

      actionDCIContainerProperties = new Action(i18n.tr("&Properties"), SharedIcons.PROPERTIES) {
         @Override
         public void run()
         {
            editDCIContainer();
         }
      };
      
      actionDCIImageProperties = new Action(i18n.tr("&Properties"), SharedIcons.PROPERTIES) {
         @Override
         public void run()
         {
            editDCIImage();
         }
      };

      actionMapProperties = new Action(i18n.tr("&Properties"), SharedIcons.PROPERTIES) {
			@Override
			public void run()
			{
				showMapProperties();
			}
		};

      actionLinkProperties = new Action(i18n.tr("&Properties"), SharedIcons.PROPERTIES) {
			@Override
			public void run()
			{
				showLinkProperties();
			}
		};
		
      actionAddTextBox = new Action("Text box") {
         @Override
         public void run()
         {
            addTextBox();
         }
      };
      
      actionTextBoxProperties = new Action(i18n.tr("&Properties"), SharedIcons.PROPERTIES) {
         @Override
         public void run()
         {
            editTextBox();
         }
      };
	}

	/**
	 * Create "Add decoration" submenu
	 * 
	 * @return menu manager for decoration submenu
	 */
	private IMenuManager createDecorationAdditionSubmenu()
	{
      MenuManager menu = new MenuManager(i18n.tr("Add &decoration"));
		menu.add(actionAddGroupBox);
		menu.add(actionAddImage);
		menu.add(actionAddTextBox);
		return menu;
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#fillMapContextMenu(org.eclipse.jface.action.IMenuManager)
    */
	@Override
	protected void fillMapContextMenu(IMenuManager manager)
	{
	   if (!readOnly)
	   {
   		manager.add(actionAddObjectMenu);
   		manager.add(actionAddDCIContainer);		
   		manager.add(actionAddDCIImage);  
   		manager.add(createDecorationAdditionSubmenu());
   		manager.add(new Separator());
	   }
		super.fillMapContextMenu(manager);
		manager.add(new Separator());
		manager.add(actionMapProperties);
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#fillObjectContextMenu(org.eclipse.jface.action.IMenuManager)
    */
	@Override
	protected void fillObjectContextMenu(IMenuManager manager, boolean isPartial)
	{
	   if (!readOnly)
	   {
         int size = viewer.getStructuredSelection().size();
   		if (size >= 2)
   			manager.add(actionLinkObjects);
   		if (size >= 2 && getMapObject().getMapType() == MapType.CUSTOM)
   			manager.add(actionAutoLinkNodes);
   		manager.add(actionRemove);
   		manager.add(new Separator());
	   }
		super.fillObjectContextMenu(manager, isPartial);
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#fillLinkContextMenu(org.eclipse.jface.action.IMenuManager)
    */
	@Override
	protected void fillLinkContextMenu(IMenuManager manager)
	{
	   if (readOnly)
	   {
	      super.fillLinkContextMenu(manager);
	      return;
	   }

      int size = viewer.getStructuredSelection().size();
		super.fillLinkContextMenu(manager);
      manager.add(new Separator());
      manager.add(actionRemove);
		if (size == 1)
		{
			manager.add(new Separator());
			manager.add(actionLinkProperties);
		}
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#fillElementContextMenu(org.eclipse.jface.action.IMenuManager)
    */
	protected void fillElementContextMenu(IMenuManager manager)
	{
	   if (!readOnly)
	   {
   		manager.add(actionRemove);
         Object o = viewer.getStructuredSelection().getFirstElement();
   		if (o instanceof NetworkMapDCIContainer)
   		{
   		   manager.add(actionDCIContainerProperties);
   		}
   		else if (o instanceof NetworkMapDCIImage)
   		{
            manager.add(actionDCIImageProperties);
   		}
   		else if (o instanceof NetworkMapTextBox)
   		{
            manager.add(actionTextBoxProperties);
   		}
   		else if (o instanceof NetworkMapDecoration)
   		{
            manager.add((((NetworkMapDecoration)o).getDecorationType() == NetworkMapDecoration.IMAGE) ? actionImageProperties : actionGroupBoxProperties);
   		}
   		manager.add(new Separator());
	   }
		super.fillElementContextMenu(manager);
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#fillLocalMenu(IMenuManager)
    */
	@Override
   protected void fillLocalMenu(IMenuManager manager)
	{
	   if (!readOnly)
	   {
   		manager.add(actionAddObject);
   		manager.add(actionLinkObjects);
   		manager.add(createDecorationAdditionSubmenu());
   		manager.add(new Separator());
	   }
      super.fillLocalMenu(manager);
		manager.add(new Separator());
		manager.add(actionMapProperties);
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#fillLocalToolBar(IToolBarManager)
    */
	@Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
	   if (!readOnly)
	   {
         manager.add(actionAddObject);
   		manager.add(actionLinkObjects);
   		manager.add(new Separator());
	   }
      super.fillLocalToolBar(manager);
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#saveLayout()
    */
	@Override
	protected void saveLayout()
	{
		saveMap();
	}

	/**
	 * Add object to map
	 * @param useRightClickLocation 
	 */
	private void addObjectToMap(boolean useRightClickLocation)
	{      
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell());
		if (dlg.open() != Window.OK)
			return;
      
		addObjectsFromList(dlg.getSelectedObjects(), useRightClickLocation ? viewer.getRightClickLocation() : null);
	}

	/**
	 * Add objects from list to map
	 * 
	 * @param list
	 *           object list
	 */
	private void addObjectsFromList(List<AbstractObject> list, Point location)
	{
		int added = 0;
		for(AbstractObject object : list)
		{
			if (mapPage.findObjectElement(object.getObjectId()) == null)
			{
				final NetworkMapObject mapObject = new NetworkMapObject(mapPage.createElementId(), object.getObjectId());
				if (location != null)
				{
					mapObject.setLocation(location.x, location.y);
				}
				else
				   mapObject.setLocation(40, 40);
				mapPage.addElement(mapObject);
				added++;
			}
		}

		if (added > 0)
		{
			saveMap();
		}
	}

	/**
	 * Remove currently selected map elements
	 */
	private void removeSelectedElements()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Confirm Removal"),
            (selection.size() == 1) ? i18n.tr("Are you sure to remove selected element from map?") : i18n.tr("Are you sure to remove selected elements from map?")))
			return;

		Object[] objects = selection.toArray();
		for(Object element : objects)
		{
			if (element instanceof AbstractObject)
			{
				mapPage.removeObjectElement(((AbstractObject)element).getObjectId());
			}
			else if (element instanceof NetworkMapElement)
			{
				mapPage.removeElement(((NetworkMapElement)element).getId());
			}
			else if (element instanceof NetworkMapLink)
			{
				mapPage.removeLink((NetworkMapLink)element);
			}
		}
		saveMap();
		
		// for some reason graph viewer does not clear selection 
		// after all selected elements was removed, so we have to do it manually
		viewer.setSelection(StructuredSelection.EMPTY);
	}

   /**
    * Update map size
    * 
    * @param width
    * @param height
    */
   protected void updateMapSize(int width, int height)
   {
      mapWidth = width;
      mapHeight = height;
      super.updateMapSize(width, height);
   }

	/**
	 * Save map on server
	 */
	private void saveMap()
	{
		updateObjectPositions();
		
      final NXCObjectModificationData md = new NXCObjectModificationData(getMapObject().getObjectId());
		md.setMapContent(mapPage.getElements(), mapPage.getLinks());
		md.setMapLayout(automaticLayoutEnabled ? layoutAlgorithm : MapLayoutAlgorithm.MANUAL);
		md.setConnectionRouting(routingAlgorithm);
		md.setMapObjectDisplayMode(labelProvider.getObjectFigureType());
      md.setMapSize(mapWidth, mapHeight);
		
      int flags = getMapObject().getFlags();
		if (labelProvider.isShowStatusIcons())
			flags |= NetworkMap.MF_SHOW_STATUS_ICON;
		else
			flags &= ~NetworkMap.MF_SHOW_STATUS_ICON;
		if (labelProvider.isShowStatusFrame())
			flags |= NetworkMap.MF_SHOW_STATUS_FRAME;
		else
			flags &= ~NetworkMap.MF_SHOW_STATUS_FRAME;
		if (labelProvider.isShowStatusBackground())
			flags |= NetworkMap.MF_SHOW_STATUS_BKGND;
		else
			flags &= ~NetworkMap.MF_SHOW_STATUS_BKGND;
      if (labelProvider.isShowLinkDirection())
         flags |= NetworkMap.MF_SHOW_LINK_DIRECTION;
      else
         flags &= ~NetworkMap.MF_SHOW_LINK_DIRECTION;
      if (labelProvider.isTranslucentLabelBackground())
         flags |= NetworkMap.MF_TRANSLUCENT_LABEL_BKGND;
      else
         flags &= ~NetworkMap.MF_TRANSLUCENT_LABEL_BKGND;
		md.setObjectFlags(flags);

      new Job(String.format(i18n.tr("Save network map %s"), getObjectName()), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
            runInUIThread(() -> viewer.setInput(mapPage));
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot save map");
			}
		}.start();
	}

	/**
	 * Show properties dialog for map object
	 */
	private void showMapProperties()
	{
		updateObjectPositions();
      ObjectPropertiesManager.openObjectPropertiesDialog(getMapObject(), getWindow().getShell(), this);
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#isSelectableElement(java.lang.Object)
    */
	@Override
	protected boolean isSelectableElement(Object element)
	{
		return (element instanceof NetworkMapDecoration) || (element instanceof NetworkMapLink) ||
		       (element instanceof NetworkMapDCIContainer) || (element instanceof NetworkMapDCIImage) ||
		       (element instanceof NetworkMapTextBox);
	}

   /**
    * @see org.netxms.nxmc.modules.imagelibrary.ImageUpdateListener#imageUpdated(java.util.UUID)
    */
	@Override
	public void imageUpdated(final UUID guid)
	{
      if (guid.equals(getMapObject().getBackground()))
         viewer.setBackgroundImage(ImageProvider.getInstance().getImage(guid), getMapObject().isCenterBackgroundImage(), getMapObject().isFitBackgroundImage());

      final String guidText = guid.toString();
      for(NetworkMapElement e : mapPage.getElements())
      {
         if ((e instanceof NetworkMapDecoration) && (((NetworkMapDecoration)e).getDecorationType() == NetworkMapDecoration.IMAGE) && ((NetworkMapDecoration)e).getTitle().equals(guidText))
			{
            viewer.updateDecorationFigure((NetworkMapDecoration)e);
            break;
			}
      }
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#dispose()
    */
	@Override
	public void dispose()
	{
      ImageProvider imageProvider = ImageProvider.getInstance();
      if (imageProvider != null)
         imageProvider.removeUpdateListener(this);
		if (defaultLinkColor != null)
			defaultLinkColor.dispose();
		removeResyncNodes();
      mapObjectSyncer = null;
		super.dispose();
	}

	/**
	 * Update existing element or show message if failed
	 * 
	 * @param element element to update
	 * @return true if was successfully updated
	 */
	private boolean updateElement(NetworkMapElement element)
	{
      if (mapPage.updateElement(element))
      {
         saveMap(); 
         return true;
      }

      addMessage(MessageArea.ERROR, i18n.tr("Failed to update map element: the object no longer exists"));
      return false;
	}

   /**
    * Add DCI container to map
    */
   private void addDCIContainer()
   {
      NetworkMapDCIContainer dciContainer = new NetworkMapDCIContainer(mapPage.createElementId());
      if (showDCIContainerProperties(dciContainer))
      {
         mapPage.addElement(dciContainer);
         fullDciCacheRefresh();
         saveMap();
      }
   }

	/**
	 * Show DCI Container properties
	 */
   private void editDCIContainer()
   {
      updateObjectPositions();

      IStructuredSelection selection = viewer.getStructuredSelection();
      if ((selection.size() != 1) || !(selection.getFirstElement() instanceof NetworkMapDCIContainer))
         return;

      NetworkMapDCIContainer dciContainer = (NetworkMapDCIContainer)selection.getFirstElement();
      if (showDCIContainerProperties(dciContainer) && updateElement(dciContainer))
      {
         fullDciCacheRefresh();
         saveMap();
      }
   }

   /**
    * Show DCI container properties.
    *
    * @param container DCI container to edit
    * @return true if changes were made
    */
   private boolean showDCIContainerProperties(NetworkMapDCIContainer container)
   {
      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("dciContainer.general", new DCIContainerGeneral(container)));
      pm.addToRoot(new PreferenceNode("dciContainer.dataSources", new DCIContainerDataSources(container)));
      PropertyDialog dlg = new PropertyDialog(getWindow().getShell(), pm, i18n.tr("DCI Container Properties"));
      return dlg.open() == Window.OK;
   }

   /**
    * Add DCI image to map
    */
   private void addDCIImage()
   {
      NetworkMapDCIImage dciImage = new NetworkMapDCIImage(mapPage.createElementId());
      if (showDCIImageProperties(dciImage))
      {
         fullDciCacheRefresh();
         mapPage.addElement(dciImage);
         saveMap();
      }
   }

   /**
    * Show DCI Image properties
    */
   private void editDCIImage()
   {
      updateObjectPositions();

      IStructuredSelection selection = viewer.getStructuredSelection();
      if ((selection.size() != 1) || !(selection.getFirstElement() instanceof NetworkMapDCIImage))
         return;

      NetworkMapDCIImage dciImage = (NetworkMapDCIImage)selection.getFirstElement();
      if (showDCIImageProperties(dciImage) && updateElement(dciImage))
      {
         fullDciCacheRefresh();
         saveMap();
      }
   }

   /**
    * Show DCI image properties.
    *
    * @param dciImage DCI image to edit
    * @return true if changes were made
    */
   private boolean showDCIImageProperties(NetworkMapDCIImage dciImage)
   {
      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("dciImage.general", new DCIImageGeneral(dciImage)));
      pm.addToRoot(new PreferenceNode("dciImage.rules", new DCIImageRules(dciImage)));
      PropertyDialog dlg = new PropertyDialog(getWindow().getShell(), pm, i18n.tr("DCI Image Properties"));
      return dlg.open() == Window.OK;
   }

   /**
    * Link currently selected objects
    */
   private void linkSelectedObjects()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 2)
         return;

      Object[] objects = selection.toArray();
      long id1 = ((NetworkMapObject)objects[0]).getId();
      long id2 = ((NetworkMapObject)objects[1]).getId();
      mapPage.addLink(new NetworkMapLink(mapPage.createLinkId(), NetworkMapLink.NORMAL, id1, id2));
      saveMap();
   }
   
   /**
    * Auto link nodes based on L2 connectivity
    */
   private void autoLinkSelectedObjects()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() < 2)
         return;
      
      List<Long> nodes = new ArrayList<Long>(selection.size());
      for (Object object : selection.toList())
      {
         if (object instanceof NetworkMapObject)
         {
            nodes.add(((NetworkMapObject)object).getObjectId());
         }
      }     

      new Job("Auto link network map nodes", this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.autoLinkNetworkMapNodes(getObjectId(), nodes);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot auto link network map nodes";
         }
      }.start();
   }

	/**
	 * Show properties for currently selected link
	 */
	private void showLinkProperties()
	{
		updateObjectPositions();

      IStructuredSelection selection = viewer.getStructuredSelection();
		if ((selection.size() != 1) || !(selection.getFirstElement() instanceof NetworkMapLink))
			return;

		LinkEditor link = new LinkEditor((NetworkMapLink)selection.getFirstElement(), mapPage, disableLinkTextAutoUpdate);

      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("link.general", new LinkGeneral(link)));
      pm.addToRoot(new PreferenceNode("link.color", new LinkColor(link)));
      pm.addToRoot(new PreferenceNode("link.dataSources", new LinkDataSources(link)));

      PreferenceDialog dlg = new PreferenceDialog(getWindow().getShell(), pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText(i18n.tr("Link Properties"));
         }
      };
      dlg.setBlockOnOpen(true);
      dlg.open();
      if (link.isModified())
      {
         if (link.getRoutingAlgorithm() != NetworkMapLink.ROUTING_BENDPOINTS && bendpointEditor != null)
         {
            bendpointEditor.stop();
            bendpointEditor = null;            
         }
         if (link.update(mapPage))
         {
            fullDciCacheRefresh();
            saveMap();
         }
         else
         {
            addMessage(MessageArea.ERROR, i18n.tr("Failed to update link: the object no longer exists"));
         }
      }
	}

   /**
    * Add text box element
    */
   private void addTextBox()
   {
      NetworkMapTextBox textBox = new NetworkMapTextBox(mapPage.createElementId());
      if (showTextBoxProperties(textBox))
      {
         mapPage.addElement(textBox);
         saveMap();
      }
   }

   /**
    * Edit selected text box
    */
   private void editTextBox()
   {
      updateObjectPositions();

      IStructuredSelection selection = viewer.getStructuredSelection();
      if ((selection.size() != 1) || !(selection.getFirstElement() instanceof NetworkMapTextBox))
         return;

      NetworkMapTextBox textBox = (NetworkMapTextBox)selection.getFirstElement();
      if (showTextBoxProperties(textBox) && updateElement(textBox))
      {
         saveMap();
      }
   }

	/**
    * Show text box properties
    * 
    * @return true if there were modifications
    */
   private boolean showTextBoxProperties(NetworkMapTextBox textBox)
	{
      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("textbox.general", new TextBoxGeneral(textBox)));
      PropertyDialog dlg = new PropertyDialog(getWindow().getShell(), pm, i18n.tr("Text Box Properties"));
      return dlg.open() == Window.OK;
	}

   /**
    * Add image decoration
    */
   private void addImageDecoration()
   {
      ImageSelectionDialog dlg = new ImageSelectionDialog(getWindow().getShell());
      if (dlg.open() != Window.OK)
         return;

      UUID imageGuid = dlg.getImageGuid();
      Rectangle imageBounds = ImageProvider.getInstance().getImage(imageGuid).getBounds();

      NetworkMapDecoration element = new NetworkMapDecoration(mapPage.createElementId(), NetworkMapDecoration.IMAGE);
      element.setSize(imageBounds.width, imageBounds.height);
      element.setTitle(imageGuid.toString());
      mapPage.addElement(element);

      saveMap();
   }

	/**
	 * Edit image decoration
	 */
	private void editImageDecoration()
	{
      updateObjectPositions();
      
      IStructuredSelection selection = viewer.getStructuredSelection();
      if ((selection.size() != 1) || !(selection.getFirstElement() instanceof NetworkMapDecoration))
         return;
      
      ImageSelectionDialog dlg = new ImageSelectionDialog(getWindow().getShell());
      if (dlg.open() != Window.OK)
         return;
      
      UUID imageGuid = dlg.getImageGuid();
      Rectangle imageBounds = ImageProvider.getInstance().getImage(imageGuid).getBounds();

      NetworkMapDecoration element = (NetworkMapDecoration)selection.getFirstElement();
      element.setSize(imageBounds.width, imageBounds.height);
      element.setTitle(imageGuid.toString());
      mapPage.addElement(element);

      saveMap();
	}
	
   /**
    * Add group box decoration
    */
   private void addGroupBox()
   {
      NetworkMapDecoration element = new NetworkMapDecoration(mapPage.createElementId(), NetworkMapDecoration.GROUP_BOX);
      EditGroupBoxDialog dlg = new EditGroupBoxDialog(getWindow().getShell(), element);
      if (dlg.open() != Window.OK)
         return;

      mapPage.addElement(element);

      saveMap();
   }

	/**
	 * Edit group box
	 */
	private void editGroupBox()
	{
      updateObjectPositions();

      IStructuredSelection selection = viewer.getStructuredSelection();
      if ((selection.size() != 1) || !(selection.getFirstElement() instanceof NetworkMapDecoration))
         return;

      NetworkMapDecoration groupBox = (NetworkMapDecoration)selection.getFirstElement();
      EditGroupBoxDialog dlg = new EditGroupBoxDialog(getWindow().getShell(), groupBox);
      if (dlg.open() == Window.OK)
      {
         mapPage.addElement(groupBox);
         saveMap();
      }
	}

   /**
    * Get current object as NetworkMap object.
    *
    * @return current object as NetworkMap object
    */
   protected NetworkMap getMapObject()
   {
      return (NetworkMap)getObject();
   }

   /**
    * Schedule element save on move
    * 
    * @param element
    */
   public void onElementMove(NetworkMapElement element)
   {
      if (!allowManualLayout)
         return;
      
      synchronized(movedElementList)
      {
         movedElementList.add(element);
         if (!saveSchedulted)
         {
            saveSchedulted = true;
            getDisplay().timerExec(1000, new Runnable() {
               @Override
               public void run()
               {
                  saveObjectPositions();
               }
            });
         }
      }
   }
   
   /**
    * Schedule link save on change
    * 
    * @param link
    */
   public void onLinkChange(NetworkMapLink link)
   {       
      synchronized(movedElementList)
      {
         changedLinkList.add(link);
         if (!saveSchedulted)
         {
            saveSchedulted = true;
            getDisplay().timerExec(1000, new Runnable() {
               @Override
               public void run()
               {
                  saveObjectPositions();
               }
            });
         }
      } 
   }
   
   

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#refresh()
    */
   @Override
   public void refresh()
   {
      if (viewer.isRefreshBlocked())
         return;

      saveObjectPositions();
      super.refresh();
   }

   /**
    * Save change object positions
    * Force execute on switch to other node or on update from server
    */
   protected void saveObjectPositions()
   {
      if (!saveSchedulted)
         return;
      
      final Set<NetworkMapElement> elementListLocalCopy;
      final Set<NetworkMapLink> linkListLocalCopy;
      synchronized(movedElementList)
      {
         saveSchedulted = false;
         elementListLocalCopy = movedElementList;
         movedElementList = new HashSet<NetworkMapElement>();
         linkListLocalCopy = changedLinkList;
         changedLinkList = new HashSet<NetworkMapLink>();
      }
      
      if (elementListLocalCopy.size() > 0)
         updateObjectPositions();

      new Job("Save object's new positions", this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateNetworkMapElementPosition(getMapObject().getObjectId(), elementListLocalCopy, linkListLocalCopy);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot save network map object and link new positions";
         }
      }.start();
   }

   /**
    * Save network map zoom
    * 
    * @return map id for saved settrings names
    */
   protected void saveZoom(AbstractObject object)
   {
      final PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(getBaseId() + "@" + object.getObjectId() + ".zoom", viewer.getZoom());
   }

   /**
    * Update zoom from storage
    * 
    * @return map id for saved settrings names
    */
   protected void loadZoom(AbstractObject object)
   {
      if (object == null)
         return;
      final PreferenceStore settings = PreferenceStore.getInstance();
      viewer.zoomTo(settings.getAsDouble(getBaseId() + "@" + object.getObjectId() + ".zoom", 1.0));
   }
}
