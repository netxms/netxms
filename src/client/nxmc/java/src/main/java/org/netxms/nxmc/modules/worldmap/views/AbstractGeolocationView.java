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
package org.netxms.nxmc.modules.worldmap.views;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.base.GeoLocation;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.worldmap.GeoMapListener;
import org.netxms.nxmc.modules.worldmap.tools.MapAccessor;
import org.netxms.nxmc.modules.worldmap.widgets.AbstractGeoMapViewer;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Base class for all geographical views
 */
public abstract class AbstractGeolocationView extends View implements ISelectionProvider
{
   private static final I18n i18n = LocalizationHelper.getI18n(AbstractGeolocationView.class);

	protected AbstractGeoMapViewer map;

	private MapAccessor mapAccessor;
	private int zoomLevel = 15;
	private Action actionZoomIn;
	private Action actionZoomOut;
	private ISelection selection;
	private Set<ISelectionChangedListener> selectionChangeListeners = new HashSet<ISelectionChangedListener>();
	
	/**
    * Default constructor
    */
   protected AbstractGeolocationView()
   {
      super();
   }

   /**
    * Create new view with given name and ID.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    */
   public AbstractGeolocationView(String name, ImageDescriptor image, String id)
   {
      super(name, image, id);
   }

   /**
    * Get initial center point for displayed map
    * 
    * @return
    */
	protected abstract GeoLocation getInitialCenterPoint();
	
	/**
	 * Get initial zoom level
	 * 
	 * @return
	 */
	protected abstract int getInitialZoomLevel();

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   public void createContent(Composite parent)
	{      
		map = createMapViewer(parent, SWT.BORDER);

		createActions();
		createPopupMenu();

		// Initial map view
		mapAccessor = new MapAccessor(getInitialCenterPoint());
		zoomLevel = getInitialZoomLevel();
		mapAccessor.setZoom(zoomLevel);
		map.showMap(mapAccessor);
		
		map.addMapListener(new GeoMapListener() {
			@Override
			public void onZoom(int zoomLevel)
			{
				AbstractGeolocationView.this.zoomLevel = zoomLevel;
				mapAccessor.setZoom(zoomLevel);
				actionZoomIn.setEnabled(zoomLevel < MapAccessor.MAX_MAP_ZOOM);
				actionZoomOut.setEnabled(zoomLevel > MapAccessor.MIN_MAP_ZOOM);
			}

			@Override
			public void onPan(GeoLocation centerPoint)
			{
				mapAccessor.setLatitude(centerPoint.getLatitude());
				mapAccessor.setLongitude(centerPoint.getLongitude());
			}
		});
	}

	/**
	 * Create actual map viewer control
	 * 
	 * @param parent
	 * @param style
	 * @return
	 */
	protected abstract AbstractGeoMapViewer createMapViewer(Composite parent, int style);

	/**
	 * Create actions
	 */
	protected void createActions()
	{
      actionZoomIn = new Action(i18n.tr("Zoom &in"), SharedIcons.ZOOM_IN) {
			@Override
			public void run()
			{
				setZoomLevel(zoomLevel + 1);
			}
		};

      actionZoomOut = new Action(i18n.tr("Zoom &out"), SharedIcons.ZOOM_OUT) {
			@Override
			public void run()
			{
				setZoomLevel(zoomLevel - 1);
			}
		};
	}

	/**
	 * Create pop-up menu for variable list
	 */
	private void createPopupMenu()
	{
      MenuManager menuMgr = new ObjectContextMenuManager(this, this) {
         @Override
         protected void fillContextMenu()
         {
            if (getSelection().isEmpty())
            {
               AbstractGeolocationView.this.fillContextMenu(this);
            }
            else
            {
               super.fillContextMenu();
            }
         }
      };
		Menu menu = menuMgr.createContextMenu(map);
		map.setMenu(menu);
	}

   /**
    * Fill context menu if selection is empty
    *
    * @param manager menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
   }

	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolbar(ToolBarManager manager)
   {
      super.fillLocalToolbar(manager);
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
   }

   /**
    * Set new zoom level for map
    * 
    * @param newLevel new zoom level
    */
	private void setZoomLevel(int newLevel)
	{
		if ((newLevel < MapAccessor.MIN_MAP_ZOOM) || (newLevel > MapAccessor.MAX_MAP_ZOOM))
			return;
		
		zoomLevel = newLevel;
		mapAccessor.setZoom(zoomLevel);
		map.showMap(mapAccessor);
		
		actionZoomIn.setEnabled(zoomLevel < MapAccessor.MAX_MAP_ZOOM);
		actionZoomOut.setEnabled(zoomLevel > MapAccessor.MIN_MAP_ZOOM);
	}

	/**
	 * @return the mapAccessor
	 */
	protected MapAccessor getMapAccessor()
	{
		return mapAccessor;
	}
	
	/**
	 * Fire selection changed listeners
	 */
	protected void fireSelectionChangedListeners()
	{
	   SelectionChangedEvent e = new SelectionChangedEvent(this, selection);
      for(ISelectionChangedListener l : selectionChangeListeners)
         l.selectionChanged(e);
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void addSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionChangeListeners.add(listener);
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void removeSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionChangeListeners.remove(listener);
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
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void setSelection(ISelection selection)
   {
      this.selection = selection;
   }
}
