/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.PasswordInputField;
import org.netxms.nxmc.base.widgets.QRLabel;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for entering user response for selected two factor authentication method
 */
public class TwoFactorResponseDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(TwoFactorResponseDialog.class);

   private String challenge;
   private String qrText;
   private boolean trustedDevicesAllowed;
   private String response;
   private boolean trustedDevice;
   private LabeledText responseText;
   private Button checkTrustedDevice;

   /**
    * Create two-factor response dialog.
    *
    * @param parentShell parent shell
    * @param challenge challenge provided by server
    * @param qrText text to be displayed as QR code
    * @param trustedDevicesAllowed true if server allows trusted devices
    */
   public TwoFactorResponseDialog(Shell parentShell, String challenge, String qrText, boolean trustedDevicesAllowed)
   {
      super(parentShell);
      this.challenge = challenge;
      this.qrText = qrText;
      this.trustedDevicesAllowed = trustedDevicesAllowed;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Two-factor Authentication"));
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
         notification.setText(i18n.tr("New secret was generated. Please scan it and use for this and subsequent logins."));

         QRLabel qrLabel = new QRLabel(dialogArea, SWT.BORDER);
         qrLabel.setText(qrText);
         GridData gd = new GridData(SWT.CENTER, SWT.CENTER, true, true);
         gd.minimumWidth = 300;
         gd.minimumHeight = 300;
         qrLabel.setLayoutData(gd);

         PasswordInputField secretText = new PasswordInputField(dialogArea, SWT.NONE, true);
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
         challengeText.setLabel(i18n.tr("Challenge"));
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

      if (trustedDevicesAllowed)
      {
         checkTrustedDevice = new Button(dialogArea, SWT.CHECK);
         checkTrustedDevice.setText(i18n.tr("&Don't ask again on this computer"));
      }

      responseText.setFocus();
      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      response = responseText.getText().trim();
      trustedDevice = trustedDevicesAllowed ? checkTrustedDevice.getSelection() : false;
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

   /**
    * Get "trusted device" flag.
    *
    * @return "trusted device" flag
    */
   public boolean isTrustedDevice()
   {
      return trustedDevice;
   }
}
