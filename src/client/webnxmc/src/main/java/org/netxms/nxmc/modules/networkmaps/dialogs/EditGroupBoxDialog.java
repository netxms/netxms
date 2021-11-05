/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.networkmaps.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetFactory;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for adding group box decoration
 *
 */
public class EditGroupBoxDialog extends Dialog
{
	private static final RGB DEFAULT_COLOR = new RGB(64, 105, 156);

   private I18n i18n = LocalizationHelper.getI18n(EditGroupBoxDialog.class);
	private LabeledText textTitle;
	private Spinner spinnerWidth;
	private Spinner spinnerHeight;
	private ColorSelector colorSelector;
	private NetworkMapDecoration groupBox;
	
	/**
	 * 
	 * @param parentShell
	 */
	public EditGroupBoxDialog(Shell parentShell, NetworkMapDecoration groupBox)
	{
      super(parentShell);
	   this.groupBox = groupBox;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Group Box Properties"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		dialogArea.setLayout(layout);
		
		/* title */
		textTitle = new LabeledText(dialogArea, SWT.NONE);
      textTitle.setLabel(i18n.tr("Title"));
		textTitle.setText(groupBox.getTitle());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		textTitle.setLayoutData(gd);
		
		/* width and height */
		Composite attrArea = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.numColumns = 3;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
		attrArea.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		attrArea.setLayoutData(gd);
		
		WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite parent, int style)
			{
				Spinner spinner = new Spinner(parent, style | SWT.BORDER);
				spinner.setMinimum(20);
				spinner.setMaximum(4096);
				return spinner;
			}
		};
      spinnerWidth = (Spinner)WidgetHelper.createLabeledControl(attrArea, SWT.NONE, factory, i18n.tr("Width"), WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerWidth.setSelection(groupBox.getWidth());
      spinnerHeight = (Spinner)WidgetHelper.createLabeledControl(attrArea, SWT.NONE, factory, i18n.tr("Height"), WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerHeight.setSelection(groupBox.getHeight());
		
      colorSelector = WidgetHelper.createLabeledColorSelector(attrArea, i18n.tr("Color"), WidgetHelper.DEFAULT_LAYOUT_DATA);
		colorSelector.setColorValue(DEFAULT_COLOR);
		
		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		groupBox.setTitle(textTitle.getText());
		groupBox.setSize(spinnerWidth.getSelection(), spinnerHeight.getSelection());
		groupBox.setColor(ColorConverter.rgbToInt(colorSelector.getColorValue()));
		super.okPressed();
	}
}
