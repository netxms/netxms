/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.webui.core.dialogs;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.Properties;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
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
import org.netxms.base.NXCommon;
import org.netxms.ui.eclipse.console.api.LoginForm;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.webui.core.Activator;
import org.netxms.webui.core.BrandingManager;
import org.netxms.webui.core.Messages;

/**
 *	Login form
 */
public class DefaultLoginForm extends Window implements LoginForm
{
	private static final long serialVersionUID = 1L;
	
	private Properties properties;
	private boolean advancedSettingsEnabled;
	private ColorCache colors;
	private LabeledText textLogin;
	private LabeledText textPassword;
	private String login;
	private String password;

	/**
	 * @param parentShell
	 */
	public DefaultLoginForm(Shell parentShell, Properties properties)
	{
		super(parentShell);
		setBlockOnOpen(true);
		this.properties = properties;
		advancedSettingsEnabled = Boolean.parseBoolean(properties.getProperty("enableAdvancedSettings", "true")); //$NON-NLS-1$ //$NON-NLS-2$
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
		
		final Font headerFont = new Font(parent.getDisplay(), "Verdana", 18, SWT.BOLD); //$NON-NLS-1$
		
		parent.setBackground(colors.create(50, 99, 134));
		
		final Canvas content = new Canvas(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
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
		content.addPaintListener(new PaintListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void paintControl(PaintEvent event)
			{
				drawBackground(event.gc, content.getSize());
			}
		});
		
		Label title = new Label(content, SWT.CENTER);
		final String customTitle = BrandingManager.getInstance().getLoginTitle();
		title.setText((customTitle != null) ? customTitle : Messages.get().LoginForm_Title);
		title.setFont(headerFont);
		title.setForeground(colors.create(57, 33, 89));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		title.setLayoutData(gd);
		
		Image userImage = null;
		try
		{
			ImageDescriptor d = ImageDescriptor.createFromURL(new URL("http://127.0.0.1/netxms_login.dat")); //$NON-NLS-1$
			if (d != null)
				userImage = d.createImage(false);
		}
		catch(MalformedURLException e1)
		{
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		
		final ImageDescriptor customImage = BrandingManager.getInstance().getLoginTitleImage();
		final Image loginImage = (userImage != null) ? userImage : ((customImage != null) ? customImage.createImage() : Activator.getImageDescriptor("icons/login.png").createImage()); //$NON-NLS-1$
		Label logo = new Label(content, SWT.NONE);
		logo.setImage(loginImage);
		
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
			private static final long serialVersionUID = 1L;

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
		gd.horizontalAlignment = SWT.LEFT;
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		gd.verticalIndent = 5;
		okButton.setLayoutData(gd);
		
		final Image setupImage = Activator.getImageDescriptor("icons/app_settings.png").createImage(); //$NON-NLS-1$
		ImageHyperlink setupLink = new ImageHyperlink(loginArea, SWT.NONE);
		setupLink.setText(Messages.get().LoginForm_Options);
		setupLink.setImage(setupImage);
		gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		gd.verticalAlignment = SWT.BOTTOM;
		gd.grabExcessVerticalSpace = true;
		setupLink.setLayoutData(gd);
		setupLink.addHyperlinkListener(new HyperlinkAdapter() {
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
		
		content.addDisposeListener(new DisposeListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent event)
			{
				loginImage.dispose();
				setupImage.dispose();
				headerFont.dispose();
			}
		});
		
		Label version = new Label(parent, SWT.NONE);
		version.setText(Messages.get().LoginForm_Version + NXCommon.VERSION);
		version.setBackground(parent.getBackground());
		version.setForeground(colors.create(255, 255, 255));
		gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		gd.verticalAlignment = SWT.BOTTOM;
		version.setLayoutData(gd);
		
		getShell().setDefaultButton(okButton);
		textLogin.getTextControl().setFocus();
		return content;
	}

	/**
	 * @param gc
	 * @param size
	 */
	private void drawBackground(GC gc, Point size)
	{
		gc.setBackground(colors.create(255, 255, 255));
		gc.fillRoundRectangle(2, 2, size.x - 5, size.y - 5, 16, 16);

		gc.setForeground(colors.create(36, 66, 90));
		gc.setLineWidth(2);
		gc.drawRoundRectangle(1, 1, size.x - 3, size.y - 3, 16, 16);
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
