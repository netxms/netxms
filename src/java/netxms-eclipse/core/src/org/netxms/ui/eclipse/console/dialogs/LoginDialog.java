/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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

import java.util.Arrays;
import java.util.HashSet;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Monitor;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.forms.widgets.Form;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.BrandingManager;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Login dialog
 */
public class LoginDialog extends Dialog
{
	private FormToolkit toolkit;
	private ImageDescriptor loginImage;
	private Combo comboServer;
	private LabeledText textLogin;
	private LabeledText textPassword;
	private String password;
	private Color labelColor;
	
	/**
	 * @param parentShell
	 */
	public LoginDialog(Shell parentShell)
	{
		super(parentShell);
		
		loginImage = BrandingManager.getInstance().getLoginTitleImage();
		if (loginImage == null)
			loginImage = Activator.getImageDescriptor("icons/login.png"); //$NON-NLS-1$
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
      super.configureShell(newShell);
      String customTitle = BrandingManager.getInstance().getLoginTitle();
      newShell.setText((customTitle != null) ? customTitle : Messages.LoginDialog_title); //$NON-NLS-1$
      
      // Center dialog on screen
      // We don't have main window at this moment, so use
      // monitor data to determine right position
      Monitor [] ma = newShell.getDisplay().getMonitors();
      if (ma != null)
      	newShell.setLocation(
      	   (ma[0].getClientArea().width - newShell.getSize().x) / 2, 
      	   (ma[0].getClientArea().height - newShell.getSize().y) / 2);  
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) 
	{
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      
      toolkit = new FormToolkit(parent.getDisplay());
      Form dialogArea = toolkit.createForm(parent);
      dialogArea.setLayoutData(new GridData(GridData.FILL_BOTH));
		applyDialogFont(dialogArea);
      
      GridLayout dialogLayout = new GridLayout();
      dialogLayout.numColumns = 2;
      dialogLayout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogLayout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogLayout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.getBody().setLayout(dialogLayout);
      
      RGB customColor = BrandingManager.getInstance().getLoginTitleColor();
      labelColor = (customColor != null) ? 
         new Color(dialogArea.getDisplay(), customColor) : 
         new Color(dialogArea.getDisplay(), 36, 66, 90);
      dialogArea.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelColor.dispose();
			}
		});
      
      // Login image
      Label label = new Label(dialogArea.getBody(), SWT.NONE);
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
      gd.grabExcessVerticalSpace = true;
      label.setLayoutData(gd);
      
      Composite fields = toolkit.createComposite(dialogArea.getBody());
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
      comboServer = WidgetHelper.createLabeledCombo(fields, SWT.DROP_DOWN, Messages.LoginDialog_server, gd, toolkit);
      
      textLogin = new LabeledText(fields, SWT.NONE, SWT.SINGLE | SWT.BORDER, toolkit);
      textLogin.setLabel(Messages.LoginDialog_login);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = WidgetHelper.getTextWidth(textLogin, "M") * 24;
      textLogin.setLayoutData(gd);
      
      textPassword = new LabeledText(fields, SWT.NONE, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD, toolkit);
      textPassword.setLabel(Messages.LoginDialog_password);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textPassword.setLayoutData(gd);
      
      // Read field data
      String[] items = settings.getArray("Connect.ServerHistory"); //$NON-NLS-1$
      if (items != null)
         comboServer.setItems(items);
      String text = settings.get("Connect.Server"); //$NON-NLS-1$
      if (text != null)
      	comboServer.setText(text);

      text = settings.get("Connect.Login"); //$NON-NLS-1$
      if (text != null)
      	textLogin.setText(text);
		
      // Set initial focus
		if (comboServer.getText().isEmpty())
			comboServer.setFocus();
		else if (textLogin.getText().isEmpty())
			textLogin.setFocus();
		else
			textPassword.setFocus();
		
      return dialogArea;
   }

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed() 
	{
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      
      HashSet<String> items = new HashSet<String>();
      items.addAll(Arrays.asList(comboServer.getItems()));
      items.add(comboServer.getText());
   
      settings.put("Connect.Server", comboServer.getText()); //$NON-NLS-1$
      settings.put("Connect.ServerHistory", items.toArray(new String[items.size()])); //$NON-NLS-1$
      settings.put("Connect.Login", textLogin.getText()); //$NON-NLS-1$
//      settings.put("Connect.Encrypt", checkBoxEncrypt.getSelection()); //$NON-NLS-1$
      
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
