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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Create SSH key name
 */
public class CerateKeyNameDialog extends Dialog
{
   private LabeledText textName;
   private String name;
   
   /**
    * Create SSH key name dialog constructor
    */
   public CerateKeyNameDialog(Shell shell)
   {
      super(shell);
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
      textName.setLabel(Messages.get().CreateMappingTableDialog_Name);
      textName.getTextControl().setTextLimit(255);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      
      return dialogArea;
   }
   
   /**
    * Returns selected name
    * 
    * @return selected name 
    */
   public String getName()
   {
      return name;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Generate SSH key");
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if(textName.getText().isEmpty()) 
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Name should not be empty");
         return;
      }
      
      name = textName.getText();
      super.okPressed();
   }
}
