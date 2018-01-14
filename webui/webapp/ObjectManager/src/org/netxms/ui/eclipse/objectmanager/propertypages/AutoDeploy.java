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
import org.netxms.client.objects.AgentPolicy;
import org.netxms.client.objects.Container;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Auto Deploy" property page
 *
 */
public class AutoDeploy extends PropertyPage
{
	private AgentPolicy object;
	private Button checkboxEnableDeploy;
	private Button checkboxEnableUninstall;
	private ScriptEditor filterSource;
	private int initialFlags;
	private String initialAutoDeployFilter;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (AgentPolicy)getElement().getAdapter(AgentPolicy.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		initialFlags = object.getFlags();
		initialAutoDeployFilter = object.getAutoDeployFilter();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // Enable/disable check box
      checkboxEnableDeploy = new Button(dialogArea, SWT.CHECK);
      checkboxEnableDeploy.setText("Deploy this policy automatically to nodes selected by filter");
      checkboxEnableDeploy.setSelection(object.isAutoDeployEnabled());
      checkboxEnableDeploy.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				if (checkboxEnableDeploy.getSelection())
				{
					filterSource.setEnabled(true);
					filterSource.setFocus();
					checkboxEnableUninstall.setEnabled(true);
				}
				else
				{
					filterSource.setEnabled(false);
					checkboxEnableUninstall.setEnabled(false);
				}
			}
      });
      
      checkboxEnableUninstall = new Button(dialogArea, SWT.CHECK);
      checkboxEnableUninstall.setText("Uninstall this policy automatically when node no longer passes through filter");
      checkboxEnableUninstall.setSelection(object.isAutoUninstallEnabled());
      checkboxEnableUninstall.setEnabled(object.isAutoDeployEnabled());
      
      // Filtering script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(Messages.get().AutoBind_Script);

      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.DIALOG_SPACING;
		label.setLayoutData(gd);
      
      filterSource = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, true, "Variables:\r\n\t$node\tnode being tested (null if object is not a node).\r\n\t$object\tobject being tested.\r\n\r\nReturn value: true to deploy this policy onto node, false to uninstall, null to make no changes.");
		filterSource.setText(object.getAutoDeployFilter());
		filterSource.setEnabled(object.isAutoDeployEnabled());
		
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
		if (checkboxEnableDeploy.getSelection())
			flags |= AgentPolicy.PF_AUTO_DEPLOY;
		else
			flags &= ~AgentPolicy.PF_AUTO_DEPLOY;
		if (checkboxEnableUninstall.getSelection())
			flags |= AgentPolicy.PF_AUTO_UNINSTALL;
		else
			flags &= ~Container.CF_AUTO_UNBIND;
			
		if ((flags == initialFlags) && initialAutoDeployFilter.equals(filterSource.getText()))
			return;		// Nothing to apply

		if (isApply)
			setValid(false);
		
		final NXCSession session = ConsoleSharedData.getSession();
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setObjectFlags(flags);
		md.setAutoBindFilter(filterSource.getText());
		
		new ConsoleJob("Updating autodeploy configuration", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
				initialFlags = md.getObjectFlags();
				initialAutoDeployFilter = md.getAutoBindFilter();
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
							AutoDeploy.this.setValid(true);
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update autodeploy configuration";
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
