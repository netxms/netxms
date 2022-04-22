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
package org.netxms.nxmc.base.login;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.nxmc.BrandingManager;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Login dialog
 */
public class LoginDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(LoginDialog.class);
   private ImageDescriptor loginImage;
   private Text textLogin;
   private Text textPassword;
   private String password;
   private AuthenticationType authMethod = AuthenticationType.PASSWORD;

   /**
    * @param parentShell
    */
   public LoginDialog(Shell parentShell)
   {
      super(parentShell);

      loginImage = BrandingManager.getInstance().getLoginTitleImage();
      if (loginImage == null)
         loginImage = ResourceManager.getImageDescriptor("icons/login.png"); //$NON-NLS-1$
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell shell)
   {
      super.configureShell(shell);
      String customTitle = BrandingManager.getInstance().getLoginTitle();
      shell.setText((customTitle != null) ? customTitle : i18n.tr("NetXMS - Connect to Server")); //$NON-NLS-1$

      shell.setMaximized(true);
      shell.setFullScreen(true);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite outerArea = new Composite(parent, SWT.NONE);
      outerArea.setLayout(new GridLayout());
      outerArea.setLayoutData(new GridData(GridData.FILL_BOTH));

      Label background = new Label(outerArea, SWT.NONE);
      GridData gd = new GridData();
      gd.exclude = true;
      background.setLayoutData(gd);
      background.setData(RWT.MARKUP_ENABLED, Boolean.TRUE);
      background.setText("<span style='width: 100px; height: 100px; display: flex; width: 100vw; height: 100vh;" + "background-image: url(" +
            RWT.getResourceManager().getLocation("login-background") + "); background-size: cover; background-position: center; filter: blur(8px); -webkit-filter: blur(8px);'></span>");
      outerArea.addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            background.setSize(outerArea.getSize());
            background.moveBelow(null);
         }
      });

      initializeDialogUnits(outerArea);

      Composite innerArea = new Composite(outerArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      innerArea.setLayout(layout);
      innerArea.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, true));
      innerArea.setData(RWT.CUSTOM_VARIANT, "LoginForm");

      dialogArea = createDialogArea(innerArea);
      buttonBar = createButtonBar(innerArea);

      applyDialogFont(outerArea);
      return outerArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      PreferenceStore settings = PreferenceStore.getInstance();

      Composite dialogArea = new Composite(parent, SWT.NONE);
      dialogArea.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));
      applyDialogFont(dialogArea);

      GridLayout dialogLayout = new GridLayout();
      dialogLayout.numColumns = 1;
      dialogLayout.marginWidth = 20;
      dialogLayout.marginHeight = 20;
      dialogLayout.verticalSpacing = 50;
      dialogArea.setLayout(dialogLayout);

      // Login image
      Label label = new Label(dialogArea, SWT.CENTER);
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
      gd.verticalAlignment = SWT.CENTER;
      gd.grabExcessHorizontalSpace = true;
      label.setLayoutData(gd);

      final Composite fields = new Composite(dialogArea, SWT.NONE);
      GridLayout fieldsLayout = new GridLayout();
      fieldsLayout.verticalSpacing = 20;
      fieldsLayout.marginHeight = 0;
      fieldsLayout.marginWidth = 0;
      fields.setLayout(fieldsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      gd.grabExcessVerticalSpace = true;
      fields.setLayoutData(gd);

      textLogin = new Text(fields, SWT.SINGLE | SWT.BORDER);
      textLogin.setMessage(i18n.tr("Login"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = WidgetHelper.getTextWidth(textLogin, "M") * 48;
      textLogin.setLayoutData(gd);
      textLogin.setData(RWT.CUSTOM_VARIANT, "login");

      textPassword = new Text(fields, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD);
      textPassword.setMessage(i18n.tr("Password"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textPassword.setLayoutData(gd);
      textPassword.setData(RWT.CUSTOM_VARIANT, "login");

      String text = settings.getAsString("Connect.Login"); //$NON-NLS-1$
      if (text != null)
         textLogin.setText(text);

      // Set initial focus
      if (textLogin.getText().isEmpty())
      {
         textLogin.setFocus();
      }
      else
      {
         textPassword.setFocus();
      }

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      PreferenceStore settings = PreferenceStore.getInstance();

      settings.set("Connect.Login", textLogin.getText());
      settings.set("Connect.AuthMethod", authMethod.getValue());

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
}
