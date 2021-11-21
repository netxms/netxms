/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.nxmc.base.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for showing security warnings
 */
public class SecurityWarningDialog extends Dialog
{
	public static final int NO = 0;
	public static final int YES = 1;
	public static final int ALWAYS = 2;

	private static final Color BACKGROUND_COLOR = new Color(Display.getDefault(), 242, 177, 0);

   private I18n i18n = LocalizationHelper.getI18n(SecurityWarningDialog.class);
	private String message;
	private String details;
	private Button checkDontAskAgain;
	private Font boldFont;
	private boolean dontAskAgain = false;

	/**
	 * Show security warning dialog.
	 * 
	 * @param parentShell
	 * @param message
	 * @param details
	 * @return one of the codes NO, YES, or ALWAYS
	 */
	public static int showSecurityWarning(Shell parentShell, String message, String details)
	{
		SecurityWarningDialog dlg = new SecurityWarningDialog(parentShell, message, details);
		if (dlg.open() != Window.OK)
			return NO;
		return dlg.dontAskAgain ? ALWAYS : YES;
	}

	/**
	 * @param parentShell
	 * @param message
	 * @param details
	 */
	protected SecurityWarningDialog(Shell parentShell, String message, String details)
	{
		super(parentShell);
		
		this.message = message;
		this.details = details;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Security Warning"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected void createButtonsForButtonBar(Composite parent)
	{
		createButton(parent, IDialogConstants.OK_ID, IDialogConstants.YES_LABEL, true);
		createButton(parent, IDialogConstants.CANCEL_ID, IDialogConstants.NO_LABEL, false);
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		dialogArea.setLayout(layout);
		
		Composite topArea = new Composite(dialogArea, SWT.NONE);
		topArea.setBackground(BACKGROUND_COLOR);
		layout = new GridLayout();
		layout.marginHeight = 10;
		layout.marginWidth = 10;
		layout.numColumns = 2;
		topArea.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 500;
		topArea.setLayoutData(gd);
		
		final Label icon = new Label(topArea, SWT.NONE);
		icon.setBackground(BACKGROUND_COLOR);
      icon.setImage(ResourceManager.getImageDescriptor("icons/security-warning.png").createImage()); //$NON-NLS-1$
		icon.setLayoutData(new GridData(SWT.TOP, SWT.CENTER, false, false));
		icon.addDisposeListener(new DisposeListener() {	
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				icon.getImage().dispose();
			}
		});
		
		FontData fd = JFaceResources.getDialogFont().getFontData()[0];
		fd.setStyle(SWT.BOLD);
		boldFont = new Font(parent.getDisplay(), fd);
		
		final Label text = new Label(topArea, SWT.LEFT | SWT.WRAP);
		text.setBackground(BACKGROUND_COLOR);
		text.setFont(boldFont);
		text.setText(message);
		text.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		text.addDisposeListener(new DisposeListener() {	
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				boldFont.dispose();
			}
		});
		
		Composite bottomArea = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.marginHeight = 10;
		layout.marginWidth = 10;
		bottomArea.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 500;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		bottomArea.setLayoutData(gd);
		
		final Label detailsText = new Label(bottomArea, SWT.LEFT | SWT.WRAP);
		detailsText.setText(details);
		detailsText.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, true));
		
		checkDontAskAgain = new Button(bottomArea, SWT.CHECK);
      checkDontAskAgain.setText(i18n.tr("Don't ask me again for connections to this server"));
				
		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		dontAskAgain = checkDontAskAgain.getSelection();
		super.okPressed();
	}
}
