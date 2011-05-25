/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
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
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.GeoLocation;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.osm.GeoLocationCache;
import org.netxms.ui.eclipse.osm.GeoLocationCacheListener;
import org.netxms.ui.eclipse.osm.tools.Area;
import org.netxms.ui.eclipse.osm.tools.MapAccessor;
import org.netxms.ui.eclipse.osm.tools.MapLoader;
import org.netxms.ui.eclipse.osm.tools.TileSet;
import org.netxms.ui.eclipse.osm.widgets.helpers.GeoMapZoomListener;
import org.netxms.ui.eclipse.widgets.AnimatedImage;

/**
 * This widget shows map retrieved via OpenStreetMap Static Map API
 */
public class GeoMapViewer extends Canvas implements PaintListener, GeoLocationCacheListener, MouseWheelListener, MouseListener, MouseMoveListener
{
	private static final Color MAP_BACKGROUND = new Color(Display.getDefault(), 255, 255, 255);
	private static final Color INFO_BLOCK_BACKGROUND = new Color(Display.getDefault(), 150, 240, 88);
	private static final Color INFO_BLOCK_BORDER = new Color(Display.getDefault(), 0, 0, 0);
	private static final Color INFO_BLOCK_TEXT = new Color(Display.getDefault(), 0, 0, 0);
	private static final Color LABEL_BACKGROUND = new Color(Display.getDefault(), 240, 254, 192);
	private static final Color LABEL_BORDER = new Color(Display.getDefault(), 0, 0, 0);

	private static final int LABEL_ARROW_HEIGHT = 20;
	private static final int LABEL_ARROW_OFFSET = 10;
	private static final int LABEL_X_MARGIN = 4;
	private static final int LABEL_Y_MARGIN = 4;
	private static final int LABEL_SPACING = 4;

	private static final int DRAG_JITTER = 8;

	private ILabelProvider labelProvider;
	private Image currentImage = null;
	private Area coverage = null;
	private List<GenericObject> objects = new ArrayList<GenericObject>();
	private MapAccessor accessor;
	private IWorkbenchSiteProgressService siteService = null;
	private GenericObject centerMarker = null;
	private AnimatedImage waitingImage = null;
	private Point dragStartPoint = null;
	private boolean loading = false;
	private Set<GeoMapZoomListener> zoomListeners = new HashSet<GeoMapZoomListener>(0);

	private int offsetX;
	private int offsetY;

