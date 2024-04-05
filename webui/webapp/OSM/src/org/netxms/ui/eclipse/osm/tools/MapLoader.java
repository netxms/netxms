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
package org.netxms.ui.eclipse.osm.tools;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import org.eclipse.core.runtime.Platform;
import org.eclipse.osgi.service.datalocation.Location;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.netxms.base.GeoLocation;
import org.netxms.base.VersionInfo;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.osm.Activator;
import org.netxms.ui.eclipse.osm.GeoLocationCache;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Map Loader - loads geographic map from tile server. Uses cached tiles when possible.
 */
public class MapLoader
{
	public static final int CENTER = GeoLocationCache.CENTER;
	public static final int TOP_LEFT = GeoLocationCache.TOP_LEFT;
	public static final int BOTTOM_RIGHT = GeoLocationCache.BOTTOM_RIGHT;

	private static Object CACHE_MUTEX = new Object();

	private Display display;
	private NXCSession session;
	private ExecutorService workers;
	private Image missingTile = null; 
	private Image loadingTile = null; 
	private Image borderTile = null;

	/**
	 * Create map loader
	 * 
	 * @param display
	 */
	public MapLoader(Display display)
	{
		this.display = display;
		session = ConsoleSharedData.getSession();
		workers = Executors.newFixedThreadPool(16);
	}

	/**
	 * @param location
	 * @param zoom
	 * @return
	 */
	private static Point tileFromLocation(double lat, double lon, int zoom)
	{		
      int x = (int)Math.floor((lon + 180) / 360 * (1 << zoom));
      int y = (int)Math.floor((1 - Math.log(Math.tan(Math.toRadians(lat)) + 1 / Math.cos(Math.toRadians(lat))) / Math.PI) / 2 * (1 << zoom));
		return new Point(x, y);
	}

	/**
	 * @param x
	 * @param z
	 * @return
	 */
	private static double longitudeFromTile(int x, int z)
	{
		return x / Math.pow(2.0, z) * 360.0 - 180;
	}
	 
	/**
	 * @param y
	 * @param z
	 * @return
	 */
	private static double latitudeFromTile(int y, int z) 
	{
		double n = Math.PI - (2.0 * Math.PI * y) / Math.pow(2.0, z);
		return Math.toDegrees(Math.atan(Math.sinh(n)));
	}
	
	/**
	 * get image for missing tile
	 * 
	 * @return
	 */
	private Image getMissingTileImage()
	{
		if (missingTile == null)
		{
			display.syncExec(new Runnable() {
				@Override
				public void run()
				{
					missingTile = Activator.getImageDescriptor("icons/missing_tile.png").createImage(display);
				}
			});
		}
		return missingTile;
	}
	
	/**
	 * get image for tile being loaded
	 * 
	 * @return
	 */
	private Image getLoadingTileImage()
	{
		if (loadingTile == null)
		{
			display.syncExec(new Runnable() {
				@Override
				public void run()
				{
					loadingTile = Activator.getImageDescriptor("icons/loading_tile.png").createImage(display);
				}
			});
		}
		return loadingTile;
	}
	
	/**
	 * get image for border tile (tile out of map boundaries)
	 * 
	 * @return
	 */
	private Image getBorderTileImage()
	{
		if (borderTile == null)
		{
			display.syncExec(new Runnable() {
				@Override
				public void run()
				{
					borderTile = Activator.getImageDescriptor("icons/border_tile.png").createImage(display);
				}
			});
		}
		return borderTile;
	}
	
   /**
    * Convert x/y/zoom coordinates to Bing QUadKey
    *
    * @param x x coordinate
    * @param y y coordinate
    * @param zoom zoom level
    * @return quad key
    */
   private static String toQuadKey(int x, int y, int zoom)
   {
      char k[] = new char[zoom];
      for(int i = zoom - 1, j = 0; i >= 0; i--, j++)
      {
         char b = '0';
         int mask = 1 << i;
         if ((x & mask) != 0)
            b++;
         if ((y & mask) != 0)
            b += 2;
         k[j] = b;
      }
      return new String(k);
   }

