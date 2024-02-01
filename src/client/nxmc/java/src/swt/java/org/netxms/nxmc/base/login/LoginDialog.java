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
package org.netxms.nxmc.base.login;

import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashSet;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StackLayout;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Monitor;
import org.eclipse.swt.widgets.Shell;
import org.netxms.certificate.loader.exception.KeyStoreLoaderException;
import org.netxms.certificate.manager.CertificateManager;
import org.netxms.certificate.subject.Subject;
import org.netxms.certificate.subject.SubjectParser;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.nxmc.BrandingManager;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Login dialog
 */
public class LoginDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(LoginDialog.class);
   private ImageDescriptor loginImage;
   private Combo comboServer;
   private LabeledText textLogin;
   private LabeledText textPassword;
   private Composite authEntryFields;
   private Combo comboAuth;
   private Combo comboCert;
   private String password;
   private Certificate certificate;
   private final CertificateManager certMgr;
   private AuthenticationType authMethod = AuthenticationType.PASSWORD; 

   /**
    * @param parentShell
    */
   public LoginDialog(Shell parentShell, CertificateManager certMgr)
   {
      super(parentShell);
      loginImage = BrandingManager.getLoginImage();
      this.certMgr = certMgr;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(String.format(i18n.tr("%s - Connect to Server"), BrandingManager.getProductName()));

      // Center dialog on screen
      // We don't have main window at this moment, so use
      // monitor data to determine right position
      Monitor[] ma = newShell.getDisplay().getMonitors();
      if (ma != null)
      {
         newShell.setLocation((ma[0].getClientArea().width - newShell.getSize().x) / 2,
               (ma[0].getClientArea().height - newShell.getSize().y) / 2);
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      PreferenceStore settings = PreferenceStore.getInstance();

      Composite dialogArea = new Composite(parent, SWT.NONE);
      dialogArea.setLayoutData(new GridData(GridData.FILL_BOTH));
      applyDialogFont(dialogArea);

      GridLayout dialogLayout = new GridLayout();
      dialogLayout.numColumns = 2;
      dialogLayout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogLayout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogLayout.horizontalSpacing = WidgetHelper.DIALOG_SPACING * 2;
      dialogArea.setLayout(dialogLayout);

      // Login image
      Label label = new Label(dialogArea, SWT.NONE);
      label.setImage(loginImage.createImage());
      label.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent event)
         {
            ((Label)event.widget).getImage().dispose();
         }
      });
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.verticalAlignment = SWT.TOP;
      gd.grabExcessVerticalSpace = true;
      label.setLayoutData(gd);

      final Composite fields = new Composite(dialogArea, SWT.NONE);
      GridLayout fieldsLayout = new GridLayout();
      fieldsLayout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      fieldsLayout.marginHeight = 0;
      fieldsLayout.marginWidth = 0;
      fields.setLayout(fieldsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      gd.grabExcessVerticalSpace = true;
      fields.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      comboServer = WidgetHelper.createLabeledCombo(fields, SWT.DROP_DOWN, i18n.tr("Server"), gd);

      textLogin = new LabeledText(fields, SWT.NONE, SWT.SINGLE | SWT.BORDER);
      textLogin.setLabel(i18n.tr("Login"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = WidgetHelper.getTextWidth(textLogin, "M") * 24; //$NON-NLS-1$
      textLogin.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      comboAuth = WidgetHelper.createLabeledCombo(fields, SWT.DROP_DOWN | SWT.READ_ONLY, i18n.tr("Authentication"), gd);
      comboAuth.add(i18n.tr("Password"));
      comboAuth.add(i18n.tr("Certificate"));
      comboAuth.add(i18n.tr("Token"));
      comboAuth.select(authMethodIndex(authMethod));
      comboAuth.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				selectAuthenticationField(true);
			}
		});

      authEntryFields = new Composite(fields, SWT.NONE);
      authEntryFields.setLayout(new StackLayout());
      authEntryFields.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      textPassword = new LabeledText(authEntryFields, SWT.NONE, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD);
      textPassword.setLabel(i18n.tr("Password"));

      comboCert = WidgetHelper.createLabeledCombo(authEntryFields, SWT.DROP_DOWN | SWT.READ_ONLY, i18n.tr("Certificate"), null);
      comboCert.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
			   selectCertificate();
			}
		});
      
      // Read field data
      comboServer.setItems(settings.getAsStringArray("Connect.ServerHistory"));
      String text = settings.getAsString("Connect.Server");
      if (text != null)
         comboServer.setText(text);

      text = settings.getAsString("Connect.Login");
      if (text != null)
         textLogin.setText(text);

      authMethod = AuthenticationType.getByValue(settings.getAsInteger("Connect.AuthMethod", AuthenticationType.PASSWORD.getValue()));
      comboAuth.select(authMethod.getValue());
      selectAuthenticationField(false);

      // Set initial focus
      if (comboServer.getText().isEmpty())
      {
         comboServer.setFocus();
      }
      else if (textLogin.getText().isEmpty())
      {
         textLogin.setFocus();
      }
      else
      {
      	if (authMethod == AuthenticationType.PASSWORD)
      		textPassword.setFocus();
      	else if (authMethod == AuthenticationType.CERTIFICATE)
      		comboCert.setFocus();
      }

      return dialogArea;
   }

   /**
    * Select authentication information entry filed depending on selected authentication method.
    * 
    * @param doLayout
    */
   private void selectAuthenticationField(boolean doLayout)
   {
      authMethod = authMethodFromIndex(comboAuth.getSelectionIndex());
   	switch(authMethod)
   	{
   		case PASSWORD:
   	      ((StackLayout)authEntryFields.getLayout()).topControl = textPassword;
   	      break;
   		case CERTIFICATE:
   			fillCertCombo();
   	      ((StackLayout)authEntryFields.getLayout()).topControl = comboCert.getParent();
   	      break;
   	   default:
   	      ((StackLayout)authEntryFields.getLayout()).topControl = null;	// hide entry control
   	      break;
   	}
      if (doLayout)
      {
      	authEntryFields.layout();
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
   	if ((authMethod == AuthenticationType.CERTIFICATE) && (comboCert.getSelectionIndex() == -1))
   	{
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"),
               i18n.tr("No certificate selected. Please select certificate from the list or choose different authentication method."));
   		return;
   	}

      PreferenceStore settings = PreferenceStore.getInstance();

      HashSet<String> items = new HashSet<String>();
      items.addAll(Arrays.asList(comboServer.getItems()));
      items.add(comboServer.getText());

      settings.set("Connect.Server", comboServer.getText());
      settings.set("Connect.ServerHistory", items);
      settings.set("Connect.Login", textLogin.getText());
      settings.set("Connect.AuthMethod", authMethod.getValue());
      if (certificate != null)
         settings.set("Connect.Certificate", ((X509Certificate)certificate).getSubjectX500Principal().toString());

      password = textPassword.getText();
      super.okPressed();
   }

   /**
    * @return the password
    */
   public String getPassword()
   {
      return password;
   }

   /**
    * Select certificate
    */
   private void selectCertificate()
   {
      int index = comboCert.getSelectionIndex();
      if (index >= 0)
      {
         certificate = certMgr.getCerts()[index];
      }
   }

   /**
    * @param c
    * @return
    */
   private static String getCertificateDisplayName(Certificate c)
   {
      String subjString = ((X509Certificate)c).getSubjectX500Principal().toString();
      Subject subj = SubjectParser.parseSubject(subjString);
      return String.format("%s (%s, %s, %s)", subj.getCommonName(), subj.getOrganization(), subj.getState(), subj.getCountry()); //$NON-NLS-1$
   }

   /**
    * Fill certificate list
    * 
    * @return
    */
   private boolean fillCertCombo()
   {
      if (comboCert.getItemCount() != 0)
         return true;

      try
      {
         if (certMgr.hasNoCertificates())
         {
            certMgr.load();
         }
      }
      catch(KeyStoreLoaderException ksle)
      {
         Shell shell = Display.getCurrent().getActiveShell();
         MessageDialogHelper.openError(shell, i18n.tr("Error"), i18n.tr("The key store password you provided appears to be wrong."));
         return false;
      }

      Certificate[] certs = certMgr.getCerts();
      Arrays.sort(certs, new Comparator<Certificate>() {
         @Override
         public int compare(Certificate o1, Certificate o2)
         {
            return getCertificateDisplayName(o1).compareToIgnoreCase(getCertificateDisplayName(o2));
         }
      });
      
      String[] subjectStrings = new String[certs.length];
      
      PreferenceStore settings = PreferenceStore.getInstance();
      String lastSelected = settings.getAsString("Connect.Certificate"); //$NON-NLS-1$
      int selectionIndex = 0;

      for(int i = 0; i < certs.length; i++)
      {
         String subject = ((X509Certificate)certs[i]).getSubjectX500Principal().toString();
         if (subject.equals(lastSelected))
            selectionIndex = i;
         subjectStrings[i] = getCertificateDisplayName(certs[i]);
      }

      if (subjectStrings.length != 0)
      {
         comboCert.setItems(subjectStrings);
         comboCert.select(selectionIndex);
         selectCertificate();
         return true;
      }

      return false;
   }

   /**
    * Get selected certificate
    * 
    * @return
    */
   public Certificate getCertificate()
   {
      return certificate;
   }

   /**
    * Get authentication method index.
    *
    * @param method auth method
    * @return selection index
    */
   private static int authMethodIndex(AuthenticationType method)
   {
      switch(method)
      {
         case CERTIFICATE:
            return 1;
         case PASSWORD:
            return 0;
         case TOKEN:
            return 2;
         default:
            return -1;
      }
   }

   /**
    * Get authentication method from selection index.
    *
    * @param index selection index
    * @return authentication method
    */
   private static AuthenticationType authMethodFromIndex(int index)
   {
      switch(index)
      {
         case 0:
            return AuthenticationType.PASSWORD;
         case 1:
            return AuthenticationType.CERTIFICATE;
         case 2:
            return AuthenticationType.TOKEN;
         default:
            return AuthenticationType.PASSWORD;
      }
   }
}
