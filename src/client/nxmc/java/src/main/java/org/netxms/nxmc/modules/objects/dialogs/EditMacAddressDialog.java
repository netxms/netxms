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
package org.netxms.nxmc.modules.objects.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.MacAddress;
import org.netxms.base.MacAddressFormatException;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MacAddressValidator;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Edit MAC address dialog
 */
public class EditMacAddressDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EditMacAddressDialog.class);

   private LabeledText macAddrField;

   private MacAddress macAddress;

   /**
    * Constructor
    * 
    * @param newShell parent shell
    * @param macAddress initial MAC address
    */
   public EditMacAddressDialog(Shell newShell, MacAddress macAddress)
   {
      super(newShell);
      newShell.setText(i18n.tr("Edit MAC Address"));
      this.macAddress = macAddress;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      macAddrField = new LabeledText(dialogArea, SWT.NONE);
      macAddrField.setLabel(i18n.tr("MAC address"));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      macAddrField.setLayoutData(gd);
      macAddrField.getTextControl().setText(macAddress != null ? macAddress.toString() : "");
      
      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {      
      if (!WidgetHelper.validateTextInput(macAddrField, new MacAddressValidator(true)))
      {
         WidgetHelper.adjustWindowSize(this);
         return;
      }
      

      try
      {
         macAddress = macAddrField.getText().trim().isEmpty() ? new MacAddress() : MacAddress.parseMacAddress(macAddrField.getText());

         super.okPressed();
      }
      catch(MacAddressFormatException e)
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), String.format(i18n.tr("Failed to parse MAC: %s"), e.getLocalizedMessage()));
         WidgetHelper.adjustWindowSize(this);
      }
   }

   /**
    * Get updated MAC address
    * 
    * @return the macAddress
    */
   public MacAddress getMacAddress()
   {
      return macAddress;
   }
}
