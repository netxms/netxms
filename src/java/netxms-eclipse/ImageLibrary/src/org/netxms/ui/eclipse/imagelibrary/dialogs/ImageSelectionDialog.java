package org.netxms.ui.eclipse.imagelibrary.dialogs;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
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
import org.netxms.api.client.images.LibraryImage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.nebula.widgets.gallery.DefaultGalleryGroupRenderer;
import org.netxms.nebula.widgets.gallery.DefaultGalleryItemRenderer;
import org.netxms.nebula.widgets.gallery.Gallery;
import org.netxms.nebula.widgets.gallery.GalleryItem;
import org.netxms.ui.eclipse.imagelibrary.Activator;
import org.netxms.ui.eclipse.imagelibrary.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class ImageSelectionDialog extends Dialog
{

	private static final String SELECT_IMAGE_CY = "SelectImage.cy";
	private static final String SELECT_IMAGE_CX = "SelectImage.cx";
	private NXCSession session;
	private Gallery gallery;
	private LibraryImage selectedImage;

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
			selectedImage = (LibraryImage)selection[0].getData();
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
		final List<LibraryImage> imageLibrary = session.getImageLibrary();

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
				final LibraryImage completeImage;
				if (!image.isComplete())
				{
					completeImage = session.getImage(image.getGuid());
				}
				else
				{
					completeImage = image;
				}
				final GalleryItem imageItem = new GalleryItem(categoryItem, SWT.NONE);
				imageItem.setText(completeImage.getName());
				final byte[] binaryData = completeImage.getBinaryData();
				if (binaryData != null)
				{
					final ByteArrayInputStream stream = new ByteArrayInputStream(binaryData);
					imageItem.setImage(new Image(Display.getDefault(), stream));
				}
				imageItem.setData(completeImage);
			}
		}
	}

	public LibraryImage getLibraryImage()
	{
		return selectedImage;
	}

	public Image getImageObject()
	{
		final Image image;
		if (selectedImage != null)
		{
			final ByteArrayInputStream stream = new ByteArrayInputStream(selectedImage.getBinaryData());
			image = new Image(Display.getDefault(), stream);
		}
		else
		{
			image = null;
		}
		return image;
	}

}
