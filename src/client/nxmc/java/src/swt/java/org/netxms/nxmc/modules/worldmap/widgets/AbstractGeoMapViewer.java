/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.worldmap.widgets;

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
import org.netxms.base.GeoLocation;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.netxms.nxmc.modules.worldmap.GeoLocationCache;
import org.netxms.nxmc.modules.worldmap.GeoLocationCacheListener;
import org.netxms.nxmc.modules.worldmap.GeoMapListener;
import org.netxms.nxmc.modules.worldmap.tools.Area;
import org.netxms.nxmc.modules.worldmap.tools.MapAccessor;
import org.netxms.nxmc.modules.worldmap.tools.MapLoader;
import org.netxms.nxmc.modules.worldmap.tools.Tile;
import org.netxms.nxmc.modules.worldmap.tools.TileSet;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.FontTools;
import org.xnap.commons.i18n.I18n;

/**
 * This widget shows map retrieved via OpenStreetMap Static Map API
 */
public abstract class AbstractGeoMapViewer extends Canvas implements PaintListener, GeoLocationCacheListener, MouseWheelListener, MouseListener, MouseMoveListener
{
   private final I18n i18n = LocalizationHelper.getI18n(AbstractGeoMapViewer.class);

   protected static final Color BORDER_COLOR = new Color(Display.getCurrent(), 0, 0, 0);

   protected static final int LABEL_ARROW_HEIGHT = 20;
   protected static final int LABEL_X_MARGIN = 12;
   protected static final int LABEL_Y_MARGIN = 12;
   protected static final int LABEL_SPACING = 4;

   protected static final Color SELECTION_COLOR = new Color(Display.getCurrent(), 0, 0, 255);

	private static final int DRAG_JITTER = 8;

   private static final int BUTTON_MARGIN_WIDTH = 10;
   private static final int BUTTON_MARGIN_HEIGHT = 5;
   private static final int BUTTON_BLOCK_MARGIN_WIDTH = 10;
   private static final int BUTTON_BLOCK_MARGIN_HEIGHT = 15;

   protected ColorCache colorCache;
	protected ILabelProvider labelProvider;
   protected Area coverage = new Area(0, 0, 0, 0);
   protected MapAccessor accessor = new MapAccessor(0.0, 0.0, 4);
   protected View view = null;
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
   private int contentVerticalOffset = 0; // Vertical offset for drawing map content
	private TileSet currentTileSet = null;
	private Rectangle zoomControlRect = null;
   private Rectangle titleRect = null;
   private Font controlFont;
   private boolean enableControls = true;
   private boolean fullVerticalCoverage = false;

