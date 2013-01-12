package org.netxms.ui.eclipse.imagelibrary.dialogs;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.netxms.api.client.images.LibraryImage;
import org.netxms.base.NXCommon;
import org.netxms.nebula.widgets.gallery.DefaultGalleryGroupRenderer;
import org.netxms.nebula.widgets.gallery.DefaultGalleryItemRenderer;
import org.netxms.nebula.widgets.gallery.Gallery;
import org.netxms.nebula.widgets.gallery.GalleryItem;
import org.netxms.ui.eclipse.imagelibrary.Activator;
import org.netxms.ui.eclipse.imagelibrary.Messages;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageUpdateListener;

public class ImageSelectionDialog extends Dialog implements SelectionListener, MouseListener, ImageUpdateListener
{
	private static final long serialVersionUID = 1L;

	/**
	 * Zero
	 */
	public static final int NONE = 0;
	/**
	 * Show "Default" button and return null as image is it's pressed
	 */
	public static final int ALLOW_DEFAULT = 1;

	private static final String SELECT_IMAGE_CY = "SelectImage.cy";
	private static final String SELECT_IMAGE_CX = "SelectImage.cx";
	private static final int DEFAULT_ID = IDialogConstants.CLIENT_ID + 1;

	private Gallery gallery;
	private LibraryImage libraryImage;
	private Image imageObject;
	private int flags;
	private int maxWidth = Integer.MAX_VALUE;
	private int maxHeight = Integer.MAX_VALUE;

	/**
	 * Construct new dialog with default flags: single selection, no "default"
	 * buttion, no null images on OK button
	 * 
	 * @param parentShell
	 */
	public ImageSelectionDialog(Shell parentShell)
	{
		this(parentShell, NONE);
	}

	/**
	 * Construct new Image selection dialog with custom flags.
	 * 
	 * @param parentShell
	 * @param flags
	 *           creation flags
	 */
	public ImageSelectionDialog(final Shell parentShell, final int flags)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.flags = flags;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#close()
	 */
	@Override
	public boolean close()
	{
		ImageProvider.getInstance(getShell().getDisplay()).removeUpdateListener(this);
		return super.close();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		final FillLayout layout = new FillLayout();
		dialogArea.setLayout(layout);

		gallery = new Gallery(dialogArea, SWT.V_SCROLL /* | SWT.MULTI */);

		DefaultGalleryGroupRenderer galleryGroupRenderer = new DefaultGalleryGroupRenderer();

		galleryGroupRenderer.setMinMargin(2);
		galleryGroupRenderer.setItemHeight(48);
		galleryGroupRenderer.setItemWidth(48);
		galleryGroupRenderer.setAutoMargin(true);
		galleryGroupRenderer.setAlwaysExpanded(true);
		gallery.setGroupRenderer(galleryGroupRenderer);

		DefaultGalleryItemRenderer itemRenderer = new DefaultGalleryItemRenderer();
		gallery.setItemRenderer(itemRenderer);

		gallery.addSelectionListener(this);
		gallery.addMouseListener(this);
		ImageProvider.getInstance(getShell().getDisplay()).addUpdateListener(this);

		refreshImages();

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
	 */
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

	private void defaultPressed()
	{
		imageObject = null;
		libraryImage = new LibraryImage();
		saveSettings();
		super.okPressed();
	}

	@Override
	protected void buttonPressed(int buttonId)
	{
		super.buttonPressed(buttonId);
		if (DEFAULT_ID == buttonId)
		{
			defaultPressed();
		}
	}

	private void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put(SELECT_IMAGE_CX, size.x);
		settings.put(SELECT_IMAGE_CY, size.y);
	}

	@Override
	protected void createButtonsForButtonBar(Composite parent)
	{
		if ((flags & ALLOW_DEFAULT) == ALLOW_DEFAULT)
		{
			createButton(parent, DEFAULT_ID, "Default", false);
		}
		super.createButtonsForButtonBar(parent);
		getButton(IDialogConstants.OK_ID).setEnabled(false);
	}

	/**
	 * 
	 */
	private void refreshImages()
	{
		final ImageProvider provider = ImageProvider.getInstance(getShell().getDisplay());
		final List<LibraryImage> imageLibrary = provider.getImageLibrary();

		Map<String, List<LibraryImage>> categories = new HashMap<String, List<LibraryImage>>();
		for(LibraryImage image : imageLibrary)
		{
			final String category = image.getCategory();
			final Image swtImage = provider.getImage(image.getGuid());
			final Rectangle bounds = swtImage.getBounds();
			if (bounds.height <= maxHeight && bounds.width <= maxWidth)
			{
				if (!categories.containsKey(category))
				{
					categories.put(category, new ArrayList<LibraryImage>());
				}
				categories.get(category).add(image);
			}
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

		gallery.redraw();
	}

	public Image getImage()
	{
		return imageObject;
	}

	public UUID getGuid()
	{
		return libraryImage == null ? NXCommon.EMPTY_GUID : libraryImage.getGuid();
	}

	/**
	 * @return the libraryImage
	 */
	public LibraryImage getLibraryImage()
	{
		return libraryImage;
	}

	@Override
	public void widgetSelected(SelectionEvent e)
	{
		final GalleryItem[] selection = gallery.getSelection();
		if (selection.length > 0)
		{
			getButton(IDialogConstants.OK_ID).setEnabled(true);
		}
		else
		{
			getButton(IDialogConstants.OK_ID).setEnabled(false);
		}
	}

	@Override
	public void widgetDefaultSelected(SelectionEvent e)
	{
	}

	@Override
	public void mouseDoubleClick(MouseEvent e)
	{
		final GalleryItem[] selection = gallery.getSelection();
		if (selection.length > 0)
		{
			okPressed();
		}
	}

	@Override
	public void mouseDown(MouseEvent e)
	{
	}

	@Override
	public void mouseUp(MouseEvent e)
	{
	}

	@Override
	public void imageUpdated(UUID guid)
	{
		final Shell shell = getShell();
		if (shell != null)
		{
			final Display display = shell.getDisplay();
			display.asyncExec(new Runnable()
			{
				@Override
				public void run()
				{
					refreshImages();
				}
			});
		}
	}

	/**
	 * Set maximum allowed dimentions for images snowh by this instance of
	 * {@link ImageSelectionDialog}
	 * 
	 * @param width
	 * @param height
	 */
	public void setMaxImageDimensions(final int width, final int height)
	{
		this.maxWidth = width;
		this.maxHeight = height;
		final Shell shell = getShell();
		if (shell != null)
		{
			shell.getDisplay().asyncExec(new Runnable()
			{

				@Override
				public void run()
				{
					refreshImages();
				}
			});
		}
	}
}
