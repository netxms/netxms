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
package org.netxms.nxmc.modules.worldmap.tools;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
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
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.worldmap.GeoLocationCache;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Map Loader - loads geographic map from tile server. Uses cached tiles when possible.
 */
public class MapLoader
{
   private static final Logger logger = LoggerFactory.getLogger(MapLoader.class);

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
      missingTile = ResourceManager.getImage(display, "icons/worldmap/missing_tile.png");
      loadingTile = ResourceManager.getImage(display, "icons/worldmap/loading_tile.png");
      borderTile = ResourceManager.getImage(display, "icons/worldmap/border_tile.png");
      session = Registry.getSession();
      workers = Executors.newFixedThreadPool(16, new ThreadFactory() {
         private int threadNumber = 1;

         @Override
         public Thread newThread(Runnable r)
         {
            Thread t = new Thread(r, "MapLoader-" + Integer.toString(threadNumber++));
            t.setDaemon(true);
            return t;
         }
      });
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
         logger.warn("Invalid tile server URL", e);
			return null;
		}

		HttpURLConnection conn = null;
		InputStream in = null;
		Image image = null;
		try
      {
		   conn = (HttpURLConnection)url.openConnection();
		   conn.setRequestProperty("User-Agent", "nxmc/" + VersionInfo.version());
         conn.setAllowUserInteraction(false);
         in = new BufferedInputStream(conn.getInputStream());
         image = WidgetHelper.createImageFromStream(display, in);
      }
      catch(IOException e)
      {
         logger.warn(url.toString() + ": " + e.getMessage());
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
   private File buildCacheFileName(int zoom, int x, int y)
	{
      StringBuilder sb = new StringBuilder("MapTiles"); //$NON-NLS-1$
		sb.append(File.separatorChar);
		sb.append(zoom);
		sb.append(File.separatorChar);
		sb.append(x);
		sb.append(File.separatorChar);
		sb.append(y);
		sb.append(".png"); //$NON-NLS-1$
      return new File(Registry.getStateDir(display), sb.toString());
	}

	/**
	 * @param zoom
	 * @param x
	 * @param y
	 * @return
	 */
	private Image loadTileFromCache(int zoom, int x, int y)
	{
		try
		{
			final File imageFile = buildCacheFileName(zoom, x, y);
			ImageData[] imageData = null;
         synchronized(CACHE_MUTEX)
         {
            if (!imageFile.canRead())
               return null;
            ImageLoader imageLoader = new ImageLoader();
            imageData = imageLoader.load(imageFile.getAbsolutePath());
         }
         return WidgetHelper.createImageFromImageData(display, imageData[0]);
		}
		catch(Exception e)
		{
         logger.error("Exception in loadTileFromCache", e);
			return null;
		}
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
         return new Tile(x, y, borderTile, true, true);

      if (x < 0)
         x = (maxTileNum + 1) - (-x) % (maxTileNum + 1);
      else if (x > maxTileNum)
         x = x % (maxTileNum + 1);

		Image tileImage = loadTileFromCache(zoom, x, y);
		if (tileImage == null)
		{
			if (cachedOnly)
            return new Tile(x, y, loadingTile, false, true);
			tileImage = loadTile(zoom, x, y);
		}
      return (tileImage != null) ? new Tile(x, y, tileImage, true, false) : new Tile(x, y, missingTile, true, true);
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
                              logger.info("Tile set loading cancelled");
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
