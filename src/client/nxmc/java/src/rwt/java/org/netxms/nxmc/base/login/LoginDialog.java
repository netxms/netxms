/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.io.IOException;
import java.io.InputStream;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.base.VersionInfo;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.nxmc.AppPropertiesLoader;
import org.netxms.nxmc.BrandingManager;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ColorConverter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import jakarta.servlet.ServletContext;

/**
 * Login dialog
 */
public class LoginDialog extends Dialog
{
   private static final Logger logger = LoggerFactory.getLogger(LoginDialog.class);

   private I18n i18n = LocalizationHelper.getI18n(LoginDialog.class);
   private String loginMessage;
   private String errorMessage = null;
   private ImageDescriptor loginImage;
   private RGB loginImageBackground;
   private int loginImageMargins;
   private boolean hideVersion;
   private Text textLogin;
   private Text textPassword;
   private String password;
   private AuthenticationType authMethod = AuthenticationType.PASSWORD;

   /**
    * @param appProperties application properties
    * @param errorMessage error message to display
    */
   public LoginDialog(AppPropertiesLoader appProperties)
   {
      super((Shell)null);
      loginMessage = appProperties.getProperty("loginFormMessage");
      loginImage = loadUserImage(appProperties);
      if (loginImage == null)
         loginImage = BrandingManager.getLoginImage();
      loginImageBackground = ColorConverter.parseColorDefinition(appProperties.getProperty("loginFormImageBackground"));
      if (loginImageBackground == null)
         loginImageBackground = BrandingManager.getLoginImageBackground();
      loginImageMargins = appProperties.getPropertyAsInteger("loginFormImageMargins", 10);
      hideVersion = appProperties.getPropertyAsBoolean("loginFormNoVersion", false);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell shell)
   {
      super.configureShell(shell);
      shell.setText(String.format(i18n.tr("%s - Connect to Server"), BrandingManager.getProductName()));
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

      initializeDialogUnits(outerArea);

      Composite header = new Composite(outerArea, SWT.NONE);
      GridLayout headerLayout = new GridLayout();
      header.setLayout(headerLayout);
      header.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      Composite innerArea = new Composite(outerArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      innerArea.setLayout(layout);
      innerArea.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, true));
      innerArea.setData(RWT.CUSTOM_VARIANT, "LoginForm");

