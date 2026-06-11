/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.imagelibrary;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.LibraryImage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.ui.svg.SVGImage;
import org.netxms.ui.svg.SVGParseException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Image provider for image library. Supports raster and SVG images with three access modes:
 * <ul>
 * <li><b>Native</b> - returns LibraryImage with binary data, consumer decides rendering</li>
 * <li><b>Rasterized</b> - returns SWT Image at requested dimensions</li>
 * <li><b>Render to GC</b> - renders directly to a graphics context at specified position and size</li>
 * </ul>
 */
public class ImageProvider
{
   private static final Logger logger = LoggerFactory.getLogger(ImageProvider.class);

   private static final int DEFAULT_SVG_SIZE = 256;
   private static final int MAX_SVG_RASTER_SIZE = 4096;
   private static final int RASTER_CACHE_MAX_SIZE = 256;

   /**
    * Listener for changes in image library
    */
   private static final class ImageLibraryListener implements SessionListener
   {
      private ImageProvider imageProvider;

      private ImageLibraryListener(ImageProvider imageProvider)
      {
         this.imageProvider = imageProvider;
      }

      @Override
      public void notificationHandler(SessionNotification n)
      {
         if (n.getCode() == SessionNotification.IMAGE_LIBRARY_CHANGED)
         {
            final UUID guid = (UUID)n.getObject();
            if (n.getSubCode() == SessionNotification.IMAGE_DELETED)
               imageProvider.deleteImage(guid);
            else
               imageProvider.updateImage(guid);
         }
      }
   }

   /**
    * Create instance of image provider for given display and session. Will synchronize image metadata.
    *
    * @param display owning display
    * @param session communication session
    */
   public static ImageProvider createInstance(Display display, NXCSession session)
   {
      ImageProvider instance = new ImageProvider(display, session);
      Registry.setSingleton(display, ImageProvider.class, instance);
      try
      {
         instance.syncMetaData();
         session.addListener(new ImageLibraryListener(instance));
      }
      catch(Exception e)
      {
         logger.error("Error synchronizing image library metadata", e);
      }
      return instance;
   }

   /**
    * Get singleton instance.
    *
    * @return image provider instance
    */
   public static ImageProvider getInstance()
   {
      return Registry.getSingleton(ImageProvider.class);
   }

   /**
    * Get singleton instance.
    *
    * @param display display to get instance for
    * @return image provider instance
    */
   public static ImageProvider getInstance(Display display)
   {
      return Registry.getSingleton(ImageProvider.class, display);
   }

   private final I18n i18n = LocalizationHelper.getI18n(ImageProvider.class);
   private final NXCSession session;
   private final Display display;
   private final Image missingImage;

   /** Image metadata index (no binary data) */
   private final Map<UUID, LibraryImage> libraryIndex = Collections.synchronizedMap(new HashMap<UUID, LibraryImage>());

   /** Complete library images with binary data */
   private final Map<UUID, LibraryImage> imageDataCache = Collections.synchronizedMap(new HashMap<UUID, LibraryImage>());

   /** Parsed SVG images (for SVG entries only) */
   private final Map<UUID, SVGImage> svgCache = Collections.synchronizedMap(new HashMap<UUID, SVGImage>());

   /** Rasterized images keyed by "guid_WxH", LRU eviction */
   private final Map<String, Image> rasterCache = Collections.synchronizedMap(new LinkedHashMap<String, Image>(64, 0.75f, true) {
      private static final long serialVersionUID = 1L;

      @Override
      protected boolean removeEldestEntry(Map.Entry<String, Image> eldest)
      {
         if (size() > RASTER_CACHE_MAX_SIZE)
         {
            Image image = eldest.getValue();
            if ((image != null) && !image.isDisposed())
               image.dispose();
            return true;
         }
         return false;
      }
   });

   /** UUIDs currently being loaded from server (to avoid duplicate requests) */
   private final Set<UUID> pendingLoads = Collections.synchronizedSet(new HashSet<UUID>());