   /**
    * Create abstract geo map viewer.
    *
    * @param parent parent control
    * @param style control style
    * @param view owning view
    */
   public AbstractGeoMapViewer(Composite parent, int style, View view)
	{
		super(parent, style | SWT.NO_BACKGROUND | SWT.DOUBLE_BUFFERED);
      this.view = view;

		colorCache = new ColorCache(this);

      controlFont = FontTools.createAdjustedFont(JFaceResources.getTextFont(), 12, SWT.BOLD);

      labelProvider = new DecoratingObjectLabelProvider();
		mapLoader = new MapLoader(getDisplay());

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

            checkVerticalCoverage();
			}
		});

		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            removePaintListener(AbstractGeoMapViewer.this);
				labelProvider.dispose();
				GeoLocationCache.getInstance().removeListener(AbstractGeoMapViewer.this);
				if (bufferImage != null)
					bufferImage.dispose();
				if (currentImage != null)
					currentImage.dispose();
				mapLoader.dispose();
            controlFont.dispose();
			}
		});

		addMouseListener(this);
		addMouseMoveListener(this);
		addMouseWheelListener(this);

      GeoLocationCache.getInstance().addListener(this);
	}

   /**
    * Enable/disable map controls (zoom, scroll, etc.)
    *
    * @param enable true to enable
    */
   public void enableMapControls(boolean enable)
   {
      enableControls = enable;
      redraw();
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
	public void reloadMap()
	{
      Point size = getSize();
      Point virtualMapSize = GeoLocationCache.getVirtualMapSize(accessor.getZoom());
      if (virtualMapSize.y < size.y)
         size.y = virtualMapSize.y;
      accessor.setMapWidth(size.x);
      accessor.setMapHeight(size.y);

		if (currentImage != null)
			currentImage.dispose();
		currentImage = null;

		if (!accessor.isValid())
			return;

		final Point mapSize = new Point(accessor.getMapWidth(), accessor.getMapHeight());
		final GeoLocation centerPoint = accessor.getCenterPoint();
      Job job = new Job(i18n.tr("Downloading map tiles from server"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
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
                     AbstractGeoMapViewer.this.redraw();
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
            return i18n.tr("Cannot download map tiles from server");
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
    * @param tiles tile set to load tiles for
    */
	private void loadMissingTiles(final TileSet tiles)
	{
      Job job = new Job(i18n.tr("Loading missing tiles"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
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
						else
						{
						   tiles.cancelled = true; // Inform loader tasks that this tile set is no longer needed
						}
						if (tiles.missingTiles == 0)
						   tiles.dispose();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot load map tiles");
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
      Point virtualMapSize = GeoLocationCache.getVirtualMapSize(accessor.getZoom());
      if (size.y > virtualMapSize.y)
      {
         contentVerticalOffset = (size.y - virtualMapSize.y) / 2;
         size.y = virtualMapSize.y;
      }
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

   /**
    * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
    */
	@Override
	public void paintControl(PaintEvent e)
	{
      if (bufferImage == null)
      {
         e.gc.fillRectangle(getClientArea());
         return;
      }

      final GC gc = new GC(bufferImage);
      gc.setAntialias(SWT.ON);
      gc.setTextAntialias(SWT.ON);

      if (contentVerticalOffset > 0)
      {
         gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WIDGET_BACKGROUND));

         Rectangle rect = getClientArea();
         rect.y = rect.height - contentVerticalOffset - 1;
         rect.height = contentVerticalOffset + 1;
         gc.fillRectangle(rect);

         rect.y = 0;
         gc.fillRectangle(rect);
      }

		GeoLocation currentLocation;

		// Draw objects and decorations if user is not dragging map
		// and map is not currently loading
		if (dragStartPoint == null)
		{
         currentLocation = accessor.getCenterPoint();

	      int imgW, imgH;
	      if (currentImage != null)
	      {
            gc.drawImage(currentImage, -offsetX, contentVerticalOffset - offsetY);
	         imgW = currentImage.getImageData().width;
	         imgH = currentImage.getImageData().height;
	      }
	      else
	      {
	         imgW = -1;
	         imgH = -1;
	      }

         drawContent(gc, currentLocation, imgW, imgH, contentVerticalOffset);
		}
		else
		{
		   Point cp = GeoLocationCache.coordinateToDisplay(accessor.getCenterPoint(), accessor.getZoom());
		   cp.x += offsetX;
		   cp.y += offsetY;
         currentLocation = GeoLocationCache.displayToCoordinates(cp, accessor.getZoom(), true);

	      Point size = getSize();
		   TileSet tileSet = mapLoader.getAllTiles(size, currentLocation, MapLoader.CENTER, accessor.getZoom(), true);
	      int x = tileSet.xOffset;
	      int y = tileSet.yOffset;
	      final Tile[][] tiles = tileSet.tiles;
	      for(int i = 0; i < tiles.length; i++)
	      {
	         for(int j = 0; j < tiles[i].length; j++)
	         {
               gc.drawImage(tiles[i][j].getImage(), x, y + contentVerticalOffset);
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
      gc.setFont(null);
      String text = "Map data \u00a9 OpenStreetMap contributors\tZoom level " + accessor.getZoom() + "\tCentered at " + currentLocation.toString();
      Point infoTextSize = gc.textExtent(text);

      Rectangle infoTextRectangle = getClientArea();
      infoTextRectangle.x = infoTextRectangle.width - infoTextSize.x - 10;
      infoTextRectangle.y = infoTextRectangle.height - infoTextSize.y - 8;
      infoTextRectangle.width = infoTextSize.x + 11;
      infoTextRectangle.height = infoTextSize.y + 9;

      gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_WIDGET_BACKGROUND));
      gc.fillRectangle(infoTextRectangle.x, infoTextRectangle.y, infoTextRectangle.width, infoTextRectangle.height);

      gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_WIDGET_FOREGROUND));
      gc.drawText(text, infoTextRectangle.x + 5, infoTextRectangle.y + 4, true);

		// Draw title
		if ((title != null) && !title.isEmpty())
		{
         gc.setFont(JFaceResources.getBannerFont());
         Point size = gc.textExtent(title);
         Rectangle rect = getClientArea();
         rect.x = rect.width / 2 - size.x / 2 - 8;
         rect.y = 10;
         rect.width = size.x + 16;
         rect.height = size.y + 10;

         gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
         gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);

         gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_WIDGET_BORDER));
         gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);

         gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_LIST_FOREGROUND));
         gc.drawText(title, rect.x + 8, rect.y + 6);

         titleRect = rect;
      }
      else
      {
         titleRect = null;
		}

		// Draw zoom control
      if (enableControls)
      {
         gc.setFont(controlFont);

         Point buttonSize = gc.textExtent("+");
         buttonSize.x += BUTTON_MARGIN_WIDTH * 2;
         buttonSize.y += BUTTON_MARGIN_HEIGHT * 2;

         Rectangle rect = getClientArea();
         rect.x = rect.width - buttonSize.x - BUTTON_BLOCK_MARGIN_WIDTH;
         rect.y = rect.height - buttonSize.y * 2 - infoTextRectangle.height - BUTTON_BLOCK_MARGIN_HEIGHT;
         rect.width = buttonSize.x;
         rect.height = buttonSize.y * 2 + 1;

         gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
         gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);

         gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_WIDGET_BORDER));
         gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
         gc.drawLine(rect.x, rect.y + buttonSize.y + 1, rect.x + rect.width, rect.y + buttonSize.y + 1);

         gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_LIST_FOREGROUND));
         gc.drawText("+", rect.x + 10, rect.y + 5, true);
         gc.drawText("-", rect.x + 10, rect.y + buttonSize.y + 5, true);

         zoomControlRect = rect;
      }

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
    * @param verticalOffset offset from control's top border
    */
   protected abstract void drawContent(GC gc, GeoLocation currentLocation, int imgW, int imgH, int verticalOffset);

	/**
	 * @see org.netxms.nxmc.modules.worldmap.GeoLocationCacheListener#geoLocationCacheChanged(org.netxms.client.objects.AbstractObject, org.netxms.base.GeoLocation)
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
	
   /**
    * @see org.eclipse.swt.events.MouseWheelListener#mouseScrolled(org.eclipse.swt.events.MouseEvent)
    */
	@Override
	public void mouseScrolled(MouseEvent e)
	{
      if (!enableControls)
         return;

		int zoom = accessor.getZoom();
		if (e.count > 0)
		{
			if (zoom < MapAccessor.MAX_MAP_ZOOM)
				zoom++;
		}
		else
		{
         if (zoom > 1)
         {
            Point virtualMapSize = GeoLocationCache.getVirtualMapSize(zoom);
            Point widgetSize = getSize();
            if ((widgetSize.x < virtualMapSize.x) || (widgetSize.y < virtualMapSize.y))
               zoom--;
         }
		}

		if (zoom != accessor.getZoom())
		{
         // Get current location under cursor and make sure that it is still visible after zoom
         Point pt = new Point(e.x, e.y);
         GeoLocation oldLocation = getLocationAtPoint(pt);

         Point mapSize = new Point(accessor.getMapWidth(), accessor.getMapHeight());
         Area newCoverage = GeoLocationCache.calculateCoverage(mapSize, accessor.getCenterPoint(), GeoLocationCache.CENTER, zoom);
         Point cp = GeoLocationCache.coordinateToDisplay(new GeoLocation(newCoverage.getxHigh(), newCoverage.getyLow()), zoom);
         GeoLocation newLocation = GeoLocationCache.displayToCoordinates(new Point(cp.x + pt.x, cp.y + pt.y), zoom, true);

         accessor.setLatitude(accessor.getLatitude() - (newLocation.getLatitude() - oldLocation.getLatitude()));
         accessor.setLongitude(accessor.getLongitude() - (newLocation.getLongitude() - oldLocation.getLongitude()));
         accessor.setZoom(zoom);

         checkVerticalCoverage();
			reloadMap();
			notifyOnZoomChange();
		}
	}

   /**
    * @see org.eclipse.swt.events.MouseListener#mouseDoubleClick(org.eclipse.swt.events.MouseEvent)
    */
	@Override
	public void mouseDoubleClick(MouseEvent e)
	{
      if (!enableControls || (e.button != 1) || zoomControlRect.contains(e.x, e.y) || ((titleRect != null) && titleRect.contains(e.x, e.y)))
         return;

      int zoom = accessor.getZoom();
      if (zoom < MapAccessor.MAX_MAP_ZOOM)
      {
         int step = ((e.stateMask & SWT.SHIFT) != 0) ? 4 : 1;
         zoom = (zoom + step > MapAccessor.MAX_MAP_ZOOM) ? MapAccessor.MAX_MAP_ZOOM : zoom + step;

         final GeoLocation geoLocation = getLocationAtPoint(new Point(e.x, e.y));
         accessor.setZoom(zoom);
         accessor.setLatitude(geoLocation.getLatitude());
         accessor.setLongitude(geoLocation.getLongitude());
         checkVerticalCoverage();
         reloadMap();
         notifyOnZoomChange();
         notifyOnPositionChange();
      }
	}

   /**
    * @see org.eclipse.swt.events.MouseListener#mouseDown(org.eclipse.swt.events.MouseEvent)
    */
	@Override
	public void mouseDown(MouseEvent e)
	{
      if (!enableControls)
         return;

		if (e.button == 1) // left button, ignore if map is currently loading
		{
		   if (zoomControlRect.contains(e.x, e.y))
		   {
            Rectangle r = new Rectangle(zoomControlRect.x, zoomControlRect.y, zoomControlRect.width, zoomControlRect.height / 2);
		      int zoom = accessor.getZoom();
		      if (r.contains(e.x, e.y))
		      {
		         if (zoom < MapAccessor.MAX_MAP_ZOOM)
		            zoom++;
		      }
		      else
		      {
               r.y += zoomControlRect.height / 2 + 1;
               if (r.contains(e.x, e.y) && (zoom > 1))
               {
                  Point virtualMapSize = GeoLocationCache.getVirtualMapSize(zoom);
                  Point widgetSize = getSize();
                  if ((widgetSize.x < virtualMapSize.x) || (widgetSize.y < virtualMapSize.y))
                     zoom--;
               }
		      }

		      if (zoom != accessor.getZoom())
		      {
		         accessor.setZoom(zoom);
               checkVerticalCoverage();
		         reloadMap();
		         notifyOnZoomChange();
		      }
		   }
         else if ((titleRect != null) && titleRect.contains(e.x, e.y))
         {
            // Click on title, do nothing
         }
		   else if ((e.stateMask & SWT.SHIFT) != 0)
			{
				if (accessor.getZoom() < MapAccessor.MAX_MAP_ZOOM)
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

   /**
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
            final GeoLocation geoLocation = GeoLocationCache.displayToCoordinates(centerXY, accessor.getZoom(), true);
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
				while(zoom < MapAccessor.MAX_MAP_ZOOM)
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
               checkVerticalCoverage();
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

   /**
    * @see org.eclipse.swt.events.MouseMoveListener#mouseMove(org.eclipse.swt.events.MouseEvent)
    */
	@Override
	public void mouseMove(MouseEvent e)
	{
		if (dragStartPoint != null)
		{
			int deltaX = dragStartPoint.x - e.x;
         int deltaY = fullVerticalCoverage ? 0 : dragStartPoint.y - e.y; // Forbid vertical drag if widget shows entire map height
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
      return GeoLocationCache.displayToCoordinates(new Point(cp.x + p.x, cp.y + p.y - contentVerticalOffset), accessor.getZoom(), true);
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

   /**
    * Check map vertical coverage
    */
   private void checkVerticalCoverage()
   {
      Point size = getSize();
      Point virtualMapSize = GeoLocationCache.getVirtualMapSize(accessor.getZoom());
      if (size.y >= virtualMapSize.y)
      {
         fullVerticalCoverage = true;
         contentVerticalOffset = (size.y - virtualMapSize.y) / 2;
         accessor.setLatitude(0); // center map vertically
      }
      else
      {
         fullVerticalCoverage = false;
         contentVerticalOffset = 0;
      }
   }
}