      dialogArea = createDialogArea(innerArea);
      dialogArea.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));

      Composite spacer = new Composite(outerArea, SWT.NONE);
      GridData gd = new GridData();
      gd.heightHint = 100;
      spacer.setLayoutData(gd);

      buttonBar = createButtonBar(innerArea);
      ((GridData)buttonBar.getLayoutData()).horizontalAlignment = SWT.FILL;
      ((GridData)buttonBar.getLayoutData()).grabExcessHorizontalSpace = true;

      Composite footer = new Composite(outerArea, SWT.NONE);
      GridLayout footerLayout = new GridLayout();
      footerLayout.numColumns = 2;
      footer.setLayout(footerLayout);
      footer.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      Label copyright = new Label(footer, SWT.LEFT);
      copyright.setText("Copyright \u00a9 2013-2025 Raden Solutions");
      copyright.setLayoutData(new GridData(SWT.LEFT, SWT.BOTTOM, true, false));

      Label version = new Label(footer, SWT.RIGHT);
      version.setText(hideVersion ? "" : VersionInfo.version());
      version.setLayoutData(new GridData(SWT.RIGHT, SWT.BOTTOM, true, false));

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

      Composite dialogArea = new Composite(parent, SWT.BORDER);
      applyDialogFont(dialogArea);

      GridLayout dialogLayout = new GridLayout();
      dialogLayout.numColumns = 2;
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogLayout.horizontalSpacing = 0;
      dialogArea.setLayout(dialogLayout);

      Color imageBackgroundColor = new Color(parent.getDisplay(), loginImageBackground);

      Composite imageArea = new Composite(dialogArea, SWT.NONE);
      imageArea.setBackground(imageBackgroundColor);
      imageArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true, 1, 3));
      GridLayout imageAreaLayout = new GridLayout();
      imageAreaLayout.marginWidth = loginImageMargins;
      imageAreaLayout.marginHeight = loginImageMargins;
      imageArea.setLayout(imageAreaLayout);

      // Login image
      Label appIcon = new Label(imageArea, SWT.TOP);
      appIcon.setBackground(imageArea.getBackground());
      appIcon.setImage(loginImage.createImage());
      appIcon.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent event)
         {
            ((Label)event.widget).getImage().dispose();
            imageBackgroundColor.dispose();
         }
      });
      appIcon.setLayoutData(new GridData(SWT.CENTER, SWT.TOP, false, true));

      Composite formArea = new Composite(dialogArea, SWT.NONE);
      GridLayout formLayout = new GridLayout();
      formLayout.marginWidth = 20;
      formLayout.marginHeight = 20;
      formLayout.verticalSpacing = 30;
      formArea.setLayout(formLayout);

      // Login label
      Label label = new Label(formArea, SWT.CENTER);
      label.setText("Log in");
      label.setData(RWT.CUSTOM_VARIANT, "LoginHeader");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.verticalAlignment = SWT.CENTER;
      gd.grabExcessHorizontalSpace = true;
      label.setLayoutData(gd);

      if ((errorMessage != null) && !errorMessage.isEmpty())
      {
         label = new Label(formArea, SWT.CENTER);
         label.setText(errorMessage);
         label.setForeground(label.getDisplay().getSystemColor(SWT.COLOR_RED));
         gd = new GridData();
         gd.horizontalAlignment = SWT.CENTER;
         gd.verticalAlignment = SWT.CENTER;
         gd.grabExcessHorizontalSpace = true;
         label.setLayoutData(gd);
      }

      if ((loginMessage != null) && !loginMessage.isEmpty())
      {
         label = new Label(formArea, SWT.CENTER);
         label.setText(loginMessage);
         gd = new GridData();
         gd.horizontalAlignment = SWT.CENTER;
         gd.verticalAlignment = SWT.CENTER;
         gd.grabExcessHorizontalSpace = true;
         label.setLayoutData(gd);
      }

      final Composite fields = new Composite(formArea, SWT.NONE);
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

      textLogin = createTextField(fields, i18n.tr("Login name"), SWT.NONE);
      textPassword = createTextField(fields, i18n.tr("Password"), SWT.PASSWORD);

      String text = settings.getAsString("Connect.Login");
      if (text != null)
         textLogin.setText(text);

      Button button = new Button(formArea, SWT.PUSH);
      button.setText(i18n.tr("Log in"));
      button.setFont(JFaceResources.getDialogFont());
      button.setData(Integer.valueOf(IDialogConstants.OK_ID));
      button.addSelectionListener(new SelectionAdapter() {
         public void widgetSelected(SelectionEvent event)
         {
            buttonPressed(((Integer)event.widget.getData()).intValue());
         }
      });
      Shell shell = parent.getShell();
      if (shell != null)
      {
         shell.setDefaultButton(button);
      }
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      button.setLayoutData(gd);

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
    * Create text field.
    *
    * @param parent parent composite
    * @param name field name
    * @param style text control style
    * @return text control
    */
   private static Text createTextField(Composite parent, String name, int style)
   {
      Composite area = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 5;
      area.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      area.setLayoutData(gd);

      Label label = new Label(area, SWT.NONE);
      label.setText(name);

      Composite textWrapper = new Composite(area, SWT.BORDER);
      textWrapper.setLayout(new GridLayout());
      textWrapper.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Text text = new Text(textWrapper, style);
      text.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      return text;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
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

   /**
    * Set error message.
    *
    * @param errorMessage error message
    */
   public void setErrorMessage(String errorMessage)
   {
      this.errorMessage = errorMessage;
   }

   /**
    * Load user image for login form
    * 
    * @param appProperties application properties loader
    * @return
    */
   private ImageDescriptor loadUserImage(AppPropertiesLoader appProperties)
   {
      String name = appProperties.getProperty("loginFormImage");
      if (name == null)
         return null;

      InputStream in = null;
      try
      {
         in = getClass().getResourceAsStream(name);
         if (in == null)
         {
            ServletContext context = RWT.getRequest().getSession().getServletContext();
            if (context != null)
            {
               in = context.getResourceAsStream(name);
            }
         }
         if (in != null)
         {
            ImageLoader loader = new ImageLoader();
            ImageData[] data = loader.load(in);
            return ImageDescriptor.createFromImageData(data[0]);
         }
      }
      catch(Exception e)
      {
         logger.error("Exception while reading user image", e);
      }
      finally
      {
         if (in != null)
         {
            try
            {
               in.close();
            }
            catch(IOException e)
            {
            }
         }
      }
      return null;
   }
}