	/**
	 * Load tile from tile server. Expected to be executed on background thread.
	 * 
	 * @param zoom
	 * @param x
	 * @param y
	 * @return
	 */
	private Image loadTile(int zoom, int x, int y)
	{
		final String tileServerURL = session.getTileServerURL();
		URL url = null;
		try
		{
         if (tileServerURL.contains("{"))
         {
            // URL template with placeholders
            url = new URL(tileServerURL
                  .replace("{x}", Integer.toString(x))
                  .replace("{y}", Integer.toString(y))
                  .replace("{-y}", Integer.toString(-y))
                  .replace("{z}", Integer.toString(zoom))
                  .replace("{q}", toQuadKey(x, y, zoom))
               );
         }
         else
         {
            url = new URL(tileServerURL + zoom + "/" + x + "/" + y + ".png");
         }
		}
		catch(MalformedURLException e)
		{
         Activator.log("Invalid tile server URL", e);
			return null;
		}

		Image image = null;
      HttpURLConnection conn = null;
      InputStream in = null;
      try
      {
         conn = (HttpURLConnection)url.openConnection();
         conn.setRequestProperty("User-Agent", "nxmc-webui/" + VersionInfo.version());
         in = new BufferedInputStream(conn.getInputStream());
         final ImageData imageData = new ImageData(in);

         final Image[] imageContainer = new Image[1];
         display.syncExec(new Runnable() {
            @Override
            public void run()
            {
               try
               {
                  imageContainer[0] = new Image(display, imageData);
               }
               catch(Exception e)
               {
                  Activator.log("Image creation failed", e);
                  imageContainer[0] = null;
               }
            }
         });
         image = imageContainer[0];
      }
      catch(IOException e)
      {
         Activator.log(url.toString() + ": " + e.getMessage());
      }
      finally
      {
         try
         {
            if (in != null)
               in.close();
            if (conn != null)
               conn.disconnect();
         }
         catch(IOException e)
         {
         }
      }

		if (image != null)
		{
			// save to cache
         File imageFile = buildCacheFileName(zoom, x, y);
			synchronized(CACHE_MUTEX)
			{
				imageFile.getParentFile().mkdirs();
			}
         ImageLoader imageLoader = new ImageLoader();
         imageLoader.data = new ImageData[] { image.getImageData() };
         synchronized(CACHE_MUTEX)
         {
            imageLoader.save(imageFile.getAbsolutePath(), SWT.IMAGE_PNG);
         }
		}

		return image;
	}

	/**
	 * @param zoom
	 * @param x
	 * @param y
	 * @return
	 */
	private static File buildCacheFileName(int zoom, int x, int y)
	{
		Location loc = Platform.getInstanceLocation();
		File targetDir;
		try
		{
			targetDir = new File(loc.getURL().toURI());
		}
		catch(URISyntaxException e)
		{
			targetDir = new File(loc.getURL().getPath());
		}
		
		StringBuilder sb = new StringBuilder("OSM"); //$NON-NLS-1$
		sb.append(File.separatorChar);
		sb.append(zoom);
		sb.append(File.separatorChar);
		sb.append(x);
		sb.append(File.separatorChar);
		sb.append(y);
		sb.append(".png"); //$NON-NLS-1$

		return new File(targetDir, sb.toString());
	}

	/**
	 * @param zoom
	 * @param x
	 * @param y
	 * @return
	 */
	private Image loadTileFromCache(int zoom, int x, int y)
	{
		final File imageFile = buildCacheFileName(zoom, x, y);
		synchronized(CACHE_MUTEX)
		{
			if (imageFile.canRead())
			{
			   final Image[] image = new Image[1];
			   display.syncExec(new Runnable() {
               @Override
               public void run()
               {
                  try
                  {
                     image[0] = new Image(display, imageFile.getAbsolutePath());
                  }
                  catch(Exception e)
                  {
                     Activator.log("Image creation failed", e);
                     image[0] = null;
                  }
               }
            });
				return image[0];
			}
		}
		return null;
	}

	/**
	 * @param zoom
	 * @param x
	 * @param y
	 * @return
	 */
	public Tile getTile(int zoom, int x, int y, boolean cachedOnly)
	{
		// check x and y for validity
		int maxTileNum = (1 << zoom) - 1;
      if ((y < 0) || (y > maxTileNum))
         return new Tile(x, y, getBorderTileImage(), true, true);

      if (x < 0)
         x = (maxTileNum + 1) - (-x) % (maxTileNum + 1);
      else if (x > maxTileNum)
         x = x % (maxTileNum + 1);

		Image tileImage = loadTileFromCache(zoom, x, y);
		if (tileImage == null)
		{
			if (cachedOnly)
				return new Tile(x, y, getLoadingTileImage(), false, true);
			tileImage = loadTile(zoom, x, y);
		}
		return (tileImage != null) ? new Tile(x, y, tileImage, true, false) : new Tile(x, y, getMissingTileImage(), true, true);
	}

