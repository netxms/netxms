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
package org.netxms.nxmc.modules.imagelibrary.views;

import java.io.File;
import java.io.FileInputStream;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.base.NXCommon;
import org.netxms.client.LibraryImage;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.dialogs.ImagePropertiesDialog;
import org.netxms.nxmc.modules.imagelibrary.views.helpers.ImageCategory;
import org.netxms.nxmc.modules.imagelibrary.views.helpers.ImageLibraryContentProvider;
import org.netxms.nxmc.modules.imagelibrary.views.helpers.ImageLibraryLabelProvider;
import org.netxms.nxmc.modules.imagelibrary.widgets.ImagePreview;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Image library configurator
 */
public class ImageLibrary extends ConfigurationView
{
   private static final Logger logger = LoggerFactory.getLogger(ImageLibrary.class);

   private final I18n i18n = LocalizationHelper.getI18n(ImageLibrary.class);

	public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_PROTECTED = 2;
   public static final int COLUMN_GUID = 3;

   private NXCSession session = Registry.getSession();
   private SessionListener sessionListener;
	private Map<String, ImageCategory> imageCategories = new HashMap<String, ImageCategory>();
	private SashForm splitter;
	private SortableTreeViewer viewer;
	private ImagePreview imagePreview;
   private Action actionNew;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionCopy;
   private Action actionSave;

   /**
    * Create image library view
    */
   public ImageLibrary()
   {
      super(LocalizationHelper.getI18n(ImageLibrary.class).tr("Image Library"), ResourceManager.getImageDescriptor("icons/config-views/image_library.png"), "ImageLibrary", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   public void createContent(final Composite parent)
	{
		splitter = new SashForm(parent, SWT.HORIZONTAL);

      final String[] names = { i18n.tr("Name"), i18n.tr("MIME type"), i18n.tr("Protected"), i18n.tr("GUID") };
		final int[] widths = { 400, 120, 80, 200 };
		viewer = new SortableTreeViewer(splitter, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION | SWT.BORDER);
		viewer.setContentProvider(new ImageLibraryContentProvider());
		viewer.setLabelProvider(new ImageLibraryLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            String n1 = (e1 instanceof LibraryImage) ? ((LibraryImage)e1).getName() : ((ImageCategory)e1).getName();
            String n2 = (e2 instanceof LibraryImage) ? ((LibraryImage)e2).getName() : ((ImageCategory)e2).getName();
            return n1.compareToIgnoreCase(n2);
         }
      });

      viewer.addDoubleClickListener((e) -> {
         IStructuredSelection selection = viewer.getStructuredSelection();
         if (selection.getFirstElement() instanceof ImageCategory)
         {
            if (viewer.getExpandedState(selection.getFirstElement()))
               viewer.collapseToLevel(selection.getFirstElement(), TreeViewer.ALL_LEVELS);
            else
               viewer.expandToLevel(selection.getFirstElement(), 1);
         }
         else if (selection.getFirstElement() instanceof LibraryImage)
         {
            editImage();
         }
      });
      viewer.addSelectionChangedListener((e) -> {
         IStructuredSelection selection = viewer.getStructuredSelection();
         if ((selection.size() == 1) && (selection.getFirstElement() instanceof LibraryImage))
         {
            imagePreview.setImage((LibraryImage)selection.getFirstElement());
            actionEdit.setEnabled(true);
            actionCopy.setEnabled(true);
            actionSave.setEnabled(true);
         }
         else
         {
            imagePreview.setImage(null);
            actionEdit.setEnabled(false);
            actionCopy.setEnabled(false);
            actionSave.setEnabled(false);
         }
      });
		
		imagePreview = new ImagePreview(splitter, SWT.BORDER);
		
		splitter.setWeights(new int[] { 70, 30 });

		createActions();
		createContextMenu();

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() != SessionNotification.IMAGE_LIBRARY_CHANGED)
               return;

