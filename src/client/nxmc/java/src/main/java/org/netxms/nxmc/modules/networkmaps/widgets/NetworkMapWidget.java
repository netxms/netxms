/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.networkmaps.widgets;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.Stack;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.draw2d.ManhattanConnectionRouter;
import org.eclipse.gef4.zest.layouts.LayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.CompositeLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.GridLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.RadialLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.SpringLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.TreeLayoutAlgorithm;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.maps.MapLayoutAlgorithm;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.configs.DCIImageConfiguration;
import org.netxms.client.maps.configs.MapDataSource;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.modules.networkmaps.ObjectDoubleClickHandlerRegistry;
import org.netxms.nxmc.modules.networkmaps.algorithms.ManualLayout;
import org.netxms.nxmc.modules.networkmaps.algorithms.SparseTree;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.ExtendedGraphViewer;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.LinkDciValueProvider;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.MapContentProvider;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.MapLabelProvider;
import org.netxms.nxmc.tools.ColorConverter;
import org.xnap.commons.i18n.I18n;

/**
 * Widget for embedding network map into dashboards
 */
public class NetworkMapWidget extends Composite
{
	protected ExtendedGraphViewer viewer;
	protected MapLabelProvider labelProvider;

   private I18n i18n = LocalizationHelper.getI18n(NetworkMapWidget.class);
	private Color defaultLinkColor = null;
	private boolean disableGeolocationBackground = false;
   private NXCSession session = Registry.getSession();
   private SessionListener sessionListener;
   private NetworkMapPage mapPage = null;
   private ObjectDoubleClickHandlerRegistry doubleClickHandlers;
   private Stack<Long> history = new Stack<Long>();
   private long currentMapId = 0;
   private LinkDciValueProvider dciValueProvider;
   private View view;

