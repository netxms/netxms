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
package org.netxms.nxmc.modules.alarms.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog box for showing AI assistant comment for alarm
 */
public class AssistantCommentDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(AssistantCommentDialog.class);
   private static final String CONFIG_PREFIX = "AssistantCommentDialog";

	private LabeledText textControl;
	private String text;

	/**
	 * @param parentShell
	 */
   public AssistantCommentDialog(Shell parentShell, String text)
	{
		super(parentShell);
		this.text = text;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#isResizable()
    */
	@Override
	protected boolean isResizable()
	{
		return true;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("AI Assistant Comment"));

		PreferenceStore settings = PreferenceStore.getInstance();
		newShell.setSize(settings.getAsInteger(CONFIG_PREFIX + ".cx", 400), settings.getAsInteger(CONFIG_PREFIX + ".cy", 350));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);
      textControl = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.WRAP);
		textControl.setLabel(i18n.tr("Comment"));
      textControl.setText(text);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		textControl.setLayoutData(gd);

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.OK_ID, i18n.tr("&Add as comment"), true);
      createButton(parent, IDialogConstants.CANCEL_ID, i18n.tr("Close"), false);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		text = textControl.getText();
		saveSettings();
		super.okPressed();
	}

	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
      Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(CONFIG_PREFIX + ".cx", size.x);
      settings.set(CONFIG_PREFIX + ".cy", size.y);
	}

	/**
	 * @return the text
	 */
	public String getText()
	{
		return text;
	}
}
