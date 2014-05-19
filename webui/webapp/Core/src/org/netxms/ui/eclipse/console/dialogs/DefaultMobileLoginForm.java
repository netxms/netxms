/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.net.URL;
import java.util.Properties;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Layout;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.BuildNumber;
import org.netxms.base.NXCommon;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.BrandingManager;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.api.LoginForm;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 *	Login form
 */
public class DefaultMobileLoginForm extends Window implements LoginForm
{
	private static final long serialVersionUID = 1L;
	
	private ColorCache colors;
	private LabeledText textLogin;
	private LabeledText textPassword;
	private String login;
	private String password;

	/**
	 * @param parentShell
	 */
	public DefaultMobileLoginForm(Shell parentShell, Properties properties)
	{
		super(parentShell);
		setBlockOnOpen(true);
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
		
		Rectangle displayBounds = parent.getDisplay().getPrimaryMonitor().getBounds();
		boolean isLandscape = displayBounds.width > displayBounds.height;
		System.out.println(">>>> " + displayBounds.width + " x " + displayBounds.height + " DPI=" + parent.getDisplay().getDPI());
		
		final Font headerFont = new Font(parent.getDisplay(), "Verdana", 18, SWT.BOLD); //$NON-NLS-1$
		
		parent.setBackground(colors.create(255, 255, 255));
		
		final Canvas content = new Canvas(parent, SWT.NO_FOCUS);  // SWT.NO_FOCUS is a workaround for Eclipse/RAP bug 321274
		GridLayout layout = new GridLayout();
		layout.numColumns = isLandscape ? 2 : 1;
		layout.marginWidth = 10;
		layout.marginHeight = 10;
		layout.horizontalSpacing = 10;
		layout.verticalSpacing = 10;
		content.setLayout(layout);
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalAlignment = SWT.CENTER;
		gd.verticalAlignment = SWT.CENTER;
		content.setLayoutData(gd);
		content.setBackground(parent.getBackground());
		
      Label title = new Label(content, SWT.CENTER);
      final String customTitle = BrandingManager.getInstance().getLoginTitle();
      title.setText((customTitle != null) ? customTitle : Messages.get().LoginForm_Title);
      title.setFont(headerFont);
      title.setForeground(colors.create(57, 33, 89));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = layout.numColumns;
      title.setLayoutData(gd);
      
		Image userImage = null;
		try
		{
			ImageDescriptor d = ImageDescriptor.createFromURL(new URL("http://127.0.0.1/netxms_login.dat")); //$NON-NLS-1$
			if (d != null)
				userImage = d.createImage(false);
		}
		catch(Exception e)
		{
		   Activator.logError("Exception while reading custom image", e);
		   userImage = null;
		}
		
		final ImageDescriptor customImage = BrandingManager.getInstance().getLoginTitleImage();
		final Image loginImage = (userImage != null) ? userImage : ((customImage != null) ? customImage.createImage() : Activator.getImageDescriptor("icons/login.png").createImage()); //$NON-NLS-1$
		Label logo = new Label(content, SWT.NONE);
		logo.setImage(loginImage);
		logo.setLayoutData(isLandscape ? new GridData(SWT.LEFT, SWT.TOP, false, true) : new GridData(SWT.CENTER, SWT.TOP, true, false));
		
		Composite loginArea = new Composite(content, SWT.NONE);
		layout = new GridLayout();
		loginArea.setLayout(layout);
		gd = new GridData();
		gd.widthHint = 300;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		loginArea.setLayoutData(gd);
		
		textLogin = new LabeledText(loginArea, SWT.NONE);
		textLogin.setLabel(Messages.get().LoginForm_UserName);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textLogin.setLayoutData(gd);
		
		textPassword = new LabeledText(loginArea, SWT.NONE, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD);
		textPassword.setLabel(Messages.get().LoginForm_Password);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textPassword.setLayoutData(gd);
		
		Button okButton = new Button(loginArea, SWT.PUSH);
		okButton.setText(Messages.get().LoginForm_LoginButton);
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
		gd.horizontalAlignment = SWT.RIGHT;
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		gd.verticalIndent = 5;
		okButton.setLayoutData(gd);
		
		content.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent event)
			{
				loginImage.dispose();
				headerFont.dispose();
			}
		});
		
		Label version = new Label(parent, SWT.NONE);
		version.setText(String.format(Messages.get().LoginForm_Version, NXCommon.VERSION + " (" + BuildNumber.TEXT + ")"));
		version.setBackground(parent.getBackground());
		version.setForeground(colors.create(32, 32, 32));
		gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		gd.verticalAlignment = SWT.BOTTOM;
		version.setLayoutData(gd);
		
		getShell().setDefaultButton(okButton);
		textLogin.getTextControl().setFocus();
		return content;
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
