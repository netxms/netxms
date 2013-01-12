package org.netxms.ui.eclipse.imagelibrary.shared;

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
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.rap.rwt.SingletonUtil;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.netxms.api.client.images.LibraryImage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.imagelibrary.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;

public class ImageProvider
{
	private static final Map<UUID, Image> cache = Collections.synchronizedMap(new HashMap<UUID, Image>());
	private static final Map<UUID, LibraryImage> libraryIndex = Collections.synchronizedMap(new HashMap<UUID, LibraryImage>());

	/**
	 * @param display
	 * @param session
	 */
	public static void createInstance(final Display display, final NXCSession session)
	{
		display.syncExec(new Runnable() {
			@Override
			public void run()
			{
				ImageProvider p = SingletonUtil.getSessionInstance(ImageProvider.class);
				p.init(display, session);
			}
		});
	}

	/**
	 * Get image provider's instance for specific display
	 * 
	 * @return
	 */
	public static ImageProvider getInstance(Display display)
	{
		InstanceQuery q = new InstanceQuery();
		display.syncExec(q);
		return q.instance;
	}
	
	/**
	 * Helper class for query instance on UI thread
	 */
	private static class InstanceQuery implements Runnable
	{
		public ImageProvider instance;

		@Override
		public void run()
		{
			instance = SingletonUtil.getSessionInstance(ImageProvider.class);
		}
	}

	private Image missingImage;
	private Set<ImageUpdateListener> updateListeners;

	private boolean initialized = false;
	private NXCSession session;
	private Display display;
	
	/**
	 * 
	 */
	private ImageProvider()
	{
	}

	/**
	 * 
	 */
	private void init(Display display, NXCSession session)
	{
		if (initialized)
			return;
		
		this.display = display;
		this.session = session;
		final ImageDescriptor imageDescriptor = AbstractUIPlugin.imageDescriptorFromPlugin(Activator.PLUGIN_ID, "icons/missing.png");
		missingImage = imageDescriptor.createImage(display);
		updateListeners = new HashSet<ImageUpdateListener>();
		initialized = true;
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
	 * 
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
				new ConsoleJob("Load Image", null, Activator.PLUGIN_ID, null) {
					
					@Override
					protected void runInternal(IProgressMonitor monitor) throws Exception
					{
						loadImageFromServer(guid);
					}
					
					@Override
					protected String getErrorMessage()
					{
						// TODO Auto-generated method stub
						return null;
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
				display.syncExec(new Runnable() {
					@Override
					public void run()
					{
						final Image image = new Image(display, stream);
						if (image != null)
							cache.put(guid, image);
					}
				});
				notifySubscribers(guid);
			}
			catch(SWTException e)
			{
				Activator.logError("Cannot decode image", e);
				cache.put(guid, missingImage);
			}
		}
		catch(Exception e)
		{
			Activator.logError("Cannot retrive image from server", e);
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

	public void invalidateImage(UUID guid, boolean removed)
	{
		Image image = cache.get(guid);
		if (image != null && image != missingImage)
		{
			image.dispose();
		}
		cache.remove(guid);
		if (removed)
		{
			libraryIndex.remove(guid);
		}
		notifySubscribers(guid);
	}
}
