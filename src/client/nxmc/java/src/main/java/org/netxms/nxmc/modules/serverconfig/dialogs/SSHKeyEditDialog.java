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
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.SshKeyPair;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Import or edit key dialog
 */
public class SSHKeyEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SSHKeyEditDialog.class);

   private LabeledText textName;
   private LabeledText publicKey;
   private LabeledText privateKey;
   private Button replaceKeysButton;
   private SshKeyPair key;
   private boolean isNew;

   /**
    * Edit SSH key dialog constructor
    * 
    * @param shell shell
    * @param key key data or null if new key
    */
   public SSHKeyEditDialog(Shell shell, SshKeyPair key)
   {
      super(shell);
      if (key == null)
      {
         this.key = new SshKeyPair();
         isNew = true;
      }
      else
      {
         this.key = key;
         isNew = false;
      }
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
      textName.setText(key.getName());
      textName.getTextControl().setTextLimit(255);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);

      if (!isNew)
      {
         replaceKeysButton = new Button(dialogArea, SWT.CHECK);
         replaceKeysButton.setText(i18n.tr("Replace keys"));
         replaceKeysButton.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               publicKey.setEditable(replaceKeysButton.getSelection());
               privateKey.setEditable(replaceKeysButton.getSelection());
            }
         });
      }

      publicKey = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.WRAP);
      publicKey.setLabel(i18n.tr("Public key"));
      publicKey.setText(key.getPublicKey());
      publicKey.setEditable(isNew);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 200;
      gd.widthHint = 600;
      gd.verticalAlignment = SWT.FILL;
      publicKey.setLayoutData(gd);

      privateKey = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.WRAP);
      privateKey.setLabel(i18n.tr("Private key"));
      privateKey.setText("");
      privateKey.setEditable(isNew);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 200;
      gd.widthHint = 600;
      gd.verticalAlignment = SWT.FILL;
      privateKey.setLayoutData(gd);

      return dialogArea;
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
      if (textName.getText().isEmpty() || ((isNew || replaceKeysButton.getSelection()) && (publicKey.getText().isEmpty() || privateKey.getText().isEmpty())))
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("All fields should be filled"));
         return;
      }

      key.setName(textName.getText());
      if (isNew || replaceKeysButton.getSelection())
      {
         key.setKeys(publicKey.getText(), privateKey.getText());
      }
      super.okPressed();
   }
}
