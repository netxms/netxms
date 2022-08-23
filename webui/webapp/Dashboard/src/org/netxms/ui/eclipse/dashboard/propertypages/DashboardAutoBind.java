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
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
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
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.interfaces.AutoBindObject;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Auto apply" property page for template object
 *
 */
public class DashboardAutoBind extends PropertyPage
{
   private Dashboard object;
	private Button checkboxEnableAdd;
	private Button checkboxEnableRemove;
	private ScriptEditor filterSource;
	private boolean initialBind;
   private boolean initialUnbind;
	private String initialBindFilter;

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

      initialBind = object.isAutoBindEnabled();
      initialUnbind = object.isAutoUnbindEnabled();
      initialBindFilter = object.getAutoBindFilter();

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // Enable/disable check box
      checkboxEnableAdd = new Button(dialogArea, SWT.CHECK);
      checkboxEnableAdd.setText("&Add this dashboard automatically to objects selected by filter");
      checkboxEnableAdd.setSelection(object.isAutoBindEnabled());
      checkboxEnableAdd.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				if (checkboxEnableAdd.getSelection())
				{
					filterSource.setEnabled(true);
					filterSource.setFocus();
					checkboxEnableRemove.setEnabled(true);
				}
				else
				{
					filterSource.setEnabled(false);
					checkboxEnableRemove.setEnabled(false);
				}
			}
      });

      // Enable/disable check box
      checkboxEnableRemove = new Button(dialogArea, SWT.CHECK);
      checkboxEnableRemove.setText("&Remove this dashboard automatically when object no longer passes through filter");
      checkboxEnableRemove.setSelection(object.isAutoUnbindEnabled());
      checkboxEnableRemove.setEnabled(object.isAutoBindEnabled());

      // Filtering script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Filtering script");

      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.DIALOG_SPACING;
		label.setLayoutData(gd);

      filterSource = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, true,
            "Variables:\r\n\t$node\tnode being tested (null if object is not a node).\r\n\t$object\tobject being tested.\r\n\t$dashboard\tthis dashboard object.\r\n\r\nReturn value: true to add this dashboard to node, false to remove, null to make no changes.");
      filterSource.setText(object.getAutoBindFilter());
      filterSource.setEnabled(object.isAutoBindEnabled());

		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.widthHint = 0;
      gd.heightHint = 0;
		filterSource.setLayoutData(gd);

		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
	   boolean apply = checkboxEnableAdd.getSelection();
      boolean remove = checkboxEnableRemove.getSelection();

		if ((apply == initialBind) && (remove == initialUnbind) && initialBindFilter.equals(filterSource.getText()))
			return;		// Nothing to apply

		if (isApply)
			setValid(false);

      final NXCSession session = ConsoleSharedData.getSession();
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setAutoBindFilter(filterSource.getText());
      int flags = object.getAutoBindFlags();
      flags = apply ? flags | AutoBindObject.OBJECT_BIND_FLAG : flags & ~AutoBindObject.OBJECT_BIND_FLAG;  
      flags = remove ? flags | AutoBindObject.OBJECT_UNBIND_FLAG : flags & ~AutoBindObject.OBJECT_UNBIND_FLAG;  
      md.setAutoBindFlags(flags);

      new ConsoleJob("Updating autobind configuration", null, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
            initialBind = apply;
            initialUnbind = remove;
				initialBindFilter = md.getAutoBindFilter();
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
							DashboardAutoBind.this.setValid(true);
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
            return "Cannot update autobind configuration";
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
