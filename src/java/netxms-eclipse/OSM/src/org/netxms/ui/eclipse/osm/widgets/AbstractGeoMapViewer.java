/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.osm.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseWheelListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.base.GeoLocation;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.osm.Activator;
import org.netxms.ui.eclipse.osm.GeoLocationCache;
import org.netxms.ui.eclipse.osm.GeoLocationCacheListener;
import org.netxms.ui.eclipse.osm.Messages;
import org.netxms.ui.eclipse.osm.tools.Area;
import org.netxms.ui.eclipse.osm.tools.MapAccessor;
import org.netxms.ui.eclipse.osm.tools.MapLoader;
import org.netxms.ui.eclipse.osm.tools.Tile;
import org.netxms.ui.eclipse.osm.tools.TileSet;
import org.netxms.ui.eclipse.osm.widgets.helpers.GeoMapListener;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.FontTools;

/**
 * This widget shows map retrieved via OpenStreetMap Static Map API
 */
public abstract class AbstractGeoMapViewer extends Canvas implements PaintListener, GeoLocationCacheListener, MouseWheelListener, MouseListener, MouseMoveListener
{
   protected static final String[] TITLE_FONTS = { "Segoe UI", "Liberation Sans", "DejaVu Sans", "Verdana", "Arial" };
   
   protected static final Color BORDER_COLOR = new Color(Display.getCurrent(), 0, 0, 0);
   
	protected static final int LABEL_ARROW_HEIGHT = 5;
	protected static final int LABEL_X_MARGIN = 4;
	protected static final int LABEL_Y_MARGIN = 4;
	protected static final int LABEL_SPACING = 4;

   private static final Color MAP_BACKGROUND = new Color(Display.getCurrent(), 255, 255, 255);
   private static final Color INFO_BLOCK_BACKGROUND = new Color(Display.getCurrent(), 0, 0, 0);
   private static final Color INFO_BLOCK_TEXT = new Color(Display.getCurrent(), 255, 255, 255);
   private static final Color SELECTION_COLOR = new Color(Display.getCurrent(), 0, 0, 255);
   
	private static final int DRAG_JITTER = 8;

   protected ColorCache colorCache;
	protected ILabelProvider labelProvider;
   protected Area coverage = new Area(0, 0, 0, 0);
   protected MapAccessor accessor;
   protected IViewPart viewPart = null;
   protected Point currentPoint;

   private Image currentImage = null;
	private Image bufferImage = null;
	private MapLoader mapLoader;
	private Point dragStartPoint = null;
	private Point selectionStartPoint = null;
	private Point selectionEndPoint = null;
	private Set<GeoMapListener> mapListeners = new HashSet<GeoMapListener>(0);
	private String title = null;
	private int offsetX;
	private int offsetY;
	private TileSet currentTileSet = null;
	private Image imageZoomIn;
	private Image imageZoomOut;
	private Rectangle zoomControlRect = null;
   private Font mapTitleFont;
	
