package org.netxms.ui.eclipse.imagelibrary.shared;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.netxms.api.client.images.LibraryImage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.imagelibrary.Activator;

public class ImageProvider
{
	private static ImageProvider instance = new ImageProvider();

	private static final Map<UUID, Image> cache = Collections.synchronizedMap(new HashMap<UUID, Image>());
	private static final Map<UUID, LibraryImage> libraryIndex = Collections.synchronizedMap(new HashMap<UUID, LibraryImage>());

	/**
	 * @return
	 */
	public static ImageProvider getInstance()
	{
		return instance;
	}

	private final Image missingImage;
	private final Set<ImageUpdateListener> updateListeners;

	private List<LibraryImage> imageLibrary;

	/**
	 * 
	 */
	private ImageProvider()
	{
		final ImageDescriptor imageDescriptor = AbstractUIPlugin.imageDescriptorFromPlugin(Activator.PLUGIN_ID, "icons/missing.png");
		missingImage = imageDescriptor.createImage();
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
	public void syncMetaData(final NXCSession session, final Display display) throws NXCException, IOException
	{
		imageLibrary = session.getImageLibrary();
		for(final LibraryImage libraryImage : imageLibrary)
		{
			libraryIndex.put(libraryImage.getGuid(), libraryImage);
			if (!libraryImage.isComplete())
			{
				try
				{
					final LibraryImage completeLibraryImage = session.getImage(libraryImage.getGuid());
					final ByteArrayInputStream stream = new ByteArrayInputStream(completeLibraryImage.getBinaryData());
					try
					{
						final Image image = new Image(display, stream);
						cache.put(completeLibraryImage.getGuid(), image);
					}
					catch(SWTException e)
					{
						Activator.logError("Exception in ImageProvider.syncMetaData()", e);
						cache.put(completeLibraryImage.getGuid(), missingImage);
					}

					for(final ImageUpdateListener listener : updateListeners)
					{
						listener.imageUpdated(completeLibraryImage.getGuid());
					}
				}
				catch(Exception e)
				{
					Activator.logError("Exception in ImageProvider.syncMetaData()", e);
				}
			}
		}
	}

	/**
	 * @param guid
	 * @return
	 */
	public Image getImage(final UUID guid)
	{
		final Image image;
		if (cache.containsKey(guid))
		{
			image = cache.get(guid);
		}
		else
		{
			image = missingImage;
		}
		return image;
	}

	/**
	 * Get image library object
	 * 
	 * @param guid
	 *           image GUID
	 * @return image library element or null if there are no image with given
	 *         GUID
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
		return imageLibrary;
	}
}
