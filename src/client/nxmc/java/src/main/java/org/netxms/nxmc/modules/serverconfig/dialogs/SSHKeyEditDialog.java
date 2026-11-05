/**
 * NetXMS - open source network management system
 * Copyright (C) 2021-2022 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.SshKeyPair;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Import or edit key dialog
 */
public class SSHKeyEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SSHKeyEditDialog.class);

   private LabeledText textName;
   private LabeledText privateKey;
   private final SshKeyPair key;
   private final boolean isNew;
   private final String errorMessage;

   /**
    * Edit SSH key dialog with an optional error from a failed save
    *
    * @param shell shell
    * @param key key data (ID 0 means import)
    * @param errorMessage error to show inline or null
    */
   public SSHKeyEditDialog(Shell shell, SshKeyPair key, String errorMessage)
   {
      super(shell);
      this.key = key;
      isNew = this.key.getId() == 0;
      this.errorMessage = errorMessage;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell shell)
   {
      super.configureShell(shell);
      shell.setText(isNew ? i18n.tr("Import SSH Key") : i18n.tr("Edit SSH Key"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
      textName.setTextLimit(254);
      textName.setText(key.getName());
      textName.getTextControl().addModifyListener(e -> updateOkButton());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);

      if (!isNew)
      {
         LabeledText publicKey = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.WRAP | SWT.READ_ONLY);
         publicKey.setLabel(i18n.tr("Current public key"));
         publicKey.setText(key.getPublicKey());
         gd = new GridData(SWT.FILL, SWT.FILL, true, false);
         gd.heightHint = 90;
         gd.widthHint = 600;
         publicKey.setLayoutData(gd);
      }

      privateKey = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.WRAP);
      privateKey.setLabel(isNew ? i18n.tr("Private key") : i18n.tr("New private key"));
      privateKey.setText(key.getPrivateKey());
      privateKey.getTextControl().addModifyListener(e -> updateOkButton());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 200;
      gd.widthHint = 600;
      gd.verticalAlignment = SWT.FILL;
      privateKey.setLayoutData(gd);

      Label hint = new Label(dialogArea, SWT.WRAP);
      hint.setText(isNew ? i18n.tr("Public key will be derived from the private key") : i18n.tr("Leave empty to keep the current key"));
      hint.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      if (errorMessage != null)
      {
         Label error = new Label(dialogArea, SWT.WRAP);
         error.setText(errorMessage);
         error.setForeground(ThemeEngine.getForegroundColor("List.Error"));
         gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
         gd.widthHint = 600;
         error.setLayoutData(gd);
      }

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      super.createButtonsForButtonBar(parent);
      updateOkButton();
   }

   /**
    * Enable OK when required input is present
    */
   private void updateOkButton()
   {
      Button ok = getButton(IDialogConstants.OK_ID);
      if (ok != null)
         ok.setEnabled(!textName.getText().isBlank() && (!isNew || !privateKey.getText().isBlank()));
   }

   /**
    * Get updated SSH keys data
    *
    * @return updated or created SSH key data
    */
   public SshKeyPair getSshKeyData()
   {
      return key;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      key.setName(textName.getText());
      String newPrivateKey = privateKey.getText();
      key.setPrivateKey(newPrivateKey.isBlank() ? null : newPrivateKey);
      super.okPressed();
   }
}