	/**
	 * @param mapSize
	 * @param centerPoint
	 * @param zoom
	 * @return
	 */
	public static Rectangle calculateBoundingBox(Point mapSize, GeoLocation centerPoint, int zoom)
	{
		Area coverage = GeoLocationCache.calculateCoverage(mapSize, centerPoint, CENTER, zoom);
		Point topLeft = tileFromLocation(coverage.getxLow(), coverage.getyLow(), zoom);
		Point bottomRight = tileFromLocation(coverage.getxHigh(), coverage.getyHigh(), zoom);
		return new Rectangle(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
	}
	
	/**
	 * @param mapSize
	 * @param basePoint
	 * @param zoom
	 * @param cachedOnly if true, only locally cached tiles will be returned (missing tiles will be replaced by placeholder image)
	 * @return
	 */
	public TileSet getAllTiles(Point mapSize, GeoLocation basePoint, int pointLocation, int zoom, boolean cachedOnly)
	{
		if ((mapSize.x < 32) || (mapSize.y < 32) || (basePoint == null))
			return null;

		Area coverage = GeoLocationCache.calculateCoverage(mapSize, basePoint, pointLocation, zoom);
		Point bottomLeft = tileFromLocation(coverage.getxLow(), coverage.getyLow(), zoom);
		Point topRight = tileFromLocation(coverage.getxHigh(), coverage.getyHigh(), zoom);

		Tile[][] tiles = new Tile[bottomLeft.y - topRight.y + 1][topRight.x - bottomLeft.x + 1];

		int x = bottomLeft.x;
		int y = topRight.y;
		int l = (bottomLeft.y - topRight.y + 1) * (topRight.x - bottomLeft.x + 1);
		for(int i = 0; i < l; i++)
		{
			tiles[y - topRight.y][x - bottomLeft.x] = getTile(zoom, x, y, cachedOnly);
			x++;
			if (x > topRight.x)
			{
				x = bottomLeft.x;
				y++;
			}
		}

		double lat = latitudeFromTile(topRight.y, zoom);
		double lon = longitudeFromTile(bottomLeft.x, zoom);
		Point realTopLeft = GeoLocationCache.coordinateToDisplay(new GeoLocation(lat, lon), zoom);
		Point reqTopLeft = GeoLocationCache.coordinateToDisplay(new GeoLocation(coverage.getxHigh(), coverage.getyLow()), zoom);

		return new TileSet(tiles, realTopLeft.x - reqTopLeft.x, realTopLeft.y - reqTopLeft.y, zoom);
	}

	/**
	 * Load missing tiles in tile set
	 * 
	 * @param tiles
	 */
	public void loadMissingTiles(final TileSet tiles, Runnable progressHandler)
	{
	   synchronized(tiles)
	   {
   	   tiles.lastProgressUpdate = System.currentTimeMillis();
   		for(int i = 0; i < tiles.tiles.length; i++)
   		{
   			for(int j = 0; j < tiles.tiles[i].length; j++)
   			{
   				final Tile tile = tiles.tiles[i][j];
   				if (!tile.isLoaded())
   				{
   				   tiles.workers++;
   				   final int row = i;
                  final int col = j;
   				   workers.execute(new Runnable() {
                     @Override
                     public void run()
                     {
                        synchronized(tiles)
                        {
                           if (tiles.cancelled)
                           {
                              Activator.log("Tile set loading cancelled");
                              tiles.workers--;
                              return;
                           }
                        }
                        Tile loadedTile = getTile(tiles.zoom, tile.getX(), tile.getY(), false);
                        if (loadedTile != null)
                        {
                           synchronized(tiles)
                           {
                              tiles.tiles[row][col] = loadedTile;
                              tiles.missingTiles--;
                              if (tiles.missingTiles > 0)
                              {
                                 long now = System.currentTimeMillis();
                                 if (now - tiles.lastProgressUpdate >= 1000)
                                 {
                                    if (!display.isDisposed())
                                       display.asyncExec(progressHandler);
                                    else
                                       tiles.cancelled = true; // Stop loading
                                    tiles.lastProgressUpdate = now;
                                 }
                              }
                              tiles.workers--;
                              if (tiles.workers == 0)
                              {
                                 tiles.notifyAll();
                              }
                           }
                        }
                     }
                  });
   				}
   			}
   		}
   		while(tiles.workers > 0)
   		{
            try
            {
               tiles.wait();
            }
            catch(InterruptedException e)
            {
            }
   		}
	   }

      if (!display.isDisposed())
         display.asyncExec(progressHandler);
	}

	/**
	 * Returns true if given image is internally generated (not downloaded)
	 * 
	 * @param image
	 * @return
	 */
	protected boolean isInternalImage(Image image)
	{
		return (image == missingTile) || (image == borderTile) || (image == loadingTile);
	}

	/**
	 * Dispose map loader
	 */
	public void dispose()
	{
      workers.shutdown();
		if (loadingTile != null)
			loadingTile.dispose();
		if (missingTile != null)
			missingTile.dispose();
		if (borderTile != null)
			borderTile.dispose();
	}
}
