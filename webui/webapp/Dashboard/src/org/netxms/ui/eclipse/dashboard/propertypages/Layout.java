/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;

/**
 * "Layout" page for dashboard element
 */
public class Layout extends PropertyPage
{
	private Button checkGrabVerticalSpace;
   private LabeledSpinner spinnerHorizontalSpan;
   private LabeledSpinner spinnerVerticalSpan;
   private LabeledSpinner spinnerHeightHint;
	private DashboardElementConfig elementConfig;
	private DashboardElementLayout elementLayout;

   /**
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
		
      spinnerHorizontalSpan = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerHorizontalSpan.setLabel(Messages.get().Layout_HSpan);
      spinnerHorizontalSpan.setRange(1, 128);
      spinnerHorizontalSpan.setSelection(elementLayout.horizontalSpan);
      spinnerHorizontalSpan.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      spinnerHeightHint = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerHeightHint.setLabel(Messages.get().Layout_HHint);
      spinnerHeightHint.setRange(-1, 8192);
      spinnerHeightHint.setSelection(elementLayout.heightHint);
      spinnerHeightHint.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      spinnerVerticalSpan = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerVerticalSpan.setLabel(Messages.get().Layout_VSpan);
      spinnerVerticalSpan.setRange(1, 128);
      spinnerVerticalSpan.setSelection(elementLayout.verticalSpan);
      spinnerVerticalSpan.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      checkGrabVerticalSpace = new Button(dialogArea, SWT.CHECK);
      checkGrabVerticalSpace.setText(Messages.get().Layout_GrapExtraV);
      checkGrabVerticalSpace.setSelection(elementLayout.grabVerticalSpace);
      checkGrabVerticalSpace.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false, 1, 1));

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		elementLayout.grabVerticalSpace = checkGrabVerticalSpace.getSelection();
		elementLayout.horizontalSpan = spinnerHorizontalSpan.getSelection();
		elementLayout.verticalSpan = spinnerVerticalSpan.getSelection();
		elementLayout.heightHint = spinnerHeightHint.getSelection();
		return true;
	}
}
