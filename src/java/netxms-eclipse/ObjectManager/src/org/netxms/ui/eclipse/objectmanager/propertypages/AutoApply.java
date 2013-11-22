/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Template;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Auto apply" property page for template object
 *
 */
public class AutoApply extends PropertyPage
{
	private Template object;
	private Button checkboxEnableApply;
	private Button checkboxEnableRemove;
	private ScriptEditor filterSource;
	private int initialFlags;
	private String initialApplyFilter;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (Template)getElement().getAdapter(Template.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		initialFlags = object.getFlags();
		initialApplyFilter = object.getAutoApplyFilter();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // Enable/disable check box
      checkboxEnableApply = new Button(dialogArea, SWT.CHECK);
      checkboxEnableApply.setText(Messages.AutoApply_AutoApply);
      checkboxEnableApply.setSelection(object.isAutoApplyEnabled());
      checkboxEnableApply.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				if (checkboxEnableApply.getSelection())
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
      checkboxEnableRemove.setText(Messages.AutoApply_AutoRemove);
      checkboxEnableRemove.setSelection(object.isAutoRemoveEnabled());
      checkboxEnableRemove.setEnabled(object.isAutoApplyEnabled());
      
      // Filtering script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(Messages.AutoApply_Script);

      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.DIALOG_SPACING;
		label.setLayoutData(gd);
      
      filterSource = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL);
		filterSource.setText(object.getAutoApplyFilter());
		filterSource.setEnabled(object.isAutoApplyEnabled());
		
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
		int flags = object.getFlags();
		if (checkboxEnableApply.getSelection())
			flags |= Template.TF_AUTO_APPLY;
		else
			flags &= ~Template.TF_AUTO_APPLY;
		if (checkboxEnableRemove.getSelection())
			flags |= Template.TF_AUTO_REMOVE;
		else
			flags &= ~Template.TF_AUTO_REMOVE;
			
		if ((flags == initialFlags) && initialApplyFilter.equals(filterSource.getText()))
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setAutoBindFilter(filterSource.getText());
		md.setObjectFlags(flags);
		
		new ConsoleJob(Messages.AutoApply_JobName, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
				initialFlags = md.getObjectFlags();
				initialApplyFilter = md.getAutoBindFilter();
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
							AutoApply.this.setValid(true);
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.AutoApply_JobError;
			}
		}.start();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
