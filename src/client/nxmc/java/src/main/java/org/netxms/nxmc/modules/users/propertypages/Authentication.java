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
package org.netxms.nxmc.modules.users.propertypages;

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
import org.netxms.client.NXCSession;
import org.netxms.client.constants.CertificateMappingMethod;
import org.netxms.client.constants.UserAuthenticationMethod;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.dialogs.TwoFactorAuthMethodEditDialog;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * User's "authentication" property page
 */
public class Authentication extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(Authentication.class);

   private NXCSession session = Registry.getSession();
	private User user;
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
   public Authentication(User user, MessageAreaHolder messageArea)
	{
      super(LocalizationHelper.getI18n(Authentication.class).tr("Authentication"), messageArea);
      this.user = user;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      Group groupFlags = new Group(dialogArea, SWT.NONE);
      groupFlags.setText(i18n.tr("Account Options"));
      layout = new GridLayout();
      groupFlags.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = GridData.FILL;
		gd.grabExcessHorizontalSpace = true;
		groupFlags.setLayoutData(gd);

      checkDisabled = new Button(groupFlags, SWT.CHECK);
      checkDisabled.setText(i18n.tr("Account &disabled"));
      checkDisabled.setSelection(user.isDisabled());

      checkChangePassword = new Button(groupFlags, SWT.CHECK);
      checkChangePassword.setText(i18n.tr("User must &change password at next logon"));
      checkChangePassword.setSelection(user.isPasswordChangeNeeded());

      checkFixedPassword = new Button(groupFlags, SWT.CHECK);
      checkFixedPassword.setText(i18n.tr("User cannot change &password"));
      checkFixedPassword.setSelection(user.isPasswordChangeForbidden());

      checkTokenOnlyAuth = new Button(groupFlags, SWT.CHECK);
      checkTokenOnlyAuth.setText(i18n.tr("Allow only &token based authentication"));
      checkTokenOnlyAuth.setSelection((user.getFlags() & User.TOKEN_AUTH_ONLY) != 0);

      checkCloseSessions = new Button(groupFlags, SWT.CHECK);
      checkCloseSessions.setText(i18n.tr("Close other &sessions after login"));
      checkCloseSessions.setSelection((user.getFlags() & User.CLOSE_OTHER_SESSIONS) != 0);

      Group groupMethod = new Group(dialogArea, SWT.NONE);
      groupMethod.setText(i18n.tr("Authentication Method"));
      layout = new GridLayout();
      layout.numColumns = 2;
      groupMethod.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = GridData.FILL;
		gd.grabExcessHorizontalSpace = true;
		groupMethod.setLayoutData(gd);

		Label label = new Label(groupMethod, SWT.NONE);
		label.setText(i18n.tr("Authentication method:"));
		comboAuthMethod = new Combo(groupMethod, SWT.DROP_DOWN | SWT.READ_ONLY);
      comboAuthMethod.add(i18n.tr("Local password"));
		comboAuthMethod.add(i18n.tr("RADIUS password"));
		comboAuthMethod.add(i18n.tr("Certificate"));
      comboAuthMethod.add(i18n.tr("Certificate or local password"));
		comboAuthMethod.add(i18n.tr("Certificate or RADIUS password"));
      comboAuthMethod.add(i18n.tr("LDAP password"));
      comboAuthMethod.select(user.getAuthMethod().getValue());
		gd = new GridData();
		gd.horizontalAlignment = GridData.FILL;
		gd.grabExcessHorizontalSpace = true;
		comboAuthMethod.setLayoutData(gd);

		label = new Label(groupMethod, SWT.NONE);
		label.setText(i18n.tr("Certificate mapping method:"));
		comboMappingMethod = new Combo(groupMethod, SWT.DROP_DOWN | SWT.READ_ONLY);
		comboMappingMethod.add(i18n.tr("Subject"));
		comboMappingMethod.add(i18n.tr("Public key"));
      comboMappingMethod.add(i18n.tr("Common name"));
      comboMappingMethod.add("Template ID");
      comboMappingMethod.select(user.getCertMappingMethod().getValue());
		gd = new GridData();
		gd.horizontalAlignment = GridData.FILL;
		gd.grabExcessHorizontalSpace = true;
		comboMappingMethod.setLayoutData(gd);

		gd = new GridData();
		gd.horizontalAlignment = GridData.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		gd.widthHint = 300;
      textMappingData = WidgetHelper.createLabeledText(groupMethod, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Certificate mapping data"),
                                                       user.getCertMappingData(), gd);

      Group twoFactorAuth = new Group(dialogArea, SWT.NONE);
      twoFactorAuth.setText(i18n.tr("Two-factor authentication methods"));
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
      addButton.setText(i18n.tr("&Add..."));
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
      editButton.setText(i18n.tr("&Edit..."));
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
      deleteButton.setText(i18n.tr("&Delete"));
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

      twoFactorAuthMethodBindings = new ArrayList<MethodBinding>(user.getTwoFactorAuthMethodBindings().size());
      for(Entry<String, Map<String, String>> e : user.getTwoFactorAuthMethodBindings().entrySet())
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
            MessageDialogHelper.openError(getShell(), i18n.tr("Error"), String.format(i18n.tr("Two-factor authentication method %s already configured"), b.name));
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
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
	protected boolean applyChanges(final boolean isApply)
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
		flags |= user.getFlags() & AbstractUserObject.LDAP_USER;
		user.setFlags(flags);

		// Authentication
      user.setAuthMethod(UserAuthenticationMethod.getByValue(comboAuthMethod.getSelectionIndex()));
      user.setCertMappingMethod(CertificateMappingMethod.getByValue(comboMappingMethod.getSelectionIndex()));
		user.setCertMappingData(textMappingData.getText());

      // Two-factor authentication
      Map<String, Map<String, String>> bindings = new HashMap<String, Map<String, String>>(twoFactorAuthMethodBindings.size());
      for(MethodBinding b : twoFactorAuthMethodBindings)
         bindings.put(b.name, b.configuration);
      user.setTwoFactorAuthMethodBindings(bindings);

      if (isApply)
      {
         setMessage(null);
         setValid(false);
      }

      new Job(i18n.tr("Update user database object"), null, getMessageArea(isApply)) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyUserDBObject(user, AbstractUserObject.MODIFY_FLAGS | AbstractUserObject.MODIFY_AUTH_METHOD | AbstractUserObject.MODIFY_2FA_BINDINGS | AbstractUserObject.MODIFY_CERT_MAPPING);
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
				return i18n.tr("Cannot update user account");
			}
		}.start();

		return true;
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