   private final Set<ImageUpdateListener> updateListeners = new HashSet<ImageUpdateListener>();

   /**
    * Create image provider.
    *
    * @param display owning display
    * @param session communication session
    */
   private ImageProvider(Display display, NXCSession session)
   {
      this.display = display;
      this.session = session;
      missingImage = ResourceManager.getImage("icons/missing.png");
   }

   /**
    * Add update listener. Has no effect if same listener already added. Listener always called in UI thread.
    *
    * @param listener listener to add
    */
   public void addUpdateListener(final ImageUpdateListener listener)
   {
      updateListeners.add(listener);
   }

   /**
    * Remove update listener. Has no effect if same listener already removed or was not added.
    *
    * @param listener listener to remove
    */
   public void removeUpdateListener(final ImageUpdateListener listener)
   {
      updateListeners.remove(listener);
   }

   /**
    * Notify listeners on image update.
    *
    * @param guid image GUID
    */
   private void notifyListeners(final UUID guid)
   {
      for(final ImageUpdateListener listener : updateListeners)
         listener.imageUpdated(guid);
   }

   /**
    * Synchronize image library metadata.
    *
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void syncMetaData() throws IOException, NXCException
   {
      List<LibraryImage> imageLibrary = session.getImageLibrary();
      libraryIndex.clear();
      clearCache();
      for(final LibraryImage libraryImage : imageLibrary)
      {
         libraryIndex.put(libraryImage.getGuid(), libraryImage);
      }
   }

   /**
    * Clear all caches.
    */
   private void clearCache()
   {
      for(Image image : rasterCache.values())
      {
         if ((image != null) && !image.isDisposed())
            image.dispose();
      }
      rasterCache.clear();
      svgCache.clear();
      imageDataCache.clear();
   }

   /**
    * Check if image with given GUID is an SVG image (based on metadata in library index).
    *
    * @param guid image GUID
    * @return true if image is SVG
    */
   private boolean isSVG(UUID guid)
   {
      LibraryImage meta = libraryIndex.get(guid);
      return (meta != null) && meta.isSVG();
   }

   /**
    * Build raster cache key from GUID and dimensions.
    */
   private static String rasterCacheKey(UUID guid, int width, int height)
   {
      return guid.toString() + "_" + width + "x" + height;
   }

   /**
    * Get or parse SVG image from cache. Requires image data to be in imageDataCache.
    *
    * @param guid image GUID
    * @return parsed SVGImage or null on failure
    */
   private SVGImage getOrParseSVG(UUID guid)
   {
      SVGImage svg = svgCache.get(guid);
      if (svg != null)
         return svg;

      LibraryImage li = imageDataCache.get(guid);
      if ((li == null) || (li.getBinaryData() == null))
         return null;

      try
      {
         svg = SVGImage.createFromStream(new ByteArrayInputStream(li.getBinaryData()));
         svgCache.put(guid, svg);
         return svg;
      }
      catch(SVGParseException e)
      {
         logger.error("Cannot parse SVG image {}", guid, e);
         return null;
      }
   }

   /**
    * Create raster image from image data cache for non-SVG images.
    *
    * @param guid image GUID
    * @return SWT Image or null on failure
    */
   private Image createRasterFromData(UUID guid)
   {
      LibraryImage li = imageDataCache.get(guid);
      if ((li == null) || (li.getBinaryData() == null))
         return null;

      try
      {
         return new Image(display, new ByteArrayInputStream(li.getBinaryData()));
      }
      catch(SWTException e)
      {
         logger.error("Cannot decode raster image {}", guid, e);
         return null;
      }
   }

   // ==================== Mode 1: Native ====================

   /**
    * Get native image (complete LibraryImage with binary data). Returns cached data if available, otherwise loads from server
    * asynchronously and returns null. Completion callback is called when load completes.
    *
    * @param guid image GUID
    * @param completionCallback callback when image is loaded (may be null)
    * @return LibraryImage with binary data, or null if not yet loaded
    */
   public LibraryImage getNativeImage(UUID guid, Runnable completionCallback)
   {
      if (guid == null)
         return null;

      LibraryImage li = imageDataCache.get(guid);
      if (li != null)
         return li;

      loadImageFromServer(guid, completionCallback);
      return null;
   }

