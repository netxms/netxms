/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Image;
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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Image provider for image library
 */
public class ImageProvider
{
   private static final Logger logger = LoggerFactory.getLogger(ImageProvider.class);

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
	 * @return
	 */
	public static ImageProvider getInstance()
	{
      return Registry.getSingleton(ImageProvider.class);
	}

   private final I18n i18n = LocalizationHelper.getI18n(ImageProvider.class);
   private final NXCSession session;
   private final Display display;
	private final Image missingImage;
   private final Map<UUID, Image> imageCache = Collections.synchronizedMap(new HashMap<UUID, Image>());
   private final Map<UUID, Image> objectIconCache = Collections.synchronizedMap(new HashMap<UUID, Image>());
   private final Map<UUID, LibraryImage> libraryIndex = Collections.synchronizedMap(new HashMap<UUID, LibraryImage>());
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
    * Clear image cache
    */
	private void clearCache()
	{
		for(Image image : imageCache.values())
		{
			if (image != missingImage)
				image.dispose();
		}
		imageCache.clear();

      for(Image image : objectIconCache.values())
      {
         if ((image != missingImage) && !image.isDisposed())
            image.dispose();
      }
      objectIconCache.clear();
	}

	/**
    * Get image by GUID.
    *
    * @param guid image GUID
    * @return corresponding image or special image representing missing image
    */
   public Image getImage(UUID guid)
	{
      return getImage(guid, null);
   }

   /**
    * Get image by GUID.
    *
    * @param guid image GUID
    * @param completionCallback callback to be called when missing image loaded from server
    * @return corresponding image or special image representing missing image
    */
   public Image getImage(UUID guid, Runnable completionCallback)
   {
	   if (guid == null)
	      return missingImage;

		final Image image;
		if (imageCache.containsKey(guid))
		{
			image = imageCache.get(guid);
		}
		else
		{
			image = missingImage;
         loadImageFromServer(guid, completionCallback);
		}
		return image;
	}

   /**
    * Get image as object icon. Image is cut to square form if needed, and resized according to current device zoom level (16x16 on
    * 100% zoom).
    * 
    * @param guid image GUID
    * @return corresponding image as object icon or special image representing missing image
    */
   public Image getObjectIcon(UUID guid)
   {
      return getObjectIcon(guid, null);
   }

   /**
    * Get image as object icon. Image is cut to square form if needed, and resized according to current device zoom level (16x16 on
    * 100% zoom).
    * 
    * @param guid image GUID
    * @param completionCallback callback to be called when missing image loaded from server
    * @return corresponding image as object icon or special image representing missing image
    */
   public Image getObjectIcon(UUID guid, Runnable completionCallback)
   {
      if (guid == null)
         return missingImage;

      Image image;
      if (objectIconCache.containsKey(guid))
      {
         image = objectIconCache.get(guid);
      }
      else if (imageCache.containsKey(guid))
      {
         image = ImageProviderTools.createResizedImage(imageCache.get(guid), 16);
         objectIconCache.put(guid, image);
      }
      else
      {
         image = missingImage;
         loadImageFromServer(guid, completionCallback);
      }
      return image;
   }

	/**
    * Load image from server asynchronously.
    *
    * @param guid image GUID
    */
   private void loadImageFromServer(final UUID guid, final Runnable completionCallback)
	{
      imageCache.put(guid, missingImage);
      if (!libraryIndex.containsKey(guid))
      {
         logger.debug("Image GUID {} not found in library index", guid);
         return;
      }

      Job job = new Job(i18n.tr("Loading image from server"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            LibraryImage libraryImage;
            try
            {
               libraryImage = session.getImage(guid);
               final ByteArrayInputStream stream = new ByteArrayInputStream(libraryImage.getBinaryData());
               runInUIThread(() -> {
                  try
                  {
                     imageCache.put(guid, new Image(display, stream));
                     objectIconCache.remove(guid);
                     notifyListeners(guid);
                     if (completionCallback != null)
                     {
                        completionCallback.run();
                     }
                  }
                  catch(SWTException e)
                  {
                     logger.error("Cannot decode image", e);
                     imageCache.put(guid, missingImage);
                  }
               });
            }
            catch(Exception e)
            {
               logger.error("Cannot retrive image from server", e);
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
         final ByteArrayInputStream stream = new ByteArrayInputStream(libraryImage.getBinaryData());
         try
         {
            imageCache.put(guid, new Image(display, stream));
            objectIconCache.remove(guid);
         }
         catch(SWTException e)
         {
            logger.error("Cannot decode image", e);
         }
      }
      catch(Exception e)
      {
         logger.error("Cannot retrive image from server", e);
      }
   }

	/**
	 * Get image library object
	 * 
	 * @param guid image GUID
	 * @return image library element or null if there are no image with given GUID
	 */
	public LibraryImage getLibraryImageObject(final UUID guid)
	{
		return libraryIndex.get(guid);
	}

	/**
	 * @return
	 */
	public List<LibraryImage> getImageLibrary()
	{
		return new ArrayList<LibraryImage>(libraryIndex.values());
	}

   /**
    * Update image
    * 
    * @param guid image GUID
    */
   public void updateImage(final UUID guid)
	{
      Image image = imageCache.remove(guid);
      if ((image != null) && (image != missingImage))
			image.dispose();

      image = objectIconCache.remove(guid);
      if ((image != null) && (image != missingImage) && !image.isDisposed())
         image.dispose();

      Job job = new Job(i18n.tr("Update library image"), null, null, display) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               LibraryImage imageHandle = session.getImage(guid);
               libraryIndex.put(guid, imageHandle);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     notifyListeners(guid);
                  }
               });
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
    * Delete image
    * 
    * @param guid image GUID
    */
   public void deleteImage(UUID guid)
   {
      Image image = imageCache.remove(guid);
      if ((image != null) && (image != missingImage))
         image.dispose();

      image = objectIconCache.remove(guid);
      if ((image != null) && (image != missingImage) && !image.isDisposed())
         image.dispose();

      libraryIndex.remove(guid);

      display.asyncExec(new Runnable() {
         @Override
         public void run()
         {
            notifyListeners(guid);
         }
      });
   }
}
