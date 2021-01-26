/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
package org.netxms.ui.eclipse.serverconfig.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.SshKeyData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Import or edit key dialog
 */
public class EditSshKeysDialog extends Dialog
{
   private LabeledText textName;
   private LabeledText publicKey;
   private LabeledText privateKey;
   private Button replaceKeysButton;
   private SshKeyData key;
   private boolean isNew;
   
   /**
    * Edit SSH key dialog constructor
    *    
    * @param shell shell
    * @param key key data or null if new key
    */
   public EditSshKeysDialog(Shell shell, SshKeyData key)
   {
      super(shell);
      if (key == null)
      {
         this.key = new SshKeyData();
         isNew = true;
      }
      else
      {
         this.key = key;
         isNew = false;
      }
   }

   /* (non-Javadoc)
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
      textName.setLabel("Name");
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
         replaceKeysButton.setText("Replace keys");
         replaceKeysButton.addSelectionListener(new SelectionListener() {
            
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               publicKey.setEditable(replaceKeysButton.getSelection());
               privateKey.setEditable(replaceKeysButton.getSelection());               
            }
            
            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               widgetSelected(e);
            }
         });
      }
      
      publicKey = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI  | SWT.V_SCROLL | SWT.WRAP);
      publicKey.setLabel("Public keys (Optional)");
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
      privateKey.setLabel("Private keys");
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
   public SshKeyData getSshKeyData()
   {
      return key;
   }


   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if(textName.getText().isEmpty() || 
            ((isNew || replaceKeysButton.getSelection()) && (publicKey.getText().isEmpty() || privateKey.getText().isEmpty()))) 
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "All fields should be filled");
         return;
      }
      
      key.setName(textName.getText());
      if (isNew || replaceKeysButton.getSelection())
      {
         key.setPublicKey(publicKey.getText());
         key.setPrivateKey(privateKey.getText());         
      }
      super.okPressed();
   }   
}