   /**
    * Get native image (complete LibraryImage with binary data). Returns cached data if available, otherwise loads from server
    * asynchronously and returns null.
    *
    * @param guid image GUID
    * @return LibraryImage with binary data, or null if not yet loaded
    */
   public LibraryImage getNativeImage(UUID guid)
   {
      return getNativeImage(guid, null);
   }

   // ==================== Mode 2: Rasterized ====================

   /**
    * Get image rasterized at specific dimensions. For SVG, rasterizes at exact size (SWT only). For raster, scales to fit. Returns
    * missing image placeholder if not yet loaded from server.
    *
    * @param guid image GUID
    * @param width target width
    * @param height target height
    * @param completionCallback callback when image is loaded (may be null)
    * @return SWT Image at requested dimensions, or missing image placeholder
    */
   public Image getRasterizedImage(UUID guid, int width, int height, Runnable completionCallback)
   {
      if (guid == null)
         return missingImage;

      String key = rasterCacheKey(guid, width, height);
      Image cached = rasterCache.get(key);
      if (cached != null)
         return cached;

      if (imageDataCache.containsKey(guid))
      {
         Image image = createRasterizedImage(guid, width, height);
         if (image != null)
         {
            rasterCache.put(key, image);
            return image;
         }
         return missingImage;
      }

      loadImageFromServer(guid, completionCallback);
      return missingImage;
   }

   /**
    * Get image rasterized at specific dimensions.
    *
    * @param guid image GUID
    * @param width target width
    * @param height target height
    * @return SWT Image at requested dimensions, or missing image placeholder
    */
   public Image getRasterizedImage(UUID guid, int width, int height)
   {
      return getRasterizedImage(guid, width, height, null);
   }

   /**
    * Create rasterized image at specific dimensions from cached data.
    */
   private Image createRasterizedImage(UUID guid, int width, int height)
   {
      if (isSVG(guid))
      {
         SVGImage svg = getOrParseSVG(guid);
         if (svg != null)
            return ImageProviderTools.rasterizeSVG(display, svg, width, height);
         return null;
      }

      Image source = createRasterFromData(guid);
      if (source == null)
         return null;

      Rectangle bounds = source.getBounds();
      if ((bounds.width == width) && (bounds.height == height))
         return source;

      Image scaled = new Image(display, width, height);
      GC gc = new GC(scaled);
      gc.drawImage(source, 0, 0, bounds.width, bounds.height, 0, 0, width, height);
      gc.dispose();
      source.dispose();
      return scaled;
   }

   // ==================== Mode 3: Render to GC ====================

   /**
    * Render image directly to a GC at the specified position and size. Works on both SWT and RWT. For SVG, renders vector paths
    * directly to the GC for optimal quality. For raster, scales to fit the target area.
    *
    * @param guid image GUID
    * @param gc target graphics context
    * @param x target x coordinate
    * @param y target y coordinate
    * @param width target width
    * @param height target height
    */
   public void renderImage(UUID guid, GC gc, int x, int y, int width, int height)
   {
      if (guid == null)
      {
         gc.drawImage(missingImage, x, y);
         return;
      }

      if (isSVG(guid))
      {
         SVGImage svg = getOrParseSVG(guid);
         if (svg != null)
         {
            ImageProviderTools.renderImage(gc, null, svg, x, y, width, height);
            return;
         }
      }
      else
      {
         // Try raster cache first, then create from data
         String key = rasterCacheKey(guid, width, height);
         Image cached = rasterCache.get(key);
         if (cached != null)
         {
            gc.drawImage(cached, x, y);
            return;
         }

         if (imageDataCache.containsKey(guid))
         {
            Image image = createRasterizedImage(guid, width, height);
            if (image != null)
            {
               rasterCache.put(key, image);
               gc.drawImage(image, x, y);
               return;
            }
         }
      }

      // Image data not in cache - initiate async load from server
      loadImageFromServer(guid, null);

      // Draw missing image placeholder until loaded
      gc.drawImage(missingImage, x, y);
   }

