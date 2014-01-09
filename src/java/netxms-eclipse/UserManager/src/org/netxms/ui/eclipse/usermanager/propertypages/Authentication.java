/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.usermanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.api.client.Session;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.api.client.users.User;
import org.netxms.api.client.users.UserManager;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.Messages;

/**
 * User's "authentication" property page
 *
 */
public class Authentication extends PropertyPage
{
	private Session session;
	private User object;
	private Button checkDisabled;
	private Button checkChangePassword;
	private Button checkFixedPassword;
	private Combo comboAuthMethod;
	private Combo comboMappingMethod;
	private Text textMappingData;
	
	/**
	 * Default constructor
	 */
	public Authentication()
	{
		super();
		session = ConsoleSharedData.getSession();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		object = (User)getElement().getAdapter(AbstractUserObject.class);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      Group groupFlags = new Group(dialogArea, SWT.NONE);
      groupFlags.setText(Messages.get().Authentication_AccountOptions);
      GridLayout groupFlagsLayout = new GridLayout();
      groupFlags.setLayout(groupFlagsLayout);
		GridData gridData = new GridData();
		gridData.horizontalAlignment = GridData.FILL;
		gridData.grabExcessHorizontalSpace = true;
		groupFlags.setLayoutData(gridData);
		
      checkDisabled = new Button(groupFlags, SWT.CHECK);
      checkDisabled.setText(Messages.get().Authentication_AccountDisabled);
      checkDisabled.setSelection(object.isDisabled());
		
      checkChangePassword = new Button(groupFlags, SWT.CHECK);
      checkChangePassword.setText(Messages.get().Authentication_MustChangePassword);
      checkChangePassword.setSelection(object.isPasswordChangeNeeded());
		
      checkFixedPassword = new Button(groupFlags, SWT.CHECK);
      checkFixedPassword.setText(Messages.get().Authentication_CannotChangePassword);
      checkFixedPassword.setSelection(object.isPasswordChangeForbidden());
		
      Group groupMethod = new Group(dialogArea, SWT.NONE);
      groupMethod.setText(Messages.get().Authentication_AuthMethod_Group);
      GridLayout groupMethodLayout = new GridLayout();
      groupMethodLayout.numColumns = 2;
      groupMethod.setLayout(groupMethodLayout);
		gridData = new GridData();
		gridData.horizontalAlignment = GridData.FILL;
		gridData.grabExcessHorizontalSpace = true;
		groupMethod.setLayoutData(gridData);
		
		Label label = new Label(groupMethod, SWT.NONE);
		label.setText(Messages.get().Authentication_AuthMethod_Label);
		comboAuthMethod = new Combo(groupMethod, SWT.DROP_DOWN | SWT.READ_ONLY);
		comboAuthMethod.add(Messages.get().Authentication_Password);
		comboAuthMethod.add(Messages.get().Authentication_RADIUS);
		comboAuthMethod.add(Messages.get().Authentication_Certificate);
		comboAuthMethod.add(Messages.get().Authentication_CertificateOrPassword);
		comboAuthMethod.add(Messages.get().Authentication_CertificateOrRADIUS);
		comboAuthMethod.select(object.getAuthMethod());
		gridData = new GridData();
		gridData.horizontalAlignment = GridData.FILL;
		gridData.grabExcessHorizontalSpace = true;
		comboAuthMethod.setLayoutData(gridData);
		
		label = new Label(groupMethod, SWT.NONE);
		label.setText(Messages.get().Authentication_CertMapping);
		comboMappingMethod = new Combo(groupMethod, SWT.DROP_DOWN | SWT.READ_ONLY);
		comboMappingMethod.add(Messages.get().Authentication_Subject);
		comboMappingMethod.add(Messages.get().Authentication_PublicKey);
      comboMappingMethod.add("Common name");
		comboMappingMethod.select(object.getCertMappingMethod());
		gridData = new GridData();
		gridData.horizontalAlignment = GridData.FILL;
		gridData.grabExcessHorizontalSpace = true;
		comboMappingMethod.setLayoutData(gridData);

		gridData = new GridData();
		gridData.horizontalAlignment = GridData.FILL;
		gridData.grabExcessHorizontalSpace = true;
		gridData.horizontalSpan = 2;
		gridData.widthHint = 300;
      textMappingData = WidgetHelper.createLabeledText(groupMethod, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, Messages.get().Authentication_MappingData,
                                                       object.getCertMappingData(), gridData);
		
		return dialogArea;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		// Account flags
		int flags = 0;
		if (checkDisabled.getSelection())
			flags |= AbstractUserObject.DISABLED;
		if (checkChangePassword.getSelection())
			flags |= AbstractUserObject.CHANGE_PASSWORD;
		if (checkFixedPassword.getSelection())
			flags |= AbstractUserObject.CANNOT_CHANGE_PASSWORD;
		object.setFlags(flags);
		
		// Authentication
		object.setAuthMethod(comboAuthMethod.getSelectionIndex());
		object.setCertMappingMethod(comboMappingMethod.getSelectionIndex());
		object.setCertMappingData(textMappingData.getText());
		
		if (isApply)
			setValid(false);
		
		new ConsoleJob(Messages.get().Authentication_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				((UserManager)session).modifyUserDBObject(object, UserManager.USER_MODIFY_FLAGS | UserManager.USER_MODIFY_AUTH_METHOD | UserManager.USER_MODIFY_CERT_MAPPING);
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
							Authentication.this.setValid(true);
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().Authentication_JobError;
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
