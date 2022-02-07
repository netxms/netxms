/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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

import java.io.File;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetFactory;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Library image properties
 */
public class ImagePropertiesDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(ImagePropertiesDialog.class);

	private String name;
	private String category;
	private String fileName;

	private Text fileNameInputField;
	private Text nameInputField;
	private Combo categoryCombo;
	private Set<String> categories;

	/**
	 * @param parentShell
	 * @param knownCategories
	 */
	public ImagePropertiesDialog(Shell parentShell, Set<String> knownCategories)
	{
		super(parentShell);
		this.categories = knownCategories;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		final Composite dialogArea = (Composite)super.createDialogArea(parent);

		final GridLayout layout = new GridLayout();
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		dialogArea.setLayout(layout);

		WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, new WidgetFactory() {
			@Override
			public Control createControl(Composite parent, int style)
			{
				final Composite composite = new Composite(parent, SWT.NONE);
				final GridLayout compositeLayout = new GridLayout();
				compositeLayout.numColumns = 2;
				composite.setLayout(compositeLayout);
				fileNameInputField = new Text(composite, SWT.SINGLE | SWT.READ_ONLY | SWT.BORDER);
				GridData gd = new GridData();
				gd.widthHint = 300;
				gd.grabExcessHorizontalSpace = true;
				gd.grabExcessVerticalSpace = true;
				gd.horizontalAlignment = SWT.FILL;
				gd.verticalAlignment = SWT.FILL;
				fileNameInputField.setLayoutData(gd);

				final Button button = new Button(composite, SWT.NONE);
				button.addSelectionListener(new SelectionListener() {
					@Override
					public void widgetSelected(SelectionEvent e)
					{
						FileDialog dialog = new FileDialog(getShell(), SWT.OPEN);
                  dialog.setText("Image Properties");
                  WidgetHelper.setFileDialogFilterExtensions(dialog, new String[] { i18n.tr("Image files"), i18n.tr("All files") });
                  dialog.setFilterExtensions(new String[] { "*.jpg;*.jpeg;*.png;*.bmp", "*.*" });
						final String selectedFile = dialog.open();
						if (selectedFile != null)
						{
							fileName = selectedFile;
							fileNameInputField.setText(selectedFile);

							if (nameInputField.getText().length() == 0)
							{
								String imageName = new File(fileName).getName();
								final int lastIndexOf = imageName.lastIndexOf('.');
								if (lastIndexOf != -1)
								{
									imageName = imageName.substring(0, lastIndexOf);
								}
								nameInputField.setText(imageName);
							}
						}
					}

					@Override
					public void widgetDefaultSelected(SelectionEvent e)
					{
						widgetSelected(e);
					}
				});
				button.setText("..."); //$NON-NLS-1$
				return composite;
			}
      }, i18n.tr("Image file"), WidgetHelper.DEFAULT_LAYOUT_DATA);

      nameInputField = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Image name"),
				name == null ? "" : name, WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$
		nameInputField.getShell().setMinimumSize(300, 0);

      categoryCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.BORDER, i18n.tr("Category"),
				WidgetHelper.DEFAULT_LAYOUT_DATA);
		categoryCombo.getShell().setMinimumSize(300, 0);

		int counter = 0;
		for(String name : categories)
		{
			categoryCombo.add(name);
			if (name.equals(category))
			{
				categoryCombo.select(counter);
			}
			counter++;
		}

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		category = categoryCombo.getText();
		name = nameInputField.getText();
		super.okPressed();
	}

   /**
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Image Properties"));
	}

	public void setDefaultCategory(String category)
	{
		this.category = category;
	}

	public String getCategory()
	{
		return category;
	}

	public String getName()
	{
		return name;
	}

	public void setName(String name)
	{
		this.name = name;
	}

	public String getFileName()
	{
		return fileName;
	}
}