            final UUID guid = (UUID)n.getObject();
            final boolean update = (n.getSubCode() == SessionNotification.IMAGE_UPDATED);
            parent.getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  if (update)
                  {
                     updateImageLocalCopy(guid);
                  }
                  else
                  {
                     removeImageLocalCopy(guid);
                  }
               }
            });
         }
      };
      session.addListener(sessionListener);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(sessionListener);
      super.dispose();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
	@Override
	public void setFocus()
	{
	   viewer.getControl().setFocus();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionNew = new Action(i18n.tr("&Upload new image..."), SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
			   addImage();
			}
		};

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
			@Override
			public void run()
			{
			   editImage();
			}
		};

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteImages();
			}
		};

      actionCopy = new Action(i18n.tr("&Copy"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyImage();
         }
      };

      actionSave = new Action(i18n.tr("&Save as..."), SharedIcons.SAVE_AS) {
         @Override
         public void run()
         {
            saveImage();
         }
      };
	}

   /**
    * Verify if image can be created from given file. On success returns image MIME type.
    */
   private String verifyImageFile(final String fileName) throws Exception
	{
      ImageData image = new ImageData(fileName);
		String mimeType;
      switch(image.type)
      {
         case SWT.IMAGE_BMP:
         case SWT.IMAGE_BMP_RLE:
            mimeType = "image/bmp";
            break;
         case SWT.IMAGE_GIF:
            mimeType = "image/gif";
            break;
         case SWT.IMAGE_ICO:
            mimeType = "image/x-icon";
            break;
         case SWT.IMAGE_JPEG:
            mimeType = "image/jpeg";
            break;
         case SWT.IMAGE_PNG:
            mimeType = "image/png";
            break;
         case SWT.IMAGE_TIFF:
            mimeType = "image/tiff";
            break;
         default:
            mimeType = "image/unknown";
            break;
      }
      return mimeType;
	}
	
	/**
	 * Add new image
	 */
	private void addImage()
	{
      final ImagePropertiesDialog dialog = new ImagePropertiesDialog(getWindow().getShell(), imageCategories.keySet());

      IStructuredSelection selection = viewer.getStructuredSelection();
	   if (!selection.isEmpty())
      {
	      Object o = selection.getFirstElement();
	      if (o instanceof ImageCategory)
	         dialog.setDefaultCategory(((ImageCategory)o).getName());
	      else if (o instanceof LibraryImage)
            dialog.setDefaultCategory(((LibraryImage)o).getCategory());
      }

      if (dialog.open() == Dialog.OK)
      {
         uploadNewImage(dialog.getName(), dialog.getCategory(), dialog.getFileName());
      }
	}
	
	/**
	 * Edit existing image
	 */
	private void editImage()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      if (selection.getFirstElement() instanceof LibraryImage)
      {
         LibraryImage image = (LibraryImage)selection.getFirstElement();
         final ImagePropertiesDialog dialog = new ImagePropertiesDialog(getWindow().getShell(), imageCategories.keySet());
         dialog.setName(image.getName());
         dialog.setDefaultCategory(image.getCategory());
         if (dialog.open() == Dialog.OK)
         {
            doImageUpdate(image, dialog.getName(), dialog.getCategory(), dialog.getFileName());
         }
      }
	}

   /**
    * Copy selected image to clipboard
    */
   private void copyImage()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      if (selection.getFirstElement() instanceof LibraryImage)
      {
         LibraryImage imageDescriptor = (LibraryImage)selection.getFirstElement();
         Image image = ImagePreview.createImageFromDescriptor(getDisplay(), imageDescriptor);
         if (image != null)
         {
            WidgetHelper.copyToClipboard(image);
            image.dispose();
         }
      }
   }

   /**
    * Save selected image to file
    */
   private void saveImage()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      if (selection.getFirstElement() instanceof LibraryImage)
      {
         LibraryImage imageDescriptor = (LibraryImage)selection.getFirstElement();
         Image image = ImagePreview.createImageFromDescriptor(getDisplay(), imageDescriptor);
         if (image != null)
         {
            WidgetHelper.saveImageToFile(this, imageDescriptor.getName() + ".png", image);
            image.dispose();
         }
      }
   }

	/**
	 * @param galleryItem
	 * @param name
	 * @param category
	 * @param fileName
	 */
	private void doImageUpdate(final LibraryImage image, final String name, final String category, final String fileName)
	{
      if (!image.isProtected())
      {
         image.setName(name);
         image.setCategory(category);
      }

      new Job(i18n.tr("Update image"), this) {
			@Override
         protected void run(final IProgressMonitor monitor) throws Exception
			{
				if (fileName != null)
				{
               String mimeType = verifyImageFile(fileName);

					FileInputStream stream = null;
					try
					{
						final long fileSize = new File(fileName).length();
						stream = new FileInputStream(fileName);
						byte imageData[] = new byte[(int)fileSize];
						stream.read(imageData);
                  image.setBinaryData(imageData, mimeType);
					}
					finally
					{
						if (stream != null)
							stream.close();
					}
				}

				session.modifyImage(image, new ProgressListener() {
					private long prevDone = 0;

					@Override
					public void setTotalWorkAmount(long workTotal)
					{
                  monitor.beginTask(i18n.tr("Update image"), (int)workTotal);
					}

					@Override
					public void markProgress(long workDone)
					{
						monitor.worked((int)(workDone - prevDone));
						prevDone = workDone;
					}
				});

				monitor.done();
				
				runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.refresh();
               }
            });
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot update image");
			}
		}.start();
	}

	/**
	 * @param name
	 * @param category
	 * @param fileName
	 */
	protected void uploadNewImage(final String name, final String category, final String fileName)
	{
      new Job(i18n.tr("Upload image"), this) {
			@Override
         protected void run(final IProgressMonitor monitor) throws Exception
			{
            String mimeType = verifyImageFile(fileName);

            final LibraryImage image = new LibraryImage(UUID.randomUUID(), name, category, mimeType);

				final long fileSize = new File(fileName).length();
				FileInputStream stream = null;
				try
				{
					stream = new FileInputStream(fileName);
					byte imageData[] = new byte[(int)fileSize];
					stream.read(imageData);
               image.setBinaryData(imageData, mimeType);
				}
				finally
				{
					if (stream != null)
						stream.close();
				}

				session.createImage(image, new ProgressListener() {
					private long prevDone = 0;

					@Override
					public void setTotalWorkAmount(long workTotal)
					{
                  monitor.beginTask(i18n.tr("Upload image"), (int)workTotal);
					}

					@Override
					public void markProgress(long workDone)
					{
						monitor.worked((int)(workDone - prevDone));
						prevDone = workDone;
					}
				});

				monitor.done();

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  ImageCategory category = imageCategories.get(image.getCategory());
                  if (category == null)
                  {
                     category = new ImageCategory(image.getCategory());
                     imageCategories.put(category.getName(), category);
                  }
                  category.addImage(image);
                  viewer.refresh();
                  viewer.setSelection(new StructuredSelection(image));
               }
            });
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot upload image");
			}
		}.start();
	}

	/**
	 * Delete selected images
	 */
	protected void deleteImages()
	{
	   IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
	   if (selection.isEmpty())
	      return;
	   
      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Library Images"), i18n.tr("Do you really want to delete selected images?")))
	      return;

	   final Set<String> categoryDeleteList = new HashSet<String>();
	   final Set<LibraryImage> deleteList = new HashSet<LibraryImage>();
		for(Object o : selection.toList())
		{
		   if (o instanceof LibraryImage)
		   {
		      deleteList.add((LibraryImage)o);
		   }
		   else if (o instanceof ImageCategory)
		   {
		      deleteList.addAll(((ImageCategory)o).getImages());
		      categoryDeleteList.add(((ImageCategory)o).getName());
		   }
		}
		
		if (deleteList.isEmpty())
		   return;
		
      new Job(i18n.tr("Delete library images"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(LibraryImage i : deleteList)
               session.deleteImage(i);
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  for(LibraryImage i : deleteList)
                  {
                     imageCategories.get(i.getCategory()).removeImage(i.getGuid());
                  }
                  if (!categoryDeleteList.isEmpty())
                  {
                     for(String c : categoryDeleteList)
                        imageCategories.remove(c);
                  }
                  viewer.refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete library image");
         }
      }.start();
	}

	/**
    * Create viewer context menu
    */
	private void createContextMenu()
	{
		MenuManager menuManager = new MenuManager();
		menuManager.setRemoveAllWhenShown(true);
      menuManager.addMenuListener((m) -> fillContextMenu(m));

		final Menu menu = menuManager.createContextMenu(viewer.getTree());
		viewer.getTree().setMenu(menu);
	}

	/**
	 * @param manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionNew);
      IStructuredSelection selection = viewer.getStructuredSelection();
		if ((selection.size() == 1) && (selection.getFirstElement() instanceof LibraryImage))
		{
			manager.add(actionEdit);
		}
		if (!selection.isEmpty())
		{
			boolean protectedFound = false;
			for(Object o : selection.toList())
			{
				if ((o instanceof LibraryImage) && ((LibraryImage)o).isProtected())
				{
					protectedFound = true;
					break;
				}
            if ((o instanceof ImageCategory) && ((ImageCategory)o).hasProtectedImages())
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
      if ((selection.size() == 1) && (selection.getFirstElement() instanceof LibraryImage))
      {
         manager.add(new Separator());
         manager.add(actionCopy);
         manager.add(actionSave);
      }
	}

	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      super.fillLocalToolBar(manager);
      manager.add(actionNew);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
	{
      new Job(i18n.tr("Reload image library"), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final List<LibraryImage> imageLibrary = session.getImageLibrary();
				Collections.sort(imageLibrary);
				for(int i = 0; i < imageLibrary.size(); i++)
				{
					LibraryImage image = imageLibrary.get(i);
					if (!image.isComplete())
					{
						try
						{
							LibraryImage completeImage = session.getImage(image.getGuid());
							imageLibrary.set(i, completeImage);
						}
						catch(Exception e)
						{
                     logger.error("Exception in ImageLibrary.refreshImages()", e);
						}
					}
				}
            runInUIThread(() -> {
               if (!viewer.getControl().isDisposed())
                  refreshUI(imageLibrary);
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot load image library");
			}

		}.start();
	}

	/**
	 * Refresh UI after retrieving imaqe library from server
	 */
	private void refreshUI(List<LibraryImage> imageLibrary)
	{
		imageCategories.clear();
		for(LibraryImage image : imageLibrary)
		{
			ImageCategory category = imageCategories.get(image.getCategory());
			if (category == null)
			{
			   category = new ImageCategory(image.getCategory());
			   imageCategories.put(category.getName(), category);
			}
			category.addImage(image);
		}
		viewer.setInput(imageCategories);
	}

   /**
    * Update local copy of library image
    * 
    * @param guid
    */
   private void updateImageLocalCopy(final UUID guid)
   {
      Job job = new Job(i18n.tr("Load library image"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final LibraryImage image = session.getImage(guid);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
                  UUID selectedGuid = ((selection.size() == 1) && (selection.getFirstElement() instanceof LibraryImage))
                        ? ((LibraryImage)selection.getFirstElement()).getGuid()
                        : NXCommon.EMPTY_GUID;

                  ImageCategory category = imageCategories.get(image.getCategory());
                  if (category == null)
                  {
                     category = new ImageCategory(image.getCategory());
                     imageCategories.put(category.getName(), category);
                  }
                  category.addImage(image);
                  viewer.refresh();

                  if (selectedGuid.equals(image.getGuid()))
                     viewer.setSelection(new StructuredSelection(image));
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load library image");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Remove local copy of library image
    * 
    * @param guid
    */
   private void removeImageLocalCopy(UUID guid)
   {
      for(ImageCategory c : imageCategories.values())
      {
         if (c.removeImage(guid))
         {
            viewer.refresh();
            break;
         }
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}