   // ==================== Backward-compatible methods ====================

   /**
    * Get image by GUID at its natural size. For raster images, returns original size. For SVG images, uses viewBox dimensions if
    * available, otherwise defaults to 256x256.
    *
    * @param guid image GUID
    * @return corresponding image or special image representing missing image
    */
   public Image getImage(UUID guid)
   {
      return getImage(guid, DEFAULT_SVG_SIZE, DEFAULT_SVG_SIZE, null);
   }

   /**
    * Get image by GUID at its natural size with a completion callback.
    *
    * @param guid image GUID
    * @param completionCallback callback to be called when missing image loaded from server
    * @return corresponding image or special image representing missing image
    */
   public Image getImage(UUID guid, Runnable completionCallback)
   {
      return getImage(guid, DEFAULT_SVG_SIZE, DEFAULT_SVG_SIZE, completionCallback);
   }

   /**
    * Get image by GUID. For SVG images without viewBox, uses provided default dimensions.
    *
    * @param guid image GUID
    * @param defaultWidth default width for SVG without viewBox
    * @param defaultHeight default height for SVG without viewBox
    * @param completionCallback callback to be called when missing image loaded from server
    * @return corresponding image or special image representing missing image
    */
   public Image getImage(UUID guid, int defaultWidth, int defaultHeight, Runnable completionCallback)
   {
      if (guid == null)
         return missingImage;

      // For raster images, use original-size cache (key with 0x0 means "original size")
      if (!isSVG(guid))
      {
         String key = rasterCacheKey(guid, 0, 0);
         Image cached = rasterCache.get(key);
         if (cached != null)
            return cached;

         if (imageDataCache.containsKey(guid))
         {
            Image image = createRasterFromData(guid);
            if (image != null)
            {
               rasterCache.put(key, image);
               return image;
            }
            return missingImage;
         }

         loadImageFromServer(guid, completionCallback);
         return missingImage;
      }

      // SVG: determine target size from viewBox or defaults
      if (!imageDataCache.containsKey(guid))
      {
         loadImageFromServer(guid, completionCallback);
         return missingImage;
      }

      SVGImage svg = getOrParseSVG(guid);
      if (svg == null)
         return missingImage;

      int width = (svg.getWidth() > 0) ? (int)svg.getWidth() : defaultWidth;
      int height = (svg.getHeight() > 0) ? (int)svg.getHeight() : defaultHeight;

      // Cap dimensions to prevent OOM on SVGs with large viewBox values (e.g. mm-based units)
      if ((width > MAX_SVG_RASTER_SIZE) || (height > MAX_SVG_RASTER_SIZE))
      {
         float scale = (float)MAX_SVG_RASTER_SIZE / Math.max(width, height);
         width = Math.max(1, Math.round(width * scale));
         height = Math.max(1, Math.round(height * scale));
      }

      return getRasterizedImage(guid, width, height);
   }

   /**
    * Get image as object icon. Image is resized to 16x16 (scaled for DPI).
    *
    * @param guid image GUID
    * @return corresponding image as object icon or special image representing missing image
    */
   public Image getObjectIcon(UUID guid)
   {
      return getObjectIcon(guid, null);
   }

   /**
    * Get image as object icon. Image is resized to 16x16 (scaled for DPI).
    *
    * @param guid image GUID
    * @param completionCallback callback to be called when missing image loaded from server
    * @return corresponding image as object icon or special image representing missing image
    */
   public Image getObjectIcon(UUID guid, Runnable completionCallback)
   {
      if (guid == null)
         return missingImage;

      if (isSVG(guid))
         return getRasterizedImage(guid, 16, 16, completionCallback);

      // For raster icons, use createResizedImage for DPI-aware sizing
      String key = rasterCacheKey(guid, 16, 16);
      Image cached = rasterCache.get(key);
      if (cached != null)
         return cached;

      // Need original-size image first
      Image original = getImage(guid, completionCallback);
      if (original == missingImage)
         return missingImage;

      Image icon = ImageProviderTools.createResizedImage(original, 16);
      rasterCache.put(key, icon);
      return icon;
   }