	/**
	 * @param parent
	 * @param style
	 */
	public AbstractGeoMapViewer(Composite parent, int style)
	{
		super(parent, style | SWT.NO_BACKGROUND | SWT.DOUBLE_BUFFERED);
		
		colorCache = new ColorCache(this);
		
		imageZoomIn = Activator.getImageDescriptor("icons/map_zoom_in.png").createImage(); //$NON-NLS-1$
      imageZoomOut = Activator.getImageDescriptor("icons/map_zoom_out.png").createImage(); //$NON-NLS-1$
      
      mapTitleFont = FontTools.createFont(TITLE_FONTS, 2, SWT.BOLD);

		labelProvider = WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider();
		mapLoader = new MapLoader(getDisplay());

		setBackground(MAP_BACKGROUND);
		addPaintListener(this);

		final Runnable timer = new Runnable() {
			@Override
			public void run()
			{
				if (isDisposed())
					return;

				reloadMap();
			}
		};

		addListener(SWT.Resize, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				getDisplay().timerExec(-1, timer);
				getDisplay().timerExec(1000, timer);

				if (bufferImage != null)
					bufferImage.dispose();
				Rectangle rect = getClientArea();
				bufferImage = new Image(getDisplay(), rect.width, rect.height);
			}
		});

		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelProvider.dispose();
				GeoLocationCache.getInstance().removeListener(AbstractGeoMapViewer.this);
				if (bufferImage != null)
					bufferImage.dispose();
				if (currentImage != null)
					currentImage.dispose();
				mapLoader.dispose();
				imageZoomIn.dispose();
				imageZoomOut.dispose();
				mapTitleFont.dispose();
			}
		});

		addMouseListener(this);
		addMouseMoveListener(this);
		addMouseWheelListener(this);

		GeoLocationCache.getInstance().addListener(this);
	}
		
	/**
	 * Add zoom level change listener
	 * 
	 * @param listener
	 */
	public void addMapListener(GeoMapListener listener)
	{
		mapListeners.add(listener);
	}
	
	/**
	 * Remove previously registered zoom change listener
	 * 
	 * @param listener
	 */
	public void removeMapListener(GeoMapListener listener)
	{
		mapListeners.remove(listener);
	}
	
	/**
	 * Notify all listeners about zoom level change
	 */
	private void notifyOnZoomChange()
	{
		for(GeoMapListener listener : mapListeners)
			listener.onZoom(accessor.getZoom());
	}

	/**
	 * Notify all listeners about position change
	 */
	private void notifyOnPositionChange()
	{
		for(GeoMapListener listener : mapListeners)
			listener.onPan(accessor.getCenterPoint());
	}

	/**
	 * Show given map
	 * 
	 * @param accessor
	 */
	public void showMap(MapAccessor accessor)
	{
		this.accessor = new MapAccessor(accessor);
		reloadMap();
	}

	/**
	 * @param lat
	 * @param lon
	 * @param zoom
	 */
	public void showMap(double lat, double lon, int zoom)
	{
		showMap(new MapAccessor(lat, lon, zoom));
	}

	/**
	 * Reload current map
	 */
	private void reloadMap()
	{
		Rectangle rect = this.getClientArea();
		accessor.setMapWidth(rect.width);
		accessor.setMapHeight(rect.height);

		if (currentImage != null)
			currentImage.dispose();
		currentImage = null;
		
		if (!accessor.isValid())
			return;

		final Point mapSize = new Point(accessor.getMapWidth(), accessor.getMapHeight());
		final GeoLocation centerPoint = accessor.getCenterPoint();
		ConsoleJob job = new ConsoleJob(Messages.get().GeoMapViewer_DownloadJob_Title, viewPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final TileSet tiles = mapLoader.getAllTiles(mapSize, centerPoint, MapLoader.CENTER, accessor.getZoom(), true);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
                  if (isDisposed())
                     return;
                  
						currentTileSet = null;
						if (tiles != null)
						{
							drawTiles(tiles);
							if (tiles.missingTiles > 0)
							{
								currentTileSet = tiles;
								loadMissingTiles(tiles);
							}
							else
							{
								tiles.dispose();
							}
						}
						
						Point mapSize = new Point(currentImage.getImageData().width, currentImage.getImageData().height);
                  coverage = GeoLocationCache.calculateCoverage(mapSize, accessor.getCenterPoint(), GeoLocationCache.CENTER, accessor.getZoom());
                  onMapLoad();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().GeoMapViewer_DownloadError;
			}
		};
		job.setUser(false);
		job.start();
	}
	
	/**
	 * Map load handler. Called on UI thread after map was (re)loaded.
	 */
	protected abstract void onMapLoad();
	
	/**
	 * Load missing tiles in tile set
	 * 
	 * @param tiles
	 */
	private void loadMissingTiles(final TileSet tiles)
	{
		ConsoleJob job = new ConsoleJob(Messages.get().GeoMapViewer_LoadMissingJob_Title, viewPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				mapLoader.loadMissingTiles(tiles, new Runnable() {
					@Override
					public void run()
					{
						if (!AbstractGeoMapViewer.this.isDisposed() && (currentTileSet == tiles))
						{
							drawTiles(tiles);
							AbstractGeoMapViewer.this.redraw();
						}
						tiles.dispose();
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().GeoMapViewer_DownloadError;
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * @param tiles
	 */
	private void drawTiles(TileSet tileSet)
	{
		if (currentImage != null)
			currentImage.dispose();

		if ((tileSet == null) || (tileSet.tiles == null) || (tileSet.tiles.length == 0))
		{
			currentImage = null;
			return;
		}

		final Tile[][] tiles = tileSet.tiles;

		Point size = getSize();
		currentImage = new Image(getDisplay(), size.x, size.y);
		GC gc = new GC(currentImage);

		int x = tileSet.xOffset;
		int y = tileSet.yOffset;
		for(int i = 0; i < tiles.length; i++)
		{
			for(int j = 0; j < tiles[i].length; j++)
			{
				gc.drawImage(tiles[i][j].getImage(), x, y);
				x += 256;
				if (x >= size.x)
				{
					x = tileSet.xOffset;
					y += 256;
				}
			}
		}

		gc.dispose();
	}

	/*
	 * (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		final GC gc = new GC(bufferImage);
      gc.setAntialias(SWT.ON);
      gc.setTextAntialias(SWT.ON);
      
		GeoLocation currentLocation;

		// Draw objects and decorations if user is not dragging map
		// and map is not currently loading
		if (dragStartPoint == null)
		{
         currentLocation = accessor.getCenterPoint();
         
	      int imgW, imgH;
	      if (currentImage != null)
	      {
	         gc.drawImage(currentImage, -offsetX, -offsetY);
	         imgW = currentImage.getImageData().width;
	         imgH = currentImage.getImageData().height;
	      }
	      else
	      {
	         imgW = -1;
	         imgH = -1;
	      }

	      drawContent(gc, currentLocation, imgW, imgH);
		}
		else
		{
		   Point cp = GeoLocationCache.coordinateToDisplay(accessor.getCenterPoint(), accessor.getZoom());
		   cp.x += offsetX;
		   cp.y += offsetY;
		   currentLocation = GeoLocationCache.displayToCoordinates(cp, accessor.getZoom());
		   
	      Point size = getSize();
		   TileSet tileSet = mapLoader.getAllTiles(size, currentLocation, MapLoader.CENTER, accessor.getZoom(), true);
	      int x = tileSet.xOffset;
	      int y = tileSet.yOffset;
	      final Tile[][] tiles = tileSet.tiles;
	      for(int i = 0; i < tiles.length; i++)
	      {
	         for(int j = 0; j < tiles[i].length; j++)
	         {
	            gc.drawImage(tiles[i][j].getImage(), x, y);
	            x += 256;
	            if (x >= size.x)
	            {
	               x = tileSet.xOffset;
	               y += 256;
	            }
	         }
	      }
	      tileSet.dispose();
		}
		
		// Draw selection rectangle
		if ((selectionStartPoint != null) && (selectionEndPoint != null))
		{
			int x = Math.min(selectionStartPoint.x, selectionEndPoint.x);
			int y = Math.min(selectionStartPoint.y, selectionEndPoint.y);
			int w = Math.abs(selectionStartPoint.x - selectionEndPoint.x);
			int h = Math.abs(selectionStartPoint.y - selectionEndPoint.y);
			gc.setBackground(SELECTION_COLOR);
			gc.setForeground(SELECTION_COLOR);
			gc.setAlpha(64);
			gc.fillRectangle(x, y, w, h);
			gc.setAlpha(255);
			gc.setLineWidth(2);
			gc.drawRectangle(x, y, w, h);
		}
		
		// Draw current location info
      String text = currentLocation.toString();
      Point textSize = gc.textExtent(text);

      Rectangle rect = getClientArea();
      rect.x = rect.width - textSize.x - 20;
      rect.y += 10;
      rect.width = textSize.x + 10;
      rect.height = textSize.y + 8;

      gc.setBackground(INFO_BLOCK_BACKGROUND);
      gc.setAlpha(128);
      gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
      gc.setAlpha(255);

      gc.setForeground(INFO_BLOCK_TEXT);
      gc.drawText(text, rect.x + 5, rect.y + 4, true);
		
		// Draw title
		if ((title != null) && !title.isEmpty())
		{
			gc.setFont(mapTitleFont);
			rect = getClientArea();
			int x = (rect.width - gc.textExtent(title).x) / 2;
			gc.setForeground(SharedColors.getColor(SharedColors.GEOMAP_TITLE, getDisplay()));
			gc.drawText(title, x, 10, true);
		}
		
		// Draw zoom control
		gc.setFont(JFaceResources.getHeaderFont());
		text = Integer.toString(accessor.getZoom());
		textSize = gc.textExtent(text);
		
      rect = getClientArea();
      rect.x = 10;
      rect.y = 10;
      rect.width = 80;
      rect.height = 47 + textSize.y;

      gc.setBackground(INFO_BLOCK_BACKGROUND);
      gc.setAlpha(128);
      gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
      gc.setAlpha(255);
      
      gc.drawText(text, rect.x + rect.width / 2 - textSize.x / 2, rect.y + 5, true);
      gc.drawImage(imageZoomIn, rect.x + 5, rect.y + rect.height - 37);
      gc.drawImage(imageZoomOut, rect.x + 42, rect.y + rect.height - 37);
      
      zoomControlRect = rect;

      gc.dispose();
      e.gc.drawImage(bufferImage, 0, 0);      
	}
	
	/**
	 * Draw content over map
	 * 
	 * @param gc
	 * @param currentLocation current location (map center)
	 * @param imgW map image width
	 * @param imgH map image height
	 */
	protected abstract void drawContent(GC gc, GeoLocation currentLocation, int imgW, int imgH);

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.ui.eclipse.osm.GeoLocationCacheListener#geoLocationCacheChanged
	 * (org.netxms.client.objects.AbstractObject, org.netxms.client.GeoLocation)
	 */
	@Override
	public void geoLocationCacheChanged(final AbstractObject object, final GeoLocation prevLocation)
	{
		getDisplay().asyncExec(new Runnable() {
			@Override
			public void run()
			{
		      onCacheChange(object, prevLocation);
			}
		});
	}

	/**
	 * Real handler for geolocation cache changes. Must be run in UI thread.
	 * 
	 * @param object
	 * @param prevLocation
	 */
	protected abstract void onCacheChange(final AbstractObject object, final GeoLocation prevLocation);
	
	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.MouseWheelListener#mouseScrolled(org.eclipse.swt.events.MouseEvent)
	 */
	@Override
	public void mouseScrolled(MouseEvent event)
	{
		int zoom = accessor.getZoom();
		if (event.count > 0)
		{
			if (zoom < 18)
				zoom++;
		}
		else
		{
			if (zoom > 1)
				zoom--;
		}
		
		if (zoom != accessor.getZoom())
		{
			accessor.setZoom(zoom);
			reloadMap();
			notifyOnZoomChange();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.MouseListener#mouseDoubleClick(org.eclipse.swt.events.MouseEvent)
	 */
	@Override
	public void mouseDoubleClick(MouseEvent e)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.MouseListener#mouseDown(org.eclipse.swt.events.MouseEvent)
	 */
	@Override
	public void mouseDown(MouseEvent e)
	{
		if (e.button == 1) // left button, ignore if map is currently loading
		{
		   if (zoomControlRect.contains(e.x, e.y))
		   {
		      Rectangle r = new Rectangle(zoomControlRect.x + 5, zoomControlRect.y + zoomControlRect.height - 37, 32, 32);
		      int zoom = accessor.getZoom();
		      if (r.contains(e.x, e.y))
		      {
		         if (zoom < 18)
		            zoom++;
		      }
		      else
		      {
		         r.x += 37;
               if (r.contains(e.x, e.y))
               {
                  if (zoom > 1)
                     zoom--;
               }
		      }
		      
		      if (zoom != accessor.getZoom())
		      {
		         accessor.setZoom(zoom);
		         reloadMap();
		         notifyOnZoomChange();
		      }
		   }
		   else if ((e.stateMask & SWT.SHIFT) != 0)
			{
				if (accessor.getZoom() < 18)
					selectionStartPoint = new Point(e.x, e.y);
			}
			else
			{
				dragStartPoint = new Point(e.x, e.y);
				setCursor(getDisplay().getSystemCursor(SWT.CURSOR_SIZEALL));
			}
		}

		currentPoint = new Point(e.x, e.y);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.MouseListener#mouseUp(org.eclipse.swt.events.MouseEvent)
	 */
	@Override
	public void mouseUp(MouseEvent e)
	{
		if ((e.button == 1) && (dragStartPoint != null))
		{
			if (Math.abs(offsetX) > DRAG_JITTER || Math.abs(offsetY) > DRAG_JITTER)
			{
				final Point centerXY = GeoLocationCache.coordinateToDisplay(accessor.getCenterPoint(), accessor.getZoom());
				centerXY.x += offsetX;
				centerXY.y += offsetY;
				final GeoLocation geoLocation = GeoLocationCache.displayToCoordinates(centerXY, accessor.getZoom());
				accessor.setLatitude(geoLocation.getLatitude());
				accessor.setLongitude(geoLocation.getLongitude());
				reloadMap();
				notifyOnPositionChange();
			}
			offsetX = 0;
			offsetY = 0;
			dragStartPoint = null;
			setCursor(null);
		}
		if ((e.button == 1) && (selectionStartPoint != null))
		{
			if (selectionEndPoint != null)
			{
				int x1 = Math.min(selectionStartPoint.x, selectionEndPoint.x);
				int x2 = Math.max(selectionStartPoint.x, selectionEndPoint.x);
				int y1 = Math.min(selectionStartPoint.y, selectionEndPoint.y);
				int y2 = Math.max(selectionStartPoint.y, selectionEndPoint.y);

				final GeoLocation l1 = getLocationAtPoint(new Point(x1, y1));
				final GeoLocation l2 = getLocationAtPoint(new Point(x2, y2));
				final GeoLocation lc = getLocationAtPoint(new Point(x2 - (x2 - x1) / 2, y2 - (y2 - y1) / 2));

				int zoom = accessor.getZoom();
				while(zoom < 18)
				{
					zoom++;
					final Area area = GeoLocationCache.calculateCoverage(getSize(), lc, GeoLocationCache.CENTER, zoom);
					if (!area.contains(l1.getLatitude(), l1.getLongitude()) ||
					    !area.contains(l2.getLatitude(), l2.getLongitude()))
					{
						zoom--;
						break;
					}
				}

				if (zoom != accessor.getZoom())
				{
					accessor.setZoom(zoom);
					accessor.setLatitude(lc.getLatitude());
					accessor.setLongitude(lc.getLongitude());
					reloadMap();
					notifyOnPositionChange();
					notifyOnZoomChange();
				}
			}
			selectionStartPoint = null;
			selectionEndPoint = null;
			redraw();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.MouseMoveListener#mouseMove(org.eclipse.swt.events.MouseEvent)
	 */
	@Override
	public void mouseMove(MouseEvent e)
	{
		if (dragStartPoint != null)
		{
			int deltaX = dragStartPoint.x - e.x;
			int deltaY = dragStartPoint.y - e.y;
			if (Math.abs(deltaX) > DRAG_JITTER || Math.abs(deltaY) > DRAG_JITTER)
			{
				offsetX = deltaX;
				offsetY = deltaY;
				redraw();
			}
		}
		if (selectionStartPoint != null)
		{
			int deltaX = selectionStartPoint.x - e.x;
			int deltaY = selectionStartPoint.y - e.y;
			if (Math.abs(deltaX) > DRAG_JITTER || Math.abs(deltaY) > DRAG_JITTER)
			{
				selectionEndPoint = new Point(e.x, e.y);
				redraw();
			}
		}
	}

	/**
	 * @return the currentPoint
	 */
	public Point getCurrentPoint()
	{
		return new Point(currentPoint.x, currentPoint.y);
	}
	
	/**
	 * Get location at given point within widget
	 * 
	 * @param p widget-related coordinates
	 * @return location (latitude/longitude) at given point
	 */
	public GeoLocation getLocationAtPoint(Point p)
	{
		Point cp = GeoLocationCache.coordinateToDisplay(new GeoLocation(coverage.getxHigh(), coverage.getyLow()), accessor.getZoom());
		return GeoLocationCache.displayToCoordinates(new Point(cp.x + p.x, cp.y + p.y), accessor.getZoom());
	}
	
	/**
	 * @return the viewPart
	 */
	public IViewPart getViewPart()
	{
		return viewPart;
	}

	/**
	 * @param viewPart the viewPart to set
	 */
	public void setViewPart(IViewPart viewPart)
	{
		this.viewPart = viewPart;
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}

   /**
    * Get object at given point within widget
    * 
    * @param p widget-related coordinates
    * @return object at given point or null
    */
   public abstract AbstractObject getObjectAtPoint(Point p);
}
