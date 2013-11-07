/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Layout" page for dashboard element
 */
public class Layout extends PropertyPage
{
	private Combo comboHorizontalAlign;
	private Combo comboVerticalAlign;
	private Button checkGrabHorizontalSpace;
	private Button checkGrabVerticalSpace;
	private Spinner spinnerHorizontalSpan;
	private Spinner spinnerVerticalSpan;
	private Spinner spinnerWidthHint;
	private Spinner spinnerHeightHint;
	private DashboardElementConfig elementConfig;
	private DashboardElementLayout elementLayout;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		elementConfig = (DashboardElementConfig)getElement().getAdapter(DashboardElementConfig.class);
		elementLayout = elementConfig.getLayout();
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		comboHorizontalAlign = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, 
				Messages.Layout_HAlign, WidgetHelper.DEFAULT_LAYOUT_DATA);
		comboHorizontalAlign.add(Messages.Layout_Fill);
		comboHorizontalAlign.add(Messages.Layout_Center);
		comboHorizontalAlign.add(Messages.Layout_Left);
		comboHorizontalAlign.add(Messages.Layout_Right);
		comboHorizontalAlign.select(elementLayout.horizontalAlignment);
		
		comboVerticalAlign = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, 
				Messages.Layout_VAlign, WidgetHelper.DEFAULT_LAYOUT_DATA);
		comboVerticalAlign.add(Messages.Layout_Fill);
		comboVerticalAlign.add(Messages.Layout_Center);
		comboVerticalAlign.add(Messages.Layout_Top);
		comboVerticalAlign.add(Messages.Layout_Bottom);
		comboVerticalAlign.select(elementLayout.vertcalAlignment);
		
		checkGrabHorizontalSpace = new Button(dialogArea, SWT.CHECK);
		checkGrabHorizontalSpace.setText(Messages.Layout_GrapExtraH);
		checkGrabHorizontalSpace.setSelection(elementLayout.grabHorizontalSpace);
		
		checkGrabVerticalSpace = new Button(dialogArea, SWT.CHECK);
		checkGrabVerticalSpace.setText(Messages.Layout_GrapExtraV);
		checkGrabVerticalSpace.setSelection(elementLayout.grabVerticalSpace);
		
		final WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite parent, int style)
			{
				Spinner spinner = new Spinner(parent, style);
				spinner.setMinimum(1);
				spinner.setMaximum(8);
				return spinner;
			}
		};
		
		spinnerHorizontalSpan = (Spinner)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, factory, 
				Messages.Layout_HSpan, WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerHorizontalSpan.setSelection(elementLayout.horizontalSpan);

		spinnerVerticalSpan = (Spinner)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, factory, 
				Messages.Layout_VSpan, WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerVerticalSpan.setSelection(elementLayout.verticalSpan);

		spinnerWidthHint = (Spinner)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, factory, 
				Messages.Layout_WHint, WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerWidthHint.setMinimum(-1);
		spinnerWidthHint.setMaximum(8192);
		spinnerWidthHint.setSelection(elementLayout.widthHint);

		spinnerHeightHint = (Spinner)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, factory, 
				Messages.Layout_HHint, WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerHeightHint.setMinimum(-1);
		spinnerHeightHint.setMaximum(8192);
		spinnerHeightHint.setSelection(elementLayout.heightHint);

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		elementLayout.horizontalAlignment = comboHorizontalAlign.getSelectionIndex();
		elementLayout.vertcalAlignment = comboVerticalAlign.getSelectionIndex();
		elementLayout.grabHorizontalSpace = checkGrabHorizontalSpace.getSelection();
		elementLayout.grabVerticalSpace = checkGrabVerticalSpace.getSelection();
		elementLayout.horizontalSpan = spinnerHorizontalSpan.getSelection();
		elementLayout.verticalSpan = spinnerVerticalSpan.getSelection();
		elementLayout.widthHint = spinnerWidthHint.getSelection();
		elementLayout.heightHint = spinnerHeightHint.getSelection();
		return true;
	}
}
