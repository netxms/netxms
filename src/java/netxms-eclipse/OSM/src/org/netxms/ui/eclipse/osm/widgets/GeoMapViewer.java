/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackAdapter;
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
import org.eclipse.swt.widgets.ToolTip;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.osm.Activator;
import org.netxms.ui.eclipse.osm.GeoLocationCache;
import org.netxms.ui.eclipse.osm.GeoLocationCacheListener;
import org.netxms.ui.eclipse.osm.Messages;
import org.netxms.ui.eclipse.osm.tools.Area;
import org.netxms.ui.eclipse.osm.tools.MapAccessor;
import org.netxms.ui.eclipse.osm.tools.MapLoader;
import org.netxms.ui.eclipse.osm.tools.QuadTree;
import org.netxms.ui.eclipse.osm.tools.Tile;
import org.netxms.ui.eclipse.osm.tools.TileSet;
import org.netxms.ui.eclipse.osm.widgets.helpers.GeoMapListener;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * This widget shows map retrieved via OpenStreetMap Static Map API
 */
public class GeoMapViewer extends Canvas implements PaintListener, GeoLocationCacheListener, MouseWheelListener, MouseListener, MouseMoveListener
{
   private static final int START = 1;
   private static final int END = 2;
   private static final String pointInformation[] = {"Start","End"};
   
   
	private static final Color MAP_BACKGROUND = new Color(Display.getCurrent(), 255, 255, 255);
	private static final Color INFO_BLOCK_BACKGROUND = new Color(Display.getCurrent(), 150, 240, 88);
	private static final Color INFO_BLOCK_BORDER = new Color(Display.getCurrent(), 0, 0, 0);
	private static final Color INFO_BLOCK_TEXT = new Color(Display.getCurrent(), 0, 0, 0);
	private static final Color LABEL_BACKGROUND = new Color(Display.getCurrent(), 240, 254, 192);
	private static final Color LABEL_TEXT = new Color(Display.getCurrent(), 0, 0, 0);
	private static final Color BORDER_COLOR = new Color(Display.getCurrent(), 128, 128, 128);
	private static final Color SELECTION_COLOR = new Color(Display.getCurrent(), 0, 0, 255);
	
	private static final Font TITLE_FONT = new Font(Display.getCurrent(), "Verdana", 10, SWT.BOLD);  //$NON-NLS-1$

	private static final int LABEL_ARROW_HEIGHT = 20;
	private static final int LABEL_ARROW_OFFSET = 10;
	private static final int LABEL_X_MARGIN = 4;
	private static final int LABEL_Y_MARGIN = 4;
	private static final int LABEL_SPACING = 4;

	private static final int DRAG_JITTER = 8;

	private ILabelProvider labelProvider;
	private Image currentImage = null;
	private Image bufferImage = null;
	private Area coverage = null;
	private List<AbstractObject> objects = new ArrayList<AbstractObject>();
	private MapAccessor accessor;
	private MapLoader mapLoader;
	private IViewPart viewPart = null;
	private Point currentPoint;
	private Point dragStartPoint = null;
	private Point selectionStartPoint = null;
	private Point selectionEndPoint = null;
	private Set<GeoMapListener> mapListeners = new HashSet<GeoMapListener>(0);
	private String title = null;
	private int offsetX;
	private int offsetY;
	private TileSet currentTileSet = null;
	//Historical variables
	private boolean historycalData;
   private List<GeoLocation> history = new ArrayList<GeoLocation>();
   private QuadTree<GeoLocation> locationTree = new QuadTree<GeoLocation>();
   private AbstractObject historyObject = null;
   private Date till = new Date();
   private int timeFrame = 60*60*1000;
   private boolean strictTime = false;
   private int highlightobjectID = -1;
   private ToolTip toolTip;
	
