/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.widgets;

import java.util.List;
import org.eclipse.draw2d.ManhattanConnectionRouter;
import org.eclipse.gef4.zest.layouts.LayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.CompositeLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.GridLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.RadialLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.SpringLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.TreeLayoutAlgorithm;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.maps.MapLayoutAlgorithm;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.networkmaps.algorithms.ManualLayout;
import org.netxms.ui.eclipse.networkmaps.algorithms.SparseTree;
import org.netxms.ui.eclipse.networkmaps.views.helpers.ExtendedGraphViewer;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapContentProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Widget for embedding network map into dashboards
 */
public class NetworkMapWidget extends Composite
{
	protected ExtendedGraphViewer viewer;
	protected MapLabelProvider labelProvider;
	
	private Color defaultLinkColor = null;
	private boolean disableGeolocationBackground = false;
   private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   private SessionListener sessionListener;
   private NetworkMapPage mapPage = null;

	/**
	 * @param parent
	 * @param style
	 */
	public NetworkMapWidget(Composite parent, int style)
	{
		super(parent, style);
	
      final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
      disableGeolocationBackground = ps.getBoolean("DISABLE_GEOLOCATION_BACKGROUND"); //$NON-NLS-1$
		
		setLayout(new FillLayout());

		viewer = new ExtendedGraphViewer(this, SWT.NONE);
		viewer.setContentProvider(new MapContentProvider(viewer));
		labelProvider = new MapLabelProvider(viewer);
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
    * Called by session listener when NetXMS object was changed.
    * 
    * @param object changed NetXMS object
    */
   private void onObjectChange(final AbstractObject object)
   {
      if (mapPage == null)
         return ;
      
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
	 * @param page
	 */
	public void setContent(NetworkMapPage page)
	{
	   mapPage = page;
		viewer.setInput(page);
	}
	
	/**
	 * Set content from preconfigured map object
	 * 
	 * @param mapObject
	 */
	public void setContent(NetworkMap mapObject)
	{
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
				viewer.setBackgroundImage(ImageProvider.getInstance().getImage(mapObject.getBackground()));
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
		
		labelProvider.setShowStatusBackground((mapObject.getFlags() & org.netxms.client.objects.NetworkMap.MF_SHOW_STATUS_BKGND) > 0);
		labelProvider.setShowStatusFrame((mapObject.getFlags() & org.netxms.client.objects.NetworkMap.MF_SHOW_STATUS_FRAME) > 0);
		labelProvider.setShowStatusIcons((mapObject.getFlags() & org.netxms.client.objects.NetworkMap.MF_SHOW_STATUS_ICON) > 0);
		
		mapPage = mapObject.createMapPage();
		viewer.setInput(mapPage);
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
	private void setLayoutAlgorithm(MapLayoutAlgorithm alg)
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
}
