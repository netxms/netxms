package org.netxms.ui.eclipse.imagelibrary.dialogs;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.images.LibraryImage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.nebula.widgets.gallery.DefaultGalleryGroupRenderer;
import org.netxms.nebula.widgets.gallery.DefaultGalleryItemRenderer;
import org.netxms.nebula.widgets.gallery.Gallery;
import org.netxms.nebula.widgets.gallery.GalleryItem;
import org.netxms.ui.eclipse.imagelibrary.Activator;
import org.netxms.ui.eclipse.imagelibrary.Messages;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageUpdateListener;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class ImageSelectionDialog extends Dialog
{

	private static final String SELECT_IMAGE_CY = "SelectImage.cy";
	private static final String SELECT_IMAGE_CX = "SelectImage.cx";
	private NXCSession session;
	private Gallery gallery;
	private LibraryImage libraryImage;
	private Image imageObject;

	public ImageSelectionDialog(Shell parentShell)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
	}

	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.getString("ImageSelectionDialog.title"));
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt(SELECT_IMAGE_CX), settings.getInt(SELECT_IMAGE_CY));
		}
		catch(NumberFormatException e)
		{
			newShell.setSize(400, 350);
		}
	}

	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		final FillLayout layout = new FillLayout();
		dialogArea.setLayout(layout);

		session = (NXCSession)ConsoleSharedData.getSession();

		gallery = new Gallery(dialogArea, SWT.V_SCROLL | SWT.MULTI);

		DefaultGalleryGroupRenderer galleryGroupRenderer = new DefaultGalleryGroupRenderer();

		galleryGroupRenderer.setMinMargin(2);
		galleryGroupRenderer.setItemHeight(48);
		galleryGroupRenderer.setItemWidth(48);
		galleryGroupRenderer.setAutoMargin(true);
		galleryGroupRenderer.setAlwaysExpanded(true);
		gallery.setGroupRenderer(galleryGroupRenderer);

		DefaultGalleryItemRenderer itemRenderer = new DefaultGalleryItemRenderer();
		gallery.setItemRenderer(itemRenderer);

		final Display display = getShell().getDisplay();
		final ImageProvider provider = ImageProvider.getInstance();
		provider.addUpdateListener(new ImageUpdateListener()
		{
			@Override
			public void imageUpdated(UUID guid)
			{
				new UIJob(display, "Image picker update")
				{

					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						try
						{
							refreshImages();
						}
						catch(NXCException e)
						{
							e.printStackTrace();
						}
						catch(IOException e)
						{
							e.printStackTrace();
						}
						return Status.OK_STATUS;
					}
				}.schedule();
			}
		});

		try
		{
			refreshImages();
		}
		catch(Exception e1)
		{
			e1.printStackTrace();
		}

		return dialogArea;
	}

	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

	@Override
	protected void okPressed()
	{
		final GalleryItem[] selection = gallery.getSelection();
		if (selection.length > 0)
		{
			libraryImage = (LibraryImage)selection[0].getData();
			imageObject = selection[0].getImage();
		}
		saveSettings();
		super.okPressed();
	}

	private void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put(SELECT_IMAGE_CX, size.x);
		settings.put(SELECT_IMAGE_CY, size.y);
	}

	private void refreshImages() throws NXCException, IOException
	{
		final ImageProvider provider = ImageProvider.getInstance();
		final List<LibraryImage> imageLibrary = provider.getImageLibrary();

		Map<String, List<LibraryImage>> categories = new HashMap<String, List<LibraryImage>>();
		for(LibraryImage image : imageLibrary)
		{
			final String category = image.getCategory();
			if (!categories.containsKey(category))
			{
				categories.put(category, new ArrayList<LibraryImage>());
			}
			categories.get(category).add(image);
		}
		// this.knownCategories = categories.keySet();

		gallery.removeAll();
		for(String category : categories.keySet())
		{
			final GalleryItem categoryItem = new GalleryItem(gallery, SWT.NONE);
			categoryItem.setText(category);
			final List<LibraryImage> categoryImages = categories.get(category);
			for(LibraryImage image : categoryImages)
			{
				final GalleryItem imageItem = new GalleryItem(categoryItem, SWT.NONE);
				imageItem.setText(image.getName());
				imageItem.setImage(provider.getImage(image.getGuid()));
				imageItem.setData(image);
			}
		}
	}

	public Image getImage()
	{
		return imageObject;
	}

	public UUID getGuid()
	{
		return libraryImage == null ? null : libraryImage.getGuid();
	}
}