	/**
	 * @param parent
	 * @param style
	 */
	public GeoMapViewer(Composite parent, int style, final boolean historycalData, AbstractObject historyObject)
	{
		super(parent, style | SWT.NO_BACKGROUND | SWT.DOUBLE_BUFFERED);
		this.historycalData = historycalData;
		if(historycalData)
		{
		   this.historyObject = historyObject;		   
		}

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
				GeoLocationCache.getInstance().removeListener(GeoMapViewer.this);
				if (bufferImage != null)
					bufferImage.dispose();
				if (currentImage != null)
					currentImage.dispose();
				mapLoader.dispose();
			}
		});

		addMouseListener(this);
		addMouseMoveListener(this);
		addMouseWheelListener(this);

		GeoLocationCache.getInstance().addListener(this);
		addMouseTrackListener(new MouseTrackAdapter() {
         @Override
         public void mouseHover(MouseEvent e)
         {
            highlightobjectID = -1;
            toolTip.setVisible(false);
            if (!historycalData) //$NON-NLS-1$
               return;           
            
            Point p = new Point(e.x, e.y);
            p.x -= 5;
            p.y -= 5;
            GeoLocation loc1 = getLocationAtPoint(p);
            p.x += 10;
            p.y += 10;
            GeoLocation loc2 = getLocationAtPoint(p);
            Area area = new Area(loc1.getLatitude(), loc1.getLongitude(), loc2.getLatitude(), loc2.getLongitude());
            List<GeoLocation> suitablePoints = locationTree.query(area);
            if(suitablePoints.size() == 0)
               return;
            
            int i = 0;
            if(suitablePoints.size() > 1)
            {
               double minDistance = 100;
               for(int j = 0; j < suitablePoints.size(); j++)
               {
                  double newDistance =  Math.pow( Math.pow(suitablePoints.get(j).getLatitude() - loc1.getLatitude(), 2) + Math.pow(suitablePoints.get(j).getLongitude() - loc1.getLongitude(), 2), 0.5);  
                  if(minDistance > newDistance)
                  {
                     minDistance = newDistance;
                     i = j;
                  }
               }
            }
            
            highlightobjectID = history.indexOf(suitablePoints.get(i)); 
            redraw();
         }

         /* (non-Javadoc)
          * @see org.eclipse.swt.events.MouseTrackAdapter#mouseExit(org.eclipse.swt.events.MouseEvent)
          */
         @Override
         public void mouseExit(MouseEvent e)
         {
            highlightobjectID = -1;
            toolTip.setVisible(false);
            redraw();
         }
      });
		
		addMouseMoveListener(new MouseMoveListener() {
         @Override
         public void mouseMove(MouseEvent e)
         {
            if (highlightobjectID != -1)
            {
               highlightobjectID = -1;
               toolTip.setVisible(false);
               redraw();
            }
         }
      });
		
      toolTip = new ToolTip(getShell(), SWT.BALLOON);	
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
						if(!historycalData)
						{
   						objects = GeoLocationCache.getInstance().getObjectsInArea(coverage);
   						GeoMapViewer.this.redraw();
						}
						else
						{
						   GeoMapViewer.this.updateHistory();
						}
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
						if (!GeoMapViewer.this.isDisposed() && (currentTileSet == tiles))
						{
							drawTiles(tiles);
							GeoMapViewer.this.redraw();
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
		
		gc.fillRectangle(bufferImage.getBounds());

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
		// and map is not currently loading
		if (dragStartPoint == null)
		{
			gc.setAntialias(SWT.ON);
			gc.setTextAntialias(SWT.ON);

			final Point centerXY = GeoLocationCache.coordinateToDisplay(accessor.getCenterPoint(), accessor.getZoom());
         if(!historycalData)
         {
   			for(AbstractObject object : objects)
   			{
   				final Point virtualXY = GeoLocationCache.coordinateToDisplay(object.getGeolocation(), accessor.getZoom());
   				final int dx = virtualXY.x - centerXY.x;
   				final int dy = virtualXY.y - centerXY.y;
   				drawObject(gc, imgW / 2 + dx, imgH / 2 + dy, object);
   			}
         }
         else
         {  
            int nextX = 0;
            int nextY = 0;
            for(int i = 0; i < history.size(); i++)
            {
               final Point virtualXY = GeoLocationCache.coordinateToDisplay(history.get(i), accessor.getZoom());
               final int dx = virtualXY.x - centerXY.x;
               final int dy = virtualXY.y - centerXY.y;
               
               if (i != history.size() - 1)
               { 
                  final Point virtualXY2 = GeoLocationCache.coordinateToDisplay(history.get(i+1), accessor.getZoom());
                  nextX = imgW / 2 + (virtualXY2.x - centerXY.x);
                  nextY = imgH / 2 + (virtualXY2.y - centerXY.y);
               }
               
               int color = SWT.COLOR_RED;
               if(i ==  highlightobjectID)
               {
                  color = SWT.COLOR_GREEN;
                  toolTip.setText("Start time: " + history.get(i).getTimestamp() + "\nEnd Time: " + history.get(i).getEndTimestamp() + "\nLocation: " + history.get(i));
                  toolTip.setVisible(true);
               }
                  
               if(i==0)
               {
                  if(i == history.size() - 1)
                  {
                     nextX = imgW / 2 + dx;
                     nextY = imgH / 2 + dy;
                  }
                  drawObject(gc, imgW / 2 + dx, imgH / 2 + dy, GeoMapViewer.START, nextX, nextY, color);                  
                  continue;
               } 
               
               if (i == history.size() - 1)
               {    
                  drawObject(gc, imgW / 2 + dx, imgH / 2 + dy, GeoMapViewer.END, nextX, nextY, color);
                  continue;
               }
               
               drawObject(gc, imgW / 2 + dx, imgH / 2 + dy, 0, nextX, nextY, color);
            }
         }
	
			final GeoLocation gl = new GeoLocation(accessor.getLatitude(), accessor.getLongitude());
			final String text = gl.toString();
			final Point textSize = gc.textExtent(text);
	
			Rectangle rect = getClientArea();
			rect.x = 10;
			rect.x = rect.width - textSize.x - 20;
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
		
		// Draw title
		if ((title != null) && !title.isEmpty())
		{
			gc.setFont(TITLE_FONT);
			Rectangle rect = getClientArea();
			int x = (rect.width - gc.textExtent(title).x) / 2;
			gc.setForeground(SharedColors.getColor(SharedColors.GEOMAP_TITLE, getDisplay()));
			gc.drawText(title, x, 10, true);
		}
		
		gc.dispose();
		e.gc.drawImage(bufferImage, 0, 0);
	}

	/**
	 * Draw object on map
	 * 
	 * @param gc
	 * @param x
	 * @param y
	 * @param object
	 */
	private void drawObject(GC gc, int x, int y, AbstractObject object) 
	{
		final String text = object.getObjectName();
		final Point textSize = gc.textExtent(text);

		Image image = labelProvider.getImage(object);
		if (image == null)
			image = SharedIcons.IMG_UNKNOWN_OBJECT;
		
		Rectangle rect = new Rectangle(x - LABEL_ARROW_OFFSET, y - LABEL_ARROW_HEIGHT - textSize.y, textSize.x
				+ image.getImageData().width + LABEL_X_MARGIN * 2 + LABEL_SPACING, Math.max(image.getImageData().height, textSize.y)
				+ LABEL_Y_MARGIN * 2);
		
		gc.setBackground(LABEL_BACKGROUND);

		gc.setForeground(BORDER_COLOR);
		gc.setLineWidth(4);
		gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
		gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
		
		gc.setForeground(StatusDisplayInfo.getStatusColor(object.getStatus()));
		gc.setLineWidth(2);
		gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
		gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
		
		final int[] arrow = new int[] { rect.x + LABEL_ARROW_OFFSET - 4, rect.y + rect.height, x, y, rect.x + LABEL_ARROW_OFFSET + 4,
				rect.y + rect.height };

		gc.setLineWidth(4);
		gc.setForeground(BORDER_COLOR);
		gc.drawPolyline(arrow);

		gc.fillPolygon(arrow);
		gc.setForeground(LABEL_BACKGROUND);
		gc.setLineWidth(2);
		gc.drawLine(arrow[0], arrow[1], arrow[4], arrow[5]);
		gc.setForeground(StatusDisplayInfo.getStatusColor(object.getStatus()));
		gc.drawPolyline(arrow);

		gc.setForeground(LABEL_TEXT);
		gc.drawImage(image, rect.x + LABEL_X_MARGIN, rect.y + LABEL_Y_MARGIN);
		gc.drawText(text, rect.x + LABEL_X_MARGIN + image.getImageData().width + LABEL_SPACING, rect.y + LABEL_Y_MARGIN);
	}

	  /**
    * Draw object on map
    * 
    * @param gc
    * @param x
    * @param y
    * @param object
    */
   private void drawObject(GC gc, int x, int y, int flag, int prevX, int prevY, int color) 
   {    
      if(flag == GeoMapViewer.START || flag == GeoMapViewer.END)
      {
         if(flag == GeoMapViewer.START)
         {
            gc.drawLine(x, y, prevX, prevY);
         }
         gc.setBackground(Display.getCurrent().getSystemColor(color)); 
         gc.fillOval(x - 5, y -5, 10, 10);
         
         final String text = pointInformation[flag -1];
         final Point textSize = gc.textExtent(text);
         
         Rectangle rect = new Rectangle(x - LABEL_ARROW_OFFSET, y - LABEL_ARROW_HEIGHT - textSize.y, textSize.x
               + LABEL_X_MARGIN * 2 + LABEL_SPACING, textSize.y + LABEL_Y_MARGIN * 2);
         
         gc.setBackground(LABEL_BACKGROUND);

         gc.setForeground(BORDER_COLOR);
         gc.setLineWidth(4);
         gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
         gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
         
         gc.setLineWidth(2);
         gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
         gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
         final int[] arrow = new int[] { rect.x + LABEL_ARROW_OFFSET - 4, rect.y + rect.height, x, y, rect.x + LABEL_ARROW_OFFSET + 4,
               rect.y + rect.height };

         gc.setLineWidth(4);
         gc.setForeground(BORDER_COLOR);
         gc.drawPolyline(arrow);

         gc.fillPolygon(arrow);
         gc.setForeground(LABEL_BACKGROUND);
         gc.setLineWidth(2);
         gc.drawLine(arrow[0], arrow[1], arrow[4], arrow[5]);
         gc.drawPolyline(arrow);

         gc.setForeground(LABEL_TEXT);
         gc.drawText(text, rect.x + LABEL_X_MARGIN + LABEL_SPACING, rect.y + LABEL_Y_MARGIN);
      }
      else 
      {
         gc.drawLine(x, y, prevX, prevY);
         gc.setBackground(Display.getCurrent().getSystemColor(color)); 
         gc.fillOval(x - 5, y -5, 10, 10);
      }      
   }	
	
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
		getDisplay().asyncExec(new Runnable()
		{
			@Override
			public void run()
			{
			   if(!historycalData)
			   {
			      onCacheChange(object, prevLocation);
			   }
			   else
			   {
			      if(object.getObjectId() == historyObject.getObjectId())
			         onCacheChange(object, prevLocation);
			   }
			}
		});
	}

	/**
	 * Real handler for geolocation cache changes. Must be run in UI thread.
	 * 
	 * @param object
	 * @param prevLocation
	 */
	private void onCacheChange(final AbstractObject object, final GeoLocation prevLocation)
	{
		GeoLocation currLocation = object.getGeolocation();
		if (((currLocation.getType() != GeoLocation.UNSET) && coverage.contains(currLocation.getLatitude(),
				currLocation.getLongitude()))
				|| ((prevLocation != null) && (prevLocation.getType() != GeoLocation.UNSET) && coverage.contains(
						prevLocation.getLatitude(), prevLocation.getLongitude())))
		{
		   if(!historycalData)
		   {
   			objects = GeoLocationCache.getInstance().getObjectsInArea(coverage);
   			redraw();
		   }
		   else
		   {
		      updateHistory();
		   }
		}
	}
	
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
			if ((e.stateMask & SWT.SHIFT) != 0)
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
		return currentPoint;
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
	 * Updates points for historical view
	 */
	private void updateHistory()
	{
	   ConsoleJob job = new ConsoleJob(Messages.get().GeoMapViewer_DownloadJob_Title, viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            NXCSession session = (NXCSession)ConsoleSharedData.getSession();
            if(!strictTime)
            {
               till = new Date();
            }
            history = session.getLocationHistory(historyObject.getObjectId(), new Date(till.getTime() - timeFrame), till);
            for(int i = 0; i < history.size(); i++)
               locationTree.insert(history.get(i).getLatitude(), history.get(i).getLongitude(), history.get(i));
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  GeoMapViewer.this.redraw();
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
	 * Sets new view period in minutes
	 */
	public void changeTimePeriod(int timePeriod)
	{
	   strictTime = false;
	   timeFrame = timePeriod * 60 * 1000;
	   updateHistory();
	}
	
	/**
    * Set time period start
    */
   public void changeTimePeriod(Date startTime)
   {
      strictTime = true;
      till = startTime;
      updateHistory();
   }
}
