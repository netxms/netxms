/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.imagelibrary.dialogs;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.NXCommon;
import org.netxms.client.LibraryImage;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.modules.imagelibrary.ImageUpdateListener;
import org.netxms.nxmc.modules.imagelibrary.views.helpers.ImageCategory;
import org.netxms.nxmc.modules.imagelibrary.views.helpers.ImageLibraryContentProvider;
import org.netxms.nxmc.modules.imagelibrary.views.helpers.ImageLibraryLabelProvider;
import org.netxms.nxmc.modules.imagelibrary.widgets.ImagePreview;
import org.xnap.commons.i18n.I18n;

/**
 * Image selection dialog
 */
public class ImageSelectionDialog extends Dialog implements ImageUpdateListener
{
	private static final String SELECT_IMAGE_CY = "SelectImage.cy"; //$NON-NLS-1$
	private static final String SELECT_IMAGE_CX = "SelectImage.cx"; //$NON-NLS-1$
	private static final int DEFAULT_ID = IDialogConstants.CLIENT_ID + 1;

   private I18n i18n = LocalizationHelper.getI18n(ImageSelectionDialog.class);
   private Map<String, ImageCategory> imageCategories = new HashMap<String, ImageCategory>();
	private TreeViewer viewer;
	private ImagePreview imagePreview;
   private LibraryImage image;
   private boolean allowDefault;

	/**
	 * Construct new dialog with default flags: single selection, no "default"
	 * button, no null images on OK button
	 * 
	 * @param parentShell
	 */
	public ImageSelectionDialog(Shell parentShell)
	{
      this(parentShell, false);
	}

	/**
    * Construct new Image selection dialog
    * 
    * @param parentShell
    * @param allowDefault control "Default" button appearance (true to enable)
    */
   public ImageSelectionDialog(final Shell parentShell, boolean allowDefault)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
      this.allowDefault = allowDefault;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Select Image"));
      PreferenceStore settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsInteger(SELECT_IMAGE_CX, 700), settings.getAsInteger(SELECT_IMAGE_CY, 400));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

		viewer = new TreeViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
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
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		viewer.getControl().setLayoutData(gd);
		
		imagePreview = new ImagePreview(dialogArea, SWT.BORDER);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.widthHint = 300;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		imagePreview.setLayoutData(gd);
		
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            LibraryImage image = (!selection.isEmpty() && (selection.getFirstElement() instanceof LibraryImage)) ? (LibraryImage)selection.getFirstElement() : null;
            imagePreview.setImage(image);
            getButton(IDialogConstants.OK_ID).setEnabled(image != null);
         }
      });

		ImageProvider.getInstance().addUpdateListener(this);
      dialogArea.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            ImageProvider.getInstance().removeUpdateListener(ImageSelectionDialog.this);
         }
      });

		refreshImages();

      return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
	@Override
	protected void cancelPressed()
	{
      saveSettings();
		super.cancelPressed();
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty() || !(selection.getFirstElement() instanceof LibraryImage))
         return;

      image = (LibraryImage)selection.getFirstElement();

		saveSettings();
		super.okPressed();
	}

   /**
    * "Default" button handler
    */
	private void defaultPressed()
	{
      image = new LibraryImage();
		saveSettings();
		super.okPressed();
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#buttonPressed(int)
    */
	@Override
	protected void buttonPressed(int buttonId)
	{
		super.buttonPressed(buttonId);
		if (DEFAULT_ID == buttonId)
		{
			defaultPressed();
		}
	}

   /**
    * Save dialog settings
    */
	private void saveSettings()
	{
		Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(SELECT_IMAGE_CX, size.x);
      settings.set(SELECT_IMAGE_CY, size.y);
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected void createButtonsForButtonBar(Composite parent)
	{
      if (allowDefault)
		{
         createButton(parent, DEFAULT_ID, i18n.tr("&Default"), false);
		}
		super.createButtonsForButtonBar(parent);
		getButton(IDialogConstants.OK_ID).setEnabled(false);
	}

	/**
    * Refresh image list
    */
	private void refreshImages()
	{
		final ImageProvider provider = ImageProvider.getInstance();
		final List<LibraryImage> imageLibrary = provider.getImageLibrary();

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
    * Get GUID of selected library image
    * 
    * @return GUID of selected library image
    */
	public UUID getImageGuid()
	{
      return (image == null) ? NXCommon.EMPTY_GUID : image.getGuid();
	}

	/**
    * Get selected library image
    * 
    * @return selected library image
    */
   public LibraryImage getImage()
	{
      return image;
	}

   /**
    * @see org.netxms.ui.eclipse.imagelibrary.shared.ImageUpdateListener#imageUpdated(java.util.UUID)
    */
	@Override
   public void imageUpdated(final UUID guid)
	{
      if (getShell().isDisposed())
         return;

      final LibraryImage image = ImageProvider.getInstance().getLibraryImageObject(guid);
      if (image != null)
      {
         IStructuredSelection selection = viewer.getStructuredSelection();
         UUID selectedGuid = ((selection.size() == 1) && (selection.getFirstElement() instanceof LibraryImage)) ? ((LibraryImage)selection.getFirstElement()).getGuid() : NXCommon.EMPTY_GUID;

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
	}
}
