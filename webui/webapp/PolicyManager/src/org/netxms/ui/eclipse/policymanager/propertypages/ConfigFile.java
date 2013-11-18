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
package org.netxms.ui.eclipse.policymanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AgentPolicyConfig;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.policymanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Property page for agent configuration policy
 *
 */
public class ConfigFile extends PropertyPage
{
	private Text textContent;
	private String initialContent;
	private AgentPolicyConfig object;
	
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (AgentPolicyConfig)getElement().getAdapter(AgentPolicyConfig.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		//FormLayout layout = new FormLayout();
      dialogArea.setLayout(layout);
      
		// File content
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("File");
      
      initialContent = new String(object.getFileContent());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.verticalAlignment = GridData.FILL;
      gd.heightHint = 0;
      textContent = new Text(dialogArea, SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL);
      textContent.setText(object.getFileContent());
      textContent.setLayoutData(gd);
      
		return dialogArea;
	}
	
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (textContent.getText().equals(initialContent))
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final String newContent = new String(textContent.getText());
		new ConsoleJob("Change policy", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				if (object != null)
				{
					NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
					md.setConfigFileContent(newContent);
					((NXCSession)ConsoleSharedData.getSession()).modifyObject(md);
				}
				initialContent = newContent;
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					new UIJob("Update \"Configuration File\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							ConfigFile.this.setValid(true);
							return Status.OK_STATUS;
						}
					}.schedule();
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update agent policy";
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