	/**
	 * @param parent
	 * @param style
	 */
	public GeoMapViewer(Composite parent, int style)
	{
		super(parent, style);

		labelProvider = WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider();

		setBackground(MAP_BACKGROUND);
		addPaintListener(this);

		final Runnable timer = new Runnable()
		{
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
				getDisplay().timerExec(1000, timer);

				Rectangle rect = getClientArea();
				Point size = waitingImage.getSize();
				waitingImage.setLocation(rect.x + rect.width / 2 - size.x, rect.y + rect.height / 2 - size.y);
			}
		});

		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelProvider.dispose();
				GeoLocationCache.getInstance().removeListener(GeoMapViewer.this);
			}
		});

		addMouseListener(this);
		addMouseMoveListener(this);
		addMouseWheelListener(this);

		waitingImage = new AnimatedImage(this, SWT.NONE);
		waitingImage.setVisible(false);

		GeoLocationCache.getInstance().addListener(this);
	}
	
	/**
	 * Add zoom level change listener
	 * 
	 * @param listener
	 */
	public void addZoomListener(GeoMapZoomListener listener)
	{
		zoomListeners.add(listener);
	}
	
	/**
	 * Remove previously registered zoom change listener
	 * 
	 * @param listener
	 */
	public void removeZoomListener(GeoMapZoomListener listener)
	{
		zoomListeners.remove(listener);
	}
	
	/**
	 * Notify all zoom listeners
	 */
	private void notifyZoomListeners()
	{
		for(GeoMapZoomListener listener : zoomListeners)
			listener.onZoom(accessor.getZoom());
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
		
		loading = true;

		redraw();
		waitingImage.setVisible(true);
		try
		{
			waitingImage.setImage(new URL("platform:/plugin/org.netxms.ui.eclipse.library/icons/loading.gif"));
		}
		catch(MalformedURLException e)
		{
		}

		if (!accessor.isValid())
			return;

		final Point mapSize = new Point(accessor.getMapWidth(), accessor.getMapHeight());
		final GeoLocation centerPoint = accessor.getCenterPoint();
		Job job = new Job("Download map image")
		{
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				final TileSet tiles = MapLoader.getAllTiles(mapSize, centerPoint, accessor.getZoom());

				new UIJob("Redraw map")
				{
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						drawTiles(tiles);
						Point mapSize = new Point(currentImage.getImageData().width, currentImage.getImageData().height);
						coverage = GeoLocationCache.calculateCoverage(mapSize, accessor.getCenterPoint(), accessor.getZoom());
						objects = GeoLocationCache.getInstance().getObjectsInArea(coverage);
						waitingImage.setImage(null);
						waitingImage.setVisible(false);
						GeoMapViewer.this.redraw();
						loading = false;
						return Status.OK_STATUS;
					}
				}.schedule();
				return Status.OK_STATUS;
			}
		};
		if (siteService != null)
			siteService.schedule(job, 0, true);
		else
			job.schedule();
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

		final Image[][] tiles = tileSet.tiles;

		Point size = getSize();
		currentImage = new Image(getDisplay(), size.x, size.y);
		GC gc = new GC(currentImage);

		int x = tileSet.xOffset;
		int y = tileSet.yOffset;
		for(int i = 0; i < tiles.length; i++)
		{
			for(int j = 0; j < tiles[i].length; j++)
			{
				gc.drawImage(tiles[i][j], x, y);
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
	 * @return the siteService
	 */
	public IWorkbenchSiteProgressService getSiteService()
	{
		return siteService;
	}

	/**
	 * @param siteService
	 *           the siteService to set
	 */
	public void setSiteService(IWorkbenchSiteProgressService siteService)
	{
		this.siteService = siteService;
	}

	/*
	 * (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		final GC gc = e.gc;

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

		// Draw objects and decorations if user is not dragging map
		if (dragStartPoint == null)
		{
			final Point centerXY = GeoLocationCache.coordinateToDisplay(accessor.getCenterPoint(), accessor.getZoom());
			for(GenericObject object : objects)
			{
				final Point virtualXY = GeoLocationCache.coordinateToDisplay(object.getGeolocation(), accessor.getZoom());
				final int dx = virtualXY.x - centerXY.x;
				final int dy = virtualXY.y - centerXY.y;
				drawObject(gc, imgW / 2 + dx, imgH / 2 + dy, object);
			}
	
			final GeoLocation gl = new GeoLocation(accessor.getLatitude(), accessor.getLongitude());
			final String text = gl.toString();
			final Point textSize = gc.textExtent(text);
	
			Rectangle rect = getClientArea();
			rect.x = 10;
			// rect.x = rect.width - textSize.x - 20;
			rect.y += 10;
			rect.width = textSize.x + 10;
			rect.height = textSize.y + 8;
	
			gc.setAntialias(SWT.ON);
			gc.setBackground(INFO_BLOCK_BACKGROUND);
			gc.setAlpha(192);
			gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
			gc.setAlpha(255);
			gc.setForeground(INFO_BLOCK_BORDER);
			gc.setLineWidth(1);
			gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
	
			gc.setForeground(INFO_BLOCK_TEXT);
			gc.drawText(text, rect.x + 5, rect.y + 4, true);
		}
	}

	/**
	 * Draw object on map
	 * 
	 * @param gc
	 * @param x
	 * @param y
	 * @param object
	 */
	private void drawObject(GC gc, int x, int y, GenericObject object)
	{
		final String text = object.getObjectName();
		final Image image = labelProvider.getImage(object);
		final Point textSize = gc.textExtent(text);

		Rectangle rect = new Rectangle(x - LABEL_ARROW_OFFSET, y - LABEL_ARROW_HEIGHT - textSize.y, textSize.x
				+ image.getImageData().width + LABEL_X_MARGIN * 2 + LABEL_SPACING, Math.max(image.getImageData().height, textSize.y)
				+ LABEL_Y_MARGIN * 2);

		gc.setBackground(LABEL_BACKGROUND);
		gc.setForeground(LABEL_BORDER);
		gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
		gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);

		final int[] arrow = new int[] { rect.x + LABEL_ARROW_OFFSET - 4, rect.y + rect.height, x, y, rect.x + LABEL_ARROW_OFFSET + 4,
				rect.y + rect.height };
		gc.fillPolygon(arrow);
		gc.setForeground(LABEL_BACKGROUND);
		gc.drawLine(arrow[0], arrow[1], arrow[4], arrow[5]);
		gc.setForeground(LABEL_BORDER);
		gc.drawPolyline(arrow);

		gc.drawImage(image, rect.x + LABEL_X_MARGIN, rect.y + LABEL_Y_MARGIN);
		gc.drawText(text, rect.x + LABEL_X_MARGIN + image.getImageData().width + LABEL_SPACING, rect.y + LABEL_Y_MARGIN);
	}

	/**
	 * @return the centerMarker
	 */
	public GenericObject getCenterMarker()
	{
		return centerMarker;
	}

	/**
	 * @param centerMarker
	 *           the centerMarker to set
	 */
	public void setCenterMarker(GenericObject centerMarker)
	{
		this.centerMarker = centerMarker;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.ui.eclipse.osm.GeoLocationCacheListener#geoLocationCacheChanged
	 * (org.netxms.client.objects.GenericObject, org.netxms.client.GeoLocation)
	 */
	@Override
	public void geoLocationCacheChanged(final GenericObject object, final GeoLocation prevLocation)
	{
		getDisplay().asyncExec(new Runnable()
		{
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
	private void onCacheChange(final GenericObject object, final GeoLocation prevLocation)
	{
		GeoLocation currLocation = object.getGeolocation();
		if (((currLocation.getType() != GeoLocation.UNSET) && coverage.contains(currLocation.getLatitude(),
				currLocation.getLongitude()))
				|| ((prevLocation != null) && (prevLocation.getType() != GeoLocation.UNSET) && coverage.contains(
						prevLocation.getLatitude(), prevLocation.getLongitude())))
		{
			objects = GeoLocationCache.getInstance().getObjectsInArea(coverage);
			redraw();
		}
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.MouseWheelListener#mouseScrolled(org.eclipse.swt.events.MouseEvent)
	 */
	@Override
	public void mouseScrolled(MouseEvent event)
	{
		if (loading)
			return;
		
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
			notifyZoomListeners();
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
		if ((e.button == 1) && !loading) // left button, ignore if map is currently loading
		{
			dragStartPoint = new Point(e.x, e.y);
			setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.MouseListener#mouseUp(org.eclipse.swt.events.MouseEvent)
	 */
	@Override
	public void mouseUp(MouseEvent e)
	{
		if ((e.button == 1) && (dragStartPoint != null))
		{
			final Point centerXY = GeoLocationCache.coordinateToDisplay(accessor.getCenterPoint(), accessor.getZoom());
			centerXY.x += offsetX;
			centerXY.y += offsetY;
			final GeoLocation geoLocation = GeoLocationCache.displayToCoordinates(centerXY, accessor.getZoom());
			accessor.setLatitude(geoLocation.getLatitude());
			accessor.setLongitude(geoLocation.getLongitude());
			reloadMap();
			offsetX = 0;
			offsetY = 0;
			dragStartPoint = null;
			setCursor(null);
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
	}
}
