/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
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
import org.netxms.client.NXCSession;
import org.netxms.client.constants.CertificateMappingMethod;
import org.netxms.client.constants.UserAuthenticationMethod;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.Messages;
import org.netxms.ui.eclipse.usermanager.dialogs.TwoFactorAuthMethodEditDialog;

/**
 * User's "authentication" property page
 */
public class Authentication extends PropertyPage
{
	private NXCSession session;
	private User object;
   private List<MethodBinding> twoFactorAuthMethodBindings;
	private Button checkDisabled;
	private Button checkChangePassword;
	private Button checkFixedPassword;
   private Button checkTokenOnlyAuth;
   private Button checkCloseSessions;
	private Combo comboAuthMethod;
	private Combo comboMappingMethod;
	private Text textMappingData;
   private TableViewer twoFactorAuthMethodList;

	/**
	 * Default constructor
	 */
	public Authentication()
	{
		super();
		session = ConsoleSharedData.getSession();
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
      object = getElement().getAdapter(User.class);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      Group groupFlags = new Group(dialogArea, SWT.NONE);
      groupFlags.setText(Messages.get().Authentication_AccountOptions);
      layout = new GridLayout();
      groupFlags.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = GridData.FILL;
		gd.grabExcessHorizontalSpace = true;
		groupFlags.setLayoutData(gd);

      checkDisabled = new Button(groupFlags, SWT.CHECK);
      checkDisabled.setText(Messages.get().Authentication_AccountDisabled);
      checkDisabled.setSelection(object.isDisabled());

      checkChangePassword = new Button(groupFlags, SWT.CHECK);
      checkChangePassword.setText(Messages.get().Authentication_MustChangePassword);
      checkChangePassword.setSelection(object.isPasswordChangeNeeded());

      checkFixedPassword = new Button(groupFlags, SWT.CHECK);
      checkFixedPassword.setText(Messages.get().Authentication_CannotChangePassword);
      checkFixedPassword.setSelection(object.isPasswordChangeForbidden());

      checkTokenOnlyAuth = new Button(groupFlags, SWT.CHECK);
      checkTokenOnlyAuth.setText("Allow only &token based authentication");
      checkTokenOnlyAuth.setSelection((object.getFlags() & User.TOKEN_AUTH_ONLY) != 0);

      checkCloseSessions = new Button(groupFlags, SWT.CHECK);
      checkCloseSessions.setText(Messages.get().Authentication_CloseOtherSessions);
      checkCloseSessions.setSelection((object.getFlags() & User.CLOSE_OTHER_SESSIONS) != 0);

      Group groupMethod = new Group(dialogArea, SWT.NONE);
      groupMethod.setText(Messages.get().Authentication_AuthMethod_Group);
      layout = new GridLayout();
      layout.numColumns = 2;
      groupMethod.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = GridData.FILL;
		gd.grabExcessHorizontalSpace = true;
		groupMethod.setLayoutData(gd);

		Label label = new Label(groupMethod, SWT.NONE);
		label.setText(Messages.get().Authentication_AuthMethod_Label);
		comboAuthMethod = new Combo(groupMethod, SWT.DROP_DOWN | SWT.READ_ONLY);
      comboAuthMethod.add(Messages.get().Authentication_Local);
		comboAuthMethod.add(Messages.get().Authentication_RADIUS);
		comboAuthMethod.add(Messages.get().Authentication_Certificate);
      comboAuthMethod.add(Messages.get().Authentication_CertificateOrLocal);
		comboAuthMethod.add(Messages.get().Authentication_CertificateOrRADIUS);
      comboAuthMethod.add(Messages.get().Authentication_LDAP);
      comboAuthMethod.select(object.getAuthMethod().getValue());
		gd = new GridData();
		gd.horizontalAlignment = GridData.FILL;
		gd.grabExcessHorizontalSpace = true;
		comboAuthMethod.setLayoutData(gd);

		label = new Label(groupMethod, SWT.NONE);
		label.setText(Messages.get().Authentication_CertMapping);
		comboMappingMethod = new Combo(groupMethod, SWT.DROP_DOWN | SWT.READ_ONLY);
		comboMappingMethod.add(Messages.get().Authentication_Subject);
		comboMappingMethod.add(Messages.get().Authentication_PublicKey);
      comboMappingMethod.add(Messages.get().Authentication_CommonName);
      comboMappingMethod.add("Template ID");
      comboMappingMethod.select(object.getCertMappingMethod().getValue());
		gd = new GridData();
		gd.horizontalAlignment = GridData.FILL;
		gd.grabExcessHorizontalSpace = true;
		comboMappingMethod.setLayoutData(gd);

		gd = new GridData();
		gd.horizontalAlignment = GridData.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		gd.widthHint = 300;
      textMappingData = WidgetHelper.createLabeledText(groupMethod, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, Messages.get().Authentication_MappingData,
                                                       object.getCertMappingData(), gd);

      Group twoFactorAuth = new Group(dialogArea, SWT.NONE);
      twoFactorAuth.setText("Two-factor authentication methods");
      layout = new GridLayout();
      layout.numColumns = 2;
      twoFactorAuth.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      twoFactorAuth.setLayoutData(gd);

      twoFactorAuthMethodList = new TableViewer(twoFactorAuth, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      twoFactorAuthMethodList.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      twoFactorAuthMethodList.setContentProvider(new ArrayContentProvider());
      twoFactorAuthMethodList.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((MethodBinding)element).name;
         }
      });
      twoFactorAuthMethodList.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((MethodBinding)e1).name.compareToIgnoreCase(((MethodBinding)e2).name);
         }
      });

      Composite buttons = new Composite(twoFactorAuth, SWT.NONE);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      buttons.setLayout(layout);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      buttons.setLayoutData(gd);

      final Button addButton = new Button(buttons, SWT.PUSH);
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(gd);
      addButton.setText(Messages.get().Members_Add);
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addMethod();
         }
      });

      final Button editButton = new Button(buttons, SWT.PUSH);
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(gd);
      editButton.setText("&Edit...");
      editButton.setEnabled(false);
      editButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editMethod();
         }
      });

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(gd);
      deleteButton.setText(Messages.get().Members_Delete);
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            IStructuredSelection selection = twoFactorAuthMethodList.getStructuredSelection();
            for(Object o : selection.toList())
               twoFactorAuthMethodBindings.remove(o);
            twoFactorAuthMethodList.refresh();
         }
      });

      twoFactorAuthMethodList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = twoFactorAuthMethodList.getStructuredSelection();
            editButton.setEnabled(selection.size() == 1);
            deleteButton.setEnabled(!selection.isEmpty());
         }
      });
      twoFactorAuthMethodList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editMethod();
         }
      });

      twoFactorAuthMethodBindings = new ArrayList<MethodBinding>(object.getTwoFactorAuthMethodBindings().size());
      for(Entry<String, Map<String, String>> e : object.getTwoFactorAuthMethodBindings().entrySet())
         twoFactorAuthMethodBindings.add(new MethodBinding(e.getKey(), e.getValue()));
      twoFactorAuthMethodList.setInput(twoFactorAuthMethodBindings);

		return dialogArea;
	}

   /**
    * Add new method
    */
   private void addMethod()
   {
      Set<String> configuredMethods = new HashSet<>();
      for(MethodBinding b : twoFactorAuthMethodBindings)
         configuredMethods.add(b.name);
      TwoFactorAuthMethodEditDialog dlg = new TwoFactorAuthMethodEditDialog(getShell(), null, null, configuredMethods);
      if (dlg.open() != Window.OK)
         return;

      for(MethodBinding b : twoFactorAuthMethodBindings)
      {
         if (b.name.equals(dlg.getName()))
         {
            MessageDialogHelper.openError(getShell(), "Error", String.format("Two-factor authentication method %s already configured", b.name));
            return;
         }
      }

      twoFactorAuthMethodBindings.add(new MethodBinding(dlg.getName(), dlg.getConfiguration()));
      twoFactorAuthMethodList.refresh();
   }

   /**
    * Edit selected method
    */
   private void editMethod()
   {
      IStructuredSelection selection = twoFactorAuthMethodList.getStructuredSelection();
      if (selection.size() != 1)
         return;

      MethodBinding b = (MethodBinding)selection.getFirstElement();
      TwoFactorAuthMethodEditDialog dlg = new TwoFactorAuthMethodEditDialog(getShell(), b.name, b.configuration, null);
      if (dlg.open() != Window.OK)
         return;

      b.configuration = dlg.getConfiguration();
      twoFactorAuthMethodList.refresh();
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
      if (checkTokenOnlyAuth.getSelection())
         flags |= AbstractUserObject.TOKEN_AUTH_ONLY;
		if (checkCloseSessions.getSelection())
         flags |= AbstractUserObject.CLOSE_OTHER_SESSIONS;
		flags |= object.getFlags() & AbstractUserObject.LDAP_USER;
		object.setFlags(flags);

		// Authentication
      object.setAuthMethod(UserAuthenticationMethod.getByValue(comboAuthMethod.getSelectionIndex()));
      object.setCertMappingMethod(CertificateMappingMethod.getByValue(comboMappingMethod.getSelectionIndex()));
		object.setCertMappingData(textMappingData.getText());

      // Two-factor authentication
      Map<String, Map<String, String>> bindings = new HashMap<String, Map<String, String>>(twoFactorAuthMethodBindings.size());
      for(MethodBinding b : twoFactorAuthMethodBindings)
         bindings.put(b.name, b.configuration);
      object.setTwoFactorAuthMethodBindings(bindings);

		if (isApply)
			setValid(false);

      new ConsoleJob(Messages.get().Authentication_JobTitle, null, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyUserDBObject(object, AbstractUserObject.MODIFY_FLAGS | AbstractUserObject.MODIFY_AUTH_METHOD | AbstractUserObject.MODIFY_2FA_BINDINGS | AbstractUserObject.MODIFY_CERT_MAPPING);
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> Authentication.this.setValid(true));
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().Authentication_JobError;
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

   /**
    * Two-factor authentication method binding
    */
   private static class MethodBinding
   {
      String name;
      Map<String, String> configuration;

      public MethodBinding(String name, Map<String, String> configuration)
      {
         this.name = name;
         this.configuration = new HashMap<String, String>(configuration);
      }
   }
}
