/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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

	private static ImageProvider instance = null;

   private static final Map<UUID, Image> cache = Collections.synchronizedMap(new HashMap<UUID, Image>());
	private static final Map<UUID, LibraryImage> libraryIndex = Collections.synchronizedMap(new HashMap<UUID, LibraryImage>());

	public static void createInstance(Display display, NXCSession session)
	{
		if (instance == null)
			instance = new ImageProvider(display, session);
	}

	/**
	 * @return
	 */
	public static ImageProvider getInstance()
	{
		return instance;
	}

   private final I18n i18n = LocalizationHelper.getI18n(ImageProvider.class);
	private final Image missingImage;
	private final Set<ImageUpdateListener> updateListeners;

	private NXCSession session;
	private Display display;

	/**
	 * 
	 */
	private ImageProvider(Display display, NXCSession session)
	{
		this.display = display;
		this.session = session;
      missingImage = ResourceManager.getImage("icons/missing.png");
		updateListeners = new HashSet<ImageUpdateListener>();
	}

	/**
	 * @param listener
	 */
	public void addUpdateListener(final ImageUpdateListener listener)
	{
		updateListeners.add(listener);
	}

	/**
	 * @param listener
	 */
	public void removeUpdateListener(final ImageUpdateListener listener)
	{
		updateListeners.remove(listener);
	}

	/**
	 * @throws NXCException
	 * @throws IOException
	 */
	public void syncMetaData() throws NXCException, IOException
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
		for(Image image : cache.values())
		{
			if (image != missingImage)
			{
				image.dispose();
			}
		}
		cache.clear();
	}

	/**
	 * @param guid
	 * @return
	 */
	public Image getImage(final UUID guid)
	{
	   if (guid == null)
	      return missingImage;
	   
		final Image image;
		if (cache.containsKey(guid))
		{
			image = cache.get(guid);
		}
		else
		{
			image = missingImage;
			cache.put(guid, image);
			if (libraryIndex.containsKey(guid))
			{
            new Job(i18n.tr("Load image from server"), null) {
					@Override
               protected void run(IProgressMonitor monitor) throws Exception
					{
						loadImageFromServer(guid);
					}
					
					@Override
					protected String getErrorMessage()
					{
                  return i18n.tr("Cannot load image from server");
					}
				}.start();
			}
		}
		return image;
	}

	/**
	 * @param guid
	 */
	private void loadImageFromServer(final UUID guid)
	{
		LibraryImage libraryImage;
		try
		{
			libraryImage = session.getImage(guid);
			//libraryIndex.put(guid, libraryImage); // replace existing half-loaded object w/o image data
			final ByteArrayInputStream stream = new ByteArrayInputStream(libraryImage.getBinaryData());
			try
			{
				cache.put(guid, new Image(display, stream));
				notifySubscribers(guid);
			}
			catch(SWTException e)
			{
            logger.error("Cannot decode image", e);
				cache.put(guid, missingImage);
			}
		}
		catch(Exception e)
		{
         logger.error("Cannot retrive image from server", e);
		}
	}

	/**
	 * @param guid
	 */
	private void notifySubscribers(final UUID guid)
	{
		for(final ImageUpdateListener listener : updateListeners)
		{
			listener.imageUpdated(guid);
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
      Image image = cache.remove(guid);
		if (image != null && image != missingImage)
			image.dispose();

      Job job = new Job(i18n.tr("Update library image"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               LibraryImage imageHandle = session.getImage(guid);
               libraryIndex.put(guid, imageHandle);
               notifySubscribers(guid);
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
      Image image = cache.remove(guid);
      if (image != null && image != missingImage)
         image.dispose();
      libraryIndex.remove(guid);
      notifySubscribers(guid);
   }
}
