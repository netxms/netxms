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
package org.netxms.ui.eclipse.console.dialogs;

import java.io.IOException;
import java.io.InputStream;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.window.Window;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Layout;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.base.VersionInfo;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.AppPropertiesLoader;
import org.netxms.ui.eclipse.console.BrandingManager;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.api.LoginForm;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 *	Login form
 */
public class DefaultLoginForm extends Window implements LoginForm
{
   public static final RGB DEFAULT_SCREEN_BACKGROUND_COLOR = new RGB(240, 240, 240);
   public static final RGB DEFAULT_FORM_BACKGROUND_COLOR =  new RGB(0x97, 0x98, 0x8F);
   public static final RGB DEFAULT_SCREEN_TEXT_COLOR = new RGB(0x29, 0x2A, 0x30);

	private AppPropertiesLoader properties;
	private boolean advancedSettingsEnabled;
	private ColorCache colors;
	private LabeledText textLogin;
	private LabeledText textPassword;
	private String login;
	private String password;

	/**
	 * @param parentShell
	 */
	public DefaultLoginForm(Shell parentShell, AppPropertiesLoader properties)
	{
		super(parentShell);
		setBlockOnOpen(true);
		this.properties = properties;
		advancedSettingsEnabled = Boolean.parseBoolean(properties.getProperty("enableAdvancedSettings", "false")); //$NON-NLS-1$ //$NON-NLS-2$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setFullScreen(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		colors = new ColorCache(parent);
		
      RGB screenBkgnd = BrandingManager.getInstance().getLoginScreenBackgroundColor(DEFAULT_SCREEN_BACKGROUND_COLOR);
      parent.setBackground(colors.create(screenBkgnd));
		
      Composite fillerTop = new Composite(parent, SWT.NONE);
      fillerTop.setBackground(colors.create(screenBkgnd));
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = false;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.heightHint = 100;
      fillerTop.setLayoutData(gd);

      RGB formBkgnd = BrandingManager.getInstance().getLoginFormBackgroundColor(DEFAULT_FORM_BACKGROUND_COLOR);

		final Canvas content = new Canvas(parent, SWT.BORDER | SWT.NO_FOCUS);  // SWT.NO_FOCUS is a workaround for Eclipse/RAP bug 321274
		GridLayout layout = new GridLayout();
		layout.numColumns = 1;
		layout.marginWidth = 10;
		layout.marginHeight = 10;
		layout.marginTop = 20;
		layout.horizontalSpacing = 10;
		layout.verticalSpacing = 15;
		content.setLayout(layout);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalAlignment = SWT.CENTER;
		gd.verticalAlignment = SWT.CENTER;
		content.setLayoutData(gd);
		content.setBackground(colors.create(formBkgnd));
		content.setData(RWT.CUSTOM_VARIANT, "LoginForm");

		final Image userImage = (properties.getProperty("loginFormImage") != null) ? loadUserImage() : null;		
		final ImageDescriptor customImage = BrandingManager.getInstance().getLoginTitleImage();
		final Image loginImage = (userImage != null) ? userImage : ((customImage != null) ? customImage.createImage() : Activator.getImageDescriptor("icons/login.png").createImage()); //$NON-NLS-1$
		Label logo = new Label(content, SWT.NONE);
		logo.setBackground(colors.create(formBkgnd));
		logo.setImage(loginImage);
		logo.setLayoutData(new GridData(SWT.CENTER, SWT.TOP, false, false));
		
		Composite loginArea = new Composite(content, SWT.NONE);
		loginArea.setBackground(colors.create(formBkgnd));
		layout = new GridLayout();
		layout.verticalSpacing = 20;
		layout.marginWidth = 20;
		layout.marginHeight = 20;
		loginArea.setLayout(layout);
		gd = new GridData();
		gd.widthHint = 400;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		loginArea.setLayoutData(gd);
		
		textLogin = new LabeledText(loginArea, SWT.NONE);
		textLogin.setBackground(colors.create(formBkgnd));
		textLogin.setLabel(Messages.get().LoginForm_UserName);
		textLogin.getLabelControl().setData(RWT.CUSTOM_VARIANT, "login");
      textLogin.getTextControl().setData(RWT.CUSTOM_VARIANT, "login");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textLogin.setLayoutData(gd);
		
		textPassword = new LabeledText(loginArea, SWT.NONE, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD);
		textPassword.setBackground(colors.create(formBkgnd));
		textPassword.setLabel(Messages.get().LoginForm_Password);
		textPassword.getLabelControl().setData(RWT.CUSTOM_VARIANT, "login");
		textPassword.getTextControl().setData(RWT.CUSTOM_VARIANT, "login");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textPassword.setLayoutData(gd);
		
		Button okButton = new Button(loginArea, SWT.PUSH);
		okButton.setText(Messages.get().LoginForm_LoginButton);
		okButton.setData(RWT.CUSTOM_VARIANT, "login");
		okButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				okPressed();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		gd = new GridData();
		gd.horizontalAlignment = SWT.CENTER;
		gd.widthHint = WidgetHelper.WIDE_BUTTON_WIDTH_HINT;
		gd.verticalIndent = 15;
		okButton.setLayoutData(gd);

		if (advancedSettingsEnabled)
		{
   		final ImageHyperlink setupLink = new ImageHyperlink(loginArea, SWT.NONE);
   		setupLink.setText(Messages.get().LoginForm_Options);
   		setupLink.setForeground(colors.create(33, 0, 127));
   		gd = new GridData();
   		gd.horizontalAlignment = SWT.RIGHT;
   		gd.verticalAlignment = SWT.BOTTOM;
   		gd.grabExcessVerticalSpace = true;
   		setupLink.setLayoutData(gd);
   		setupLink.addHyperlinkListener(new HyperlinkAdapter() {
   			@Override
            public void linkEntered(HyperlinkEvent e)
            {
               super.linkEntered(e);
               setupLink.setUnderlined(true);
            }
   
            @Override
            public void linkExited(HyperlinkEvent e)
            {
               super.linkExited(e);
               setupLink.setUnderlined(false);
            }
   
            @Override
   			public void linkActivated(HyperlinkEvent e)
   			{
   				if (advancedSettingsEnabled)
   				{
   					LoginSettingsDialog dlg = new LoginSettingsDialog(getShell(), properties);
   					dlg.open();
   				}
   				else
   				{
   					MessageDialog.openError(getShell(), Messages.get().LoginForm_Error, Messages.get().LoginForm_AdvOptionsDisabled);
   				}
   			}
   		});
		}

		content.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent event)
			{
				loginImage.dispose();
			}
		});
		
		Composite fillerBottom = new Composite(parent, SWT.NONE);
		fillerBottom.setBackground(colors.create(screenBkgnd));
		fillerBottom.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		
		Label version = new Label(parent, SWT.NONE);
		version.setText(String.format(Messages.get().LoginForm_Version, VersionInfo.version()));
		version.setBackground(parent.getBackground());
		version.setForeground(colors.create(BrandingManager.getInstance().getLoginScreenTextColor(DEFAULT_SCREEN_TEXT_COLOR)));
		gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		gd.verticalAlignment = SWT.BOTTOM;
		version.setLayoutData(gd);
		
		getShell().setDefaultButton(okButton);
		textLogin.getTextControl().setFocus();
		return content;
	}
	
	/**
	 * Load user image for login form
	 * 
	 * @return
	 */
	private Image loadUserImage()
	{
	   Image img = null;
	   InputStream in = null;
      try
      {
         in = getClass().getResourceAsStream(properties.getProperty("loginFormImage"));
         if (in != null)
         {
            ImageLoader loader = new ImageLoader();
            ImageData[] data = loader.load(in);
            img = new Image(getShell().getDisplay(), data[0]);
         }
      }
      catch(Exception e)
      {
         Activator.logError("Exception while reading user image", e);
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
      return img;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#getLayout()
	 */
	@Override
	protected Layout getLayout()
	{
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		return layout;
	}

	/**
	 * OK button handler
	 */
	private void okPressed()
	{
		login = textLogin.getText();
		password = textPassword.getText();
		setReturnCode(OK);
		close();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.console.api.LoginForm#getLogin()
	 */
	@Override
	public String getLogin()
	{
		return login;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.console.api.LoginForm#getPassword()
	 */
	@Override
	public String getPassword()
	{
		return password;
	}
}