	/**
	 * @param parent
	 * @param style
	 */
   public NetworkMapWidget(Composite parent, int style, View view)
	{
		super(parent, style);
      dciValueProvider = LinkDciValueProvider.getInstance();
      this.view = view;
      disableGeolocationBackground = PreferenceStore.getInstance().getAsBoolean("NetMap.DisableGeolocationBackground", false);

		setLayout(new FillLayout());

      viewer = new ExtendedGraphViewer(this, SWT.NONE, view, null); 
      labelProvider = new MapLabelProvider(viewer);
		viewer.setContentProvider(new MapContentProvider(viewer, labelProvider));
		viewer.setLabelProvider(labelProvider);

		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				if (defaultLinkColor != null)
					defaultLinkColor.dispose();
		      if (sessionListener != null)
		         session.removeListener(sessionListener);
			}
		});

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(final SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_CHANGED)
            {
               getDisplay().asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     onObjectChange((AbstractObject)n.getObject());
                  }
               });
            }
         }
      };
      session.addListener(sessionListener);
	}

	/**
	 * Enable double click on objects 
	 */
	public void enableObjectDoubleClick()
	{
      doubleClickHandlers = new ObjectDoubleClickHandlerRegistry(view);
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            if (selection.isEmpty() || !(selection.getFirstElement() instanceof NetworkMapObject))
               return;

            AbstractObject object = session.findObjectById(((NetworkMapObject)selection.getFirstElement()).getObjectId());
            if (object == null)
               return;

            if (object instanceof NetworkMap)
            {
               history.push(currentMapId);
               setContent((NetworkMap)object, false);
               viewer.showBackButton(new Runnable() {
                  @Override
                  public void run()
                  {
                     goBack();
                  }
               });
            }
            else
            {
               doubleClickHandlers.handleDoubleClick(object);
            }
         }
      });
	}

	/**
	 * Go back map history
	 */
	private void goBack()
	{
	   if (history.isEmpty())
	      return;

	   long submapId = history.pop();
      NetworkMap map = session.findObjectById(submapId, NetworkMap.class);
      if (map != null)
      {
         setContent(map, false);
      }

	   if (history.isEmpty())
	      viewer.hideBackButton();
	}

   /**
    * Called by session listener when NetXMS object was changed.
    * 
    * @param object changed NetXMS object
    */
   private void onObjectChange(final AbstractObject object)
   {
      if (mapPage == null)
         return;

      NetworkMapObject element = mapPage.findObjectElement(object.getObjectId());
      if (element != null)
         viewer.refresh(element, true);

      List<NetworkMapLink> links = mapPage.findLinksWithStatusObject(object.getObjectId());
      if (links != null)
         for(NetworkMapLink l : links)
            viewer.refresh(l);
   }

	/**
    * Set content from map page
    * 
    * @param page map page
    */
	public void setContent(NetworkMapPage page)
	{
	   mapPage = page;
      addDciToRequestList();
      viewer.setInput(page);
	}

	/**
	 * Set content from preconfigured map object
	 * 
	 * @param mapObject
	 */
	public void setContent(NetworkMap mapObject)
	{
	   setContent(mapObject, true);
	}

   /**
    * Set content from preconfigured map object
    * 
    * @param mapObject
    * @param resetHistory true to reset drill-down history
    */
   private void setContent(NetworkMap mapObject, boolean resetHistory)
   {
      syncObjects(mapObject);
      
      if(!mapObject.isFitToScreen())
      {
         int width = mapObject.getWidth() == 0 ? session.getNetworkMapDefaultWidth() : mapObject.getWidth();
         int height = mapObject.getHeight() == 0 ? session.getNetworkMapDefaultHeight() : mapObject.getHeight();
         viewer.setMapSize(width, height);
      }
      else
      {
         viewer.setMapSize(-1, -1);         
      }
      
      currentMapId = mapObject.getObjectId();
		setMapLayout(mapObject.getLayout());

		if ((mapObject.getBackground() != null) && (mapObject.getBackground().compareTo(NXCommon.EMPTY_GUID) != 0))
		{
			if (mapObject.getBackground().equals(org.netxms.client.objects.NetworkMap.GEOMAP_BACKGROUND))
			{
			   if (!disableGeolocationBackground)
			      viewer.setBackgroundImage(mapObject.getBackgroundLocation(), mapObject.getBackgroundZoom());
			}
			else
			{
            viewer.setBackgroundImage(ImageProvider.getInstance().getImage(mapObject.getBackground()), mapObject.isCenterBackgroundImage(), mapObject.isFitBackgroundImage());
			}
		}

		setConnectionRouter(mapObject.getDefaultLinkRouting());
		viewer.setBackgroundColor(ColorConverter.rgbFromInt(mapObject.getBackgroundColor()));

		if (mapObject.getDefaultLinkColor() >= 0)
		{
			if (defaultLinkColor != null)
				defaultLinkColor.dispose();
			defaultLinkColor = new Color(viewer.getControl().getDisplay(), ColorConverter.rgbFromInt(mapObject.getDefaultLinkColor()));
			labelProvider.setDefaultLinkColor(defaultLinkColor);
		}

      labelProvider.setObjectFigureType(mapObject.getObjectDisplayMode());
      labelProvider.setShowStatusBackground(mapObject.isShowStatusBackground());
      labelProvider.setShowStatusFrame(mapObject.isShowStatusFrame());
      labelProvider.setShowStatusIcons(mapObject.isShowStatusIcon());
      labelProvider.setTranslucentLabelBackground(mapObject.isTranslucentLabelBackground());
      labelProvider.setDefaultLinkStyle(mapObject.getDefaultLinkStyle());
      labelProvider.setDefaultLinkWidth(mapObject.getDefaultLinkWidth());

		mapPage = mapObject.createMapPage();
      addDciToRequestList();	
		viewer.setInput(mapPage);

		if (resetHistory)
		{
		   history.clear();
		}
	}

   /**
    * Synchronize objects, required when interface objects are placed on the map
    */
   private void syncObjects(final NetworkMap mapObject)
   {
      NetworkMapPage mapPage = mapObject.createMapPage();
      final Set<Long> mapObjectIds = new HashSet<Long>(mapPage.getObjectIds());
      mapObjectIds.addAll(mapPage.getAllLinkStatusObjects());

      Job job = new Job(String.format(i18n.tr("Synchronizing missing objects for network map %s"), mapObject.getObjectName()), view) {
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
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot synchronize missing objects for network map %s"), mapObject.getObjectName());
         }
      };
      job.setUser(false);
      job.start();
   }

	/**
	 * @param layout
	 */
	public void setMapLayout(MapLayoutAlgorithm layout)
	{
		if (layout == MapLayoutAlgorithm.MANUAL)
		{
			viewer.setLayoutAlgorithm(new ManualLayout());
		}
		else
		{
			setLayoutAlgorithm(layout);
		}
	}
	
	/**
	 * Set layout algorithm for map
	 * @param alg
	 */
	public void setLayoutAlgorithm(MapLayoutAlgorithm alg)
	{
		LayoutAlgorithm algorithm;
		
		switch(alg)
		{
			case SPRING:
				algorithm = new SpringLayoutAlgorithm();
				break;
			case RADIAL:
				algorithm = new RadialLayoutAlgorithm();
				break;
			case HTREE:
				algorithm = new TreeLayoutAlgorithm(TreeLayoutAlgorithm.LEFT_RIGHT);
				break;
			case VTREE:
				algorithm = new TreeLayoutAlgorithm(TreeLayoutAlgorithm.TOP_DOWN);
				break;
			case SPARSE_VTREE:
				TreeLayoutAlgorithm mainLayoutAlgorithm = new TreeLayoutAlgorithm(TreeLayoutAlgorithm.TOP_DOWN);
				viewer.setComparator(new ViewerComparator() {
					@Override
					public int compare(Viewer viewer, Object e1, Object e2)
					{
						return e1.toString().compareToIgnoreCase(e2.toString());
					}
				});
				algorithm = new CompositeLayoutAlgorithm( 
						new LayoutAlgorithm[] { mainLayoutAlgorithm,
						                        new SparseTree() });
				break;
			default:
				algorithm = new GridLayoutAlgorithm();
				break;
		}
		viewer.setLayoutAlgorithm(algorithm);
	}

	/**
	 * Set map default connection routing algorithm
	 * 
	 * @param routingAlgorithm
	 */
	public void setConnectionRouter(int routingAlgorithm)
	{
		switch(routingAlgorithm)
		{
			case NetworkMapLink.ROUTING_MANHATTAN:
				viewer.getGraphControl().setRouter(new ManhattanConnectionRouter());
				break;
			default:
				viewer.getGraphControl().setRouter(null);
				break;
		}
		viewer.refresh();
	}
	
	/**
	 * @param zoomLevel
	 */
	public void zoomTo(double zoomLevel)
	{
		viewer.zoomTo(zoomLevel);
	}
	
	/**
	 * Refresh control
	 */
	public void refresh()
	{
	   viewer.refresh();
	}

   /**
    * Goes thought all links and trys to add to request list required DCIs.
    */
   protected void addDciToRequestList()
   {
      Collection<NetworkMapLink> linkList = mapPage.getLinks();
      for (NetworkMapLink item : linkList)
      {
         if(item.hasDciData())
         {
            for (MapDataSource value :item.getDciAsList())
            {
               if(value.getType() == MapDataSource.ITEM)
               {
                  dciValueProvider.addDci(value.getNodeId(), value.getDciId(), mapPage, 1);
               }
               else
               {
                  dciValueProvider.addDci(value.getNodeId(), value.getDciId(), value.getColumn(), value.getInstance(), mapPage, 1);
               }
            }
         }
      }
      Collection<NetworkMapElement> mapElements = mapPage.getElements();
      for (NetworkMapElement element : mapElements)
      {
         if(element instanceof NetworkMapDCIContainer)
         {
            NetworkMapDCIContainer item = (NetworkMapDCIContainer)element;
            if(item.hasDciData())
            {
               for (MapDataSource value : item.getObjectDCIArray())
               {
                  if(value.getType() == MapDataSource.ITEM)
                  {
                     dciValueProvider.addDci(value.getNodeId(), value.getDciId(), mapPage, 1);
                  }
                  else
                  {
                     dciValueProvider.addDci(value.getNodeId(), value.getDciId(), value.getColumn(), value.getInstance(), mapPage, 1);
                  }
               }
            }
         }
         
         if(element instanceof NetworkMapDCIImage)
         {
            NetworkMapDCIImage item = (NetworkMapDCIImage)element;
            DCIImageConfiguration config = item.getImageOptions();
            MapDataSource value = config.getDci();
            if(value.getType() == MapDataSource.ITEM)
            {
               dciValueProvider.addDci(value.getNodeId(), value.getDciId(), mapPage, 1);
            }
            else
            {
               dciValueProvider.addDci(value.getNodeId(), value.getDciId(), value.getColumn(), value.getInstance(), mapPage, 1);
            }
         }
      }
   }
   
   /**
    * Get map label provider
    * @return label provider
    */ 
   public MapLabelProvider getLabelProvider()
   {
      return labelProvider;
   }
   
   /**
    * Get control
    * @return control
    */
   public Control getControl()
   {
      return viewer.getControl();
   }

   /**
    * Hide link labels 
    * 
    * @param hide true if labels should not be displayed
    */
   public void hideLinkLabels(boolean hide)
   {
      labelProvider.setConnectionLabelsVisible(false);
   }
}
