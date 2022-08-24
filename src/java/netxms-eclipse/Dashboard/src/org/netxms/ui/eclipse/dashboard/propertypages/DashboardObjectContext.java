/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;

/**
 * "Object view" property page for dashboard object
 */
public class DashboardObjectContext extends PropertyPage
{
   private Dashboard object;
   private Button checkboxShowAsObjectView;
   private LabeledSpinner displayPriority;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);
		
      object = (Dashboard)getElement().getAdapter(Dashboard.class);
		if (object == null)	// Paranoid check
			return dialogArea;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // Enable/disable check box
      checkboxShowAsObjectView = new Button(dialogArea, SWT.CHECK);
      checkboxShowAsObjectView.setText("&Show this dashboard in object context");
      checkboxShowAsObjectView.setSelection((object.getFlags() & Dashboard.SHOW_AS_OBJECT_VIEW) != 0);
      checkboxShowAsObjectView.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false));

      displayPriority = new LabeledSpinner(dialogArea, SWT.NONE);
      displayPriority.setLabel("Display priority (1-65535, 0 for automatic)");
      displayPriority.setRange(0, 65535);
      displayPriority.setSelection(object.getDisplayPriority());
      displayPriority.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false));

		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);

		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setObjectFlags(checkboxShowAsObjectView.getSelection() ? Dashboard.SHOW_AS_OBJECT_VIEW : 0, Dashboard.SHOW_AS_OBJECT_VIEW);
      md.setDisplayPriority(displayPriority.getSelection());

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Updating dashboard object context configuration", null, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							DashboardObjectContext.this.setValid(true);
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
            return "Cannot update dashboard object context configuration";
			}
		}.start();
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