   // ==================== Server loading ====================

   /**
    * Load image from server asynchronously.
    *
    * @param guid image GUID
    * @param completionCallback callback when loading completes
    */
   private void loadImageFromServer(final UUID guid, final Runnable completionCallback)
   {
      if (!pendingLoads.add(guid))
         return; // Already loading

      if (!libraryIndex.containsKey(guid))
      {
         logger.debug("Image GUID {} not found in library index", guid);
         pendingLoads.remove(guid);
         return;
      }

      Job job = new Job(i18n.tr("Loading image from server"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final LibraryImage libraryImage = session.getImage(guid);
               runInUIThread(() -> {
                  imageDataCache.put(guid, libraryImage);
                  // Update metadata in index (server response has full data)
                  libraryIndex.put(guid, libraryImage);
                  pendingLoads.remove(guid);
                  notifyListeners(guid);
                  if (completionCallback != null)
                     completionCallback.run();
               });
            }
            catch(Exception e)
            {
               logger.error("Cannot retrieve image from server", e);
               pendingLoads.remove(guid);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load image from server");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Preload image from server. This is a synchronous call that will return only after image is loaded and added to the cache.
    *
    * @param guid library image GUID
    */
   public void preloadImageFromServer(UUID guid)
   {
      try
      {
         LibraryImage libraryImage = session.getImage(guid);
         imageDataCache.put(guid, libraryImage);
         libraryIndex.put(guid, libraryImage);
      }
      catch(Exception e)
      {
         logger.error("Cannot retrieve image from server", e);
      }
   }

   // ==================== Library index accessors ====================

   /**
    * Get image library metadata object.
    *
    * @param guid image GUID
    * @return image library element or null if there are no image with given GUID
    */
   public LibraryImage getLibraryImageObject(final UUID guid)
   {
      return libraryIndex.get(guid);
   }

   /**
    * Get list of all images in library (metadata only).
    *
    * @return list of all library images
    */
   public List<LibraryImage> getImageLibrary()
   {
      return new ArrayList<LibraryImage>(libraryIndex.values());
   }

   // ==================== Cache invalidation ====================

   /**
    * Update image (called from session notification).
    *
    * @param guid image GUID
    */
   public void updateImage(final UUID guid)
   {
      invalidateImageCaches(guid);

      Job job = new Job(i18n.tr("Update library image"), null, null, display) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               LibraryImage imageHandle = session.getImage(guid);
               imageDataCache.put(guid, imageHandle);
               libraryIndex.put(guid, imageHandle);
               runInUIThread(() -> notifyListeners(guid));
            }
            catch(Exception e)
            {
               logger.error("Cannot update library image", e);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }

   /**
    * Delete image (called from session notification).
    *
    * @param guid image GUID
    */
   public void deleteImage(UUID guid)
   {
      invalidateImageCaches(guid);
      libraryIndex.remove(guid);

      display.asyncExec(() -> notifyListeners(guid));
   }

   /**
    * Invalidate all cached data for given image GUID.
    */
   private void invalidateImageCaches(UUID guid)
   {
      imageDataCache.remove(guid);
      svgCache.remove(guid);

      // Remove all rasterized versions for this GUID
      String prefix = guid.toString() + "_";
      synchronized(rasterCache)
      {
         rasterCache.entrySet().removeIf(entry -> {
            if (entry.getKey().startsWith(prefix))
            {
               Image image = entry.getValue();
               if ((image != null) && !image.isDisposed())
                  image.dispose();
               return true;
            }
            return false;
         });
      }
   }
}
