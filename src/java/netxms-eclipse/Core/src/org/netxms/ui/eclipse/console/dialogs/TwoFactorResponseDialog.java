/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.PasswordInputField;
import org.netxms.ui.eclipse.widgets.QRLabel;

/**
 * Dialog for entering user response for selected two factor authentication method
 */
public class TwoFactorResponseDialog extends Dialog
{
   private String challenge;
   private String qrText;
   private String response;
   private LabeledText responseText;

   /**
    * Create two-factor response dialog.
    *
    * @param parentShell parent shell
    * @param challenge challenge provided by server
    * @param qrText text to be displayed as QR code
    */
   public TwoFactorResponseDialog(Shell parentShell, String challenge, String qrText)
   {
      super(parentShell);
      this.challenge = challenge;
      this.qrText = qrText;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Two-factor Authentication");
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
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      if ((qrText != null) && !qrText.isEmpty())
      {
         Label notification = new Label(dialogArea, SWT.NONE);
         notification.setText("New secret was generated. Please scan it and use for this and subsequent logins.");

         QRLabel qrLabel = new QRLabel(dialogArea, SWT.BORDER);
         qrLabel.setText(qrText);
         GridData gd = new GridData(SWT.CENTER, SWT.CENTER, true, true);
         gd.minimumWidth = 300;
         gd.minimumHeight = 300;
         qrLabel.setLayoutData(gd);

         PasswordInputField secretText = new PasswordInputField(dialogArea, SWT.NONE);
         secretText.setLabel("Secret as text");
         secretText.setText(qrText);
         secretText.setEditable(false);
         gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
         gd.widthHint = 400;
         secretText.setLayoutData(gd);
      }

      if ((challenge != null) && !challenge.isEmpty())
      {
         LabeledText challengeText = new LabeledText(dialogArea, SWT.NONE);
         challengeText.setLabel("Challenge");
         challengeText.setText(challenge);
         challengeText.getTextControl().setEditable(false);
         challengeText.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }

      responseText = new LabeledText(dialogArea, SWT.NONE);
      responseText.setLabel("Response");
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = 400;
      responseText.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      response = responseText.getText().trim();
      super.okPressed();
   }

   /**
    * Get user's response.
    *
    * @return user's response
    */
   public String getResponse()
   {
      return response;
   }
}
