package org.netxms.ui.eclipse.imagelibrary.views;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.MissingFormatArgumentException;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.events.MenuDetectEvent;
import org.eclipse.swt.events.MenuDetectListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.NetXMSClientException;
import org.netxms.api.client.images.LibraryImage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.nebula.widgets.gallery.DefaultGalleryGroupRenderer;
import org.netxms.nebula.widgets.gallery.DefaultGalleryItemRenderer;
import org.netxms.nebula.widgets.gallery.Gallery;
import org.netxms.nebula.widgets.gallery.GalleryItem;
import org.netxms.ui.eclipse.imagelibrary.Activator;
import org.netxms.ui.eclipse.imagelibrary.dialogs.ImagePropertiesDialog;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

public class ImageLibrary extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.imagelibrary.view.imagelibrary";

	protected static final int MIN_GRID_ICON_SIZE = 48;
	protected static final int MAX_GRID_ICON_SIZE = 256;

	private Gallery gallery;
	private Action actionNew;
	private Action actionEdit;
	private Action actionDelete;

	private Action actionRefresh;
	private Action actionZoomIn;
	private Action actionZoomOut;

	protected MenuDetectEvent menuEvent;

	private NXCSession session;

	private Set<String> knownCategories;

	protected int currentIconSize = MIN_GRID_ICON_SIZE;

	@Override
	public void createPartControl(final Composite parent)
	{
		final FillLayout layout = new FillLayout();
		parent.setLayout(layout);

		session = (NXCSession)ConsoleSharedData.getSession();

		gallery = new Gallery(parent, SWT.V_SCROLL | SWT.MULTI);

		DefaultGalleryGroupRenderer galleryGroupRenderer = new DefaultGalleryGroupRenderer();

		galleryGroupRenderer.setMinMargin(2);
		galleryGroupRenderer.setItemHeight(MIN_GRID_ICON_SIZE);
		galleryGroupRenderer.setItemWidth(MIN_GRID_ICON_SIZE);
		galleryGroupRenderer.setAutoMargin(true);
		galleryGroupRenderer.setAlwaysExpanded(true);
		gallery.setGroupRenderer(galleryGroupRenderer);

		DefaultGalleryItemRenderer itemRenderer = new DefaultGalleryItemRenderer();
		gallery.setItemRenderer(itemRenderer);

		createActions();
		createPopupMenu();
		contributeToActionBars();

		gallery.addMenuDetectListener(new MenuDetectListener()
		{

			@Override
			public void menuDetected(MenuDetectEvent e)
			{
				menuEvent = e;
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
	}

	@Override
	public void setFocus()
	{
	}

	private void createActions()
	{
		actionNew = new Action("&Upload New Image")
		{
			@Override
			public void run()
			{
				final ImagePropertiesDialog dialog = new ImagePropertiesDialog(getSite().getShell(), knownCategories);
				final GalleryItem[] selection = gallery.getSelection();
				if (selection.length > 0)
				{
					LibraryImage image = (LibraryImage)selection[0].getData();
					dialog.setDefaultCategory(image.getCategory());
				}

				if (dialog.open() == Dialog.OK)
				{
					uploadNewImage(dialog.getName(), dialog.getCategory(), dialog.getFileName());
				}
			}
		};
		actionNew.setImageDescriptor(SharedIcons.ADD_OBJECT);

		actionEdit = new Action("&Edit")
		{
			@Override
			public void run()
			{
				final GalleryItem[] selection = gallery.getSelection();
				if (selection.length == 1)
				{
					final ImagePropertiesDialog dialog = new ImagePropertiesDialog(getSite().getShell(), knownCategories);
					LibraryImage image = (LibraryImage)selection[0].getData();
					dialog.setName(image.getName());
					dialog.setDefaultCategory(image.getCategory());
					if (dialog.open() == Dialog.OK)
					{
						editImage(selection[0], dialog.getName(), dialog.getCategory(), dialog.getFileName());
					}
				}
			}
		};
		actionEdit.setImageDescriptor(SharedIcons.EDIT);

		actionDelete = new Action("&Delete")
		{
			@Override
			public void run()
			{
				deleteImage();
			}

		};
		actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);

		actionRefresh = new Action("&Refresh")
		{
			@Override
			public void run()
			{
				try
				{
					refreshImages();
				}
				catch(NetXMSClientException e)
				{
					// FIXME
					e.printStackTrace();
				}
				catch(IOException e)
				{
					// FIXME
					e.printStackTrace();
				}
			}
		};
		actionRefresh.setImageDescriptor(SharedIcons.REFRESH);

		actionZoomIn = new Action("Zoom In")
		{
			@Override
			public void run()
			{
				final DefaultGalleryGroupRenderer groupRenderer = (DefaultGalleryGroupRenderer)gallery.getGroupRenderer();
				if (currentIconSize < MAX_GRID_ICON_SIZE)
				{
					currentIconSize += 16;
					groupRenderer.setItemHeight(currentIconSize);
					groupRenderer.setItemWidth(currentIconSize);
				}
			}
		};
		actionZoomIn.setImageDescriptor(SharedIcons.ZOOM_IN);
		actionZoomOut = new Action("Zoom Out")
		{
			@Override
			public void run()
			{
				final DefaultGalleryGroupRenderer groupRenderer = (DefaultGalleryGroupRenderer)gallery.getGroupRenderer();
				if (currentIconSize > MIN_GRID_ICON_SIZE)
				{
					currentIconSize -= 16;
					groupRenderer.setItemHeight(currentIconSize);
					groupRenderer.setItemWidth(currentIconSize);
				}
			}
		};
		actionZoomOut.setImageDescriptor(SharedIcons.ZOOM_OUT);
	}

	protected void editImage(final GalleryItem galleryItem, final String name, final String category, final String fileName)
	{
		new ConsoleJob("Update Image", this, Activator.PLUGIN_ID, null)
		{
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				new UIJob("Update Image")
				{
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						try
						{
							final LibraryImage image = (LibraryImage)galleryItem.getData();

							if (fileName != null)
							{
								// TODO: add error checking
								final long fileSize = new File(fileName).length();
								final FileInputStream stream = new FileInputStream(fileName);
								byte imageData[] = new byte[(int)fileSize];
								stream.read(imageData);

								image.setBinaryData(imageData);
							}

							if (!image.isProtected())
							{
								image.setName(name);
								image.setCategory(category);
							}

							session.modifyImage(image);

							refreshImages(); // TODO: should be changed to update
													// single elemnt
						}
						catch(Exception e)
						{
							e.printStackTrace();
						}
						return Status.OK_STATUS;
					}
				}.schedule();
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update image";
			}
		}.start();
	}

	protected void uploadNewImage(final String name, final String category, final String fileName)
	{
		new ConsoleJob("Upload New Image", this, Activator.PLUGIN_ID, null)
		{
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				new UIJob("Upload New Image")
				{
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						try
						{
							final LibraryImage image = new LibraryImage();

							// TODO: add error checking
							final long fileSize = new File(fileName).length();
							final FileInputStream stream = new FileInputStream(fileName);
							byte imageData[] = new byte[(int)fileSize];
							stream.read(imageData);

							image.setBinaryData(imageData);
							image.setName(name);
							image.setCategory(category);

							session.createImage(image);

							refreshImages(); // TODO: should be changed to add single
													// elemnt
						}
						catch(Exception e)
						{
							e.printStackTrace();
						}
						return Status.OK_STATUS;
					}
				}.schedule();
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update image";
			}
		}.start();
	}

	protected void deleteImage()
	{
		final GalleryItem[] selection = gallery.getSelection();
		for(GalleryItem item : selection)
		{
			try
			{
				session.deleteImage((LibraryImage)item.getData());
				final GalleryItem category = item.getParentItem();
				gallery.remove(item);
				if (category != null)
				{
					if (category.getItemCount() == 0)
					{
						gallery.remove(category);
					}
				}
			}
			catch(NXCException e)
			{
				e.printStackTrace();
			}
			catch(IOException e)
			{
				e.printStackTrace();
			}
		}
	}

	private void createPopupMenu()
	{
		MenuManager menuManager = new MenuManager();
		menuManager.setRemoveAllWhenShown(true);
		menuManager.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager manager)
			{
				fillContextMenu(manager);
			}
		});

		final Menu menu = menuManager.createContextMenu(gallery);
		gallery.setMenu(menu);
	}

	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionNew);
		final int selectionCount = gallery.getSelectionCount();
		if (selectionCount == 1)
		{
			manager.add(actionEdit);
		}
		if (selectionCount > 0)
		{
			final GalleryItem[] items = gallery.getSelection();
			boolean protectedFound = false;
			for(GalleryItem item : items)
			{
				if (((LibraryImage)item.getData()).isProtected())
				{
					protectedFound = true;
					break;
				}
			}
			if (!protectedFound)
			{
				manager.add(actionDelete);
			}
		}
	}

	private void contributeToActionBars()
	{
		IActionBars actionBars = getViewSite().getActionBars();
		final IToolBarManager toolBarManager = actionBars.getToolBarManager();

		toolBarManager.add(actionNew);
		toolBarManager.add(actionRefresh);
		toolBarManager.add(new Separator());
		toolBarManager.add(actionZoomIn);
		toolBarManager.add(actionZoomOut);
	}

	private void refreshImages() throws NetXMSClientException, IOException
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
		this.knownCategories = categories.keySet();

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
					try
					{
						imageItem.setImage(new Image(Display.getDefault(), stream));
					}
					catch(SWTException e)
					{
						// TODO: log?
						imageItem.setImage(ImageProvider.getInstance().getImage(null)); // show as "missing"
					}
				}
				imageItem.setData(completeImage);
			}
		}

		// if (gallery.getItemCount() > 0)
		// {
		// gallery.getItem(0).setExpanded(true);
		// }
	}
}
