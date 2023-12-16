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
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
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
   private Button checkShowOnNarrowScreen;
   private LabeledSpinner spinnerNSOrder;
   private LabeledSpinner spinnerNSHeightHint;
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
      checkGrabVerticalSpace.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false, 2, 1));

      Label separator = new Label(dialogArea, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false, 2, 1));

      checkShowOnNarrowScreen = new Button(dialogArea, SWT.CHECK);
      checkShowOnNarrowScreen.setText("Visible in narrow screen mode");
      checkShowOnNarrowScreen.setSelection(elementLayout.showInNarrowScreenMode);
      checkShowOnNarrowScreen.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false, 2, 1));
      checkShowOnNarrowScreen.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            boolean checked = checkShowOnNarrowScreen.getSelection();
            spinnerNSOrder.setEnabled(checked);
            spinnerNSHeightHint.setEnabled(checked);
         }
      });

      spinnerNSOrder = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerNSOrder.setLabel("Display order in narrow screen mode");
      spinnerNSOrder.setRange(0, 255);
      spinnerNSOrder.setSelection(elementLayout.narrowScreenOrder);
      spinnerNSOrder.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));
      spinnerNSOrder.setEnabled(elementLayout.showInNarrowScreenMode);

      spinnerNSHeightHint = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerNSHeightHint.setLabel("Height hint for narrow screen mode (-1 to ignore)");
      spinnerNSHeightHint.setRange(-1, 8192);
      spinnerNSHeightHint.setSelection(elementLayout.narrowScreenHeightHint);
      spinnerNSHeightHint.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));
      spinnerNSHeightHint.setEnabled(elementLayout.showInNarrowScreenMode);

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
      elementLayout.showInNarrowScreenMode = checkShowOnNarrowScreen.getSelection();
      elementLayout.narrowScreenOrder = spinnerNSOrder.getSelection();
      elementLayout.narrowScreenHeightHint = spinnerNSHeightHint.getSelection();
      return true;
   }
}
