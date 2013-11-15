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
package org.netxms.ui.eclipse.networkmaps.dialogs;

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
import org.netxms.ui.eclipse.networkmaps.Messages;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for adding group box decoration
 *
 */
public class AddGroupBoxDialog extends Dialog
{
	private static final RGB DEFAULT_COLOR = new RGB(64, 105, 156);
	
	private String title;
	private int width;
	private int height;
	private RGB color;
	
	private LabeledText textTitle;
	private Spinner spinnerWidth;
	private Spinner spinnerHeight;
	private ColorSelector colorSelector;
	
	/**
	 * 
	 * @param parentShell
	 */
	public AddGroupBoxDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().AddGroupBoxDialog_DialogTitle);
	}

	/* (non-Javadoc)
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
		textTitle.setLabel(Messages.get().AddGroupBoxDialog_Title);
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
		spinnerWidth = (Spinner)WidgetHelper.createLabeledControl(attrArea, SWT.NONE, factory, Messages.get().AddGroupBoxDialog_Width, WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerWidth.setSelection(250);
		spinnerHeight = (Spinner)WidgetHelper.createLabeledControl(attrArea, SWT.NONE, factory, Messages.get().AddGroupBoxDialog_Height, WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerHeight.setSelection(100);
		
		colorSelector = WidgetHelper.createLabeledColorSelector(attrArea, Messages.get().AddGroupBoxDialog_Color, WidgetHelper.DEFAULT_LAYOUT_DATA);
		colorSelector.setColorValue(DEFAULT_COLOR);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		title = textTitle.getText();
		width = spinnerWidth.getSelection();
		height = spinnerHeight.getSelection();
		color = colorSelector.getColorValue();
		super.okPressed();
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @return the width
	 */
	public int getWidth()
	{
		return width;
	}

	/**
	 * @return the height
	 */
	public int getHeight()
	{
		return height;
	}

	/**
	 * @return the color
	 */
	public RGB getColor()
	{
		return color;
	}
}
