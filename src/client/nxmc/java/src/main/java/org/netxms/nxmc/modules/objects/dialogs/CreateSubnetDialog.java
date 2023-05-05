/**
 * NetXMS - open source network management system
 * Copyright (C) 2021-2023 Raden Solutions
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

import java.net.Inet4Address;
import java.net.InetAddress;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.InetAddressEx;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.IPAddressValidator;
import org.netxms.nxmc.tools.IPNetMaskValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * New subnet creation dialog
 */
public class CreateSubnetDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(CreateSubnetDialog.class);

   private String objectName = "";
   private String objectAlias = "";
   private InetAddressEx ipAddress;
   
   private LabeledText objectNameText;
   private LabeledText objectAliasText;
   private LabeledText ipAddressText;
   private LabeledText maskText;
   
   /**
    * Dialog constructor
    * 
    * @param shell
    */
   public CreateSubnetDialog(Shell shell)
   {
      super(shell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Subnet"));
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      objectNameText = new LabeledText(dialogArea, SWT.NONE);
      objectNameText.setLabel(i18n.tr("Name (leave empty to construct from address and mask)"));
      objectNameText.getTextControl().setTextLimit(255);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 600;
      gd.horizontalSpan = 2;
      objectNameText.setLayoutData(gd);
      objectNameText.setText(objectName);
      
      objectAliasText = new LabeledText(dialogArea, SWT.NONE);
      objectAliasText.setLabel(i18n.tr("Alias"));
      objectAliasText.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      objectAliasText.setLayoutData(gd);
      objectAliasText.setText(objectAlias);

      ipAddressText = new LabeledText(dialogArea, SWT.NONE);
      ipAddressText.setLabel(i18n.tr("IP address"));
      ipAddressText.getTextControl().setTextLimit(64);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      ipAddressText.setLayoutData(gd);
      ipAddressText.setText("");

      maskText = new LabeledText(dialogArea, SWT.NONE);
      maskText.setLabel(i18n.tr("Network mask"));
      maskText.getTextControl().setTextLimit(16);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      maskText.setLayoutData(gd);
      maskText.setText("");

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (!WidgetHelper.validateTextInput(ipAddressText, new IPAddressValidator(false)) || !WidgetHelper.validateTextInput(maskText, new IPNetMaskValidator(false, ipAddressText.getText())))
      {
         WidgetHelper.adjustWindowSize(this);
         return;
      }

      try
      {
         objectName = objectNameText.getText().trim();
         objectAlias = objectAliasText.getText().trim();
         InetAddress addr = InetAddress.getByName(ipAddressText.getText().trim());
         int maskBits;
         try
         {
            maskBits = Integer.parseInt(maskText.getText().trim());
         }
         catch(NumberFormatException e)
         {
            InetAddress mask = InetAddress.getByName(maskText.getText().trim());
            maskBits = InetAddressEx.bitsInMask(mask);
         }
         if (maskBits > (addr instanceof Inet4Address ? 31 : 127))
         {
            maskText.setErrorMessage("Invalid network mask");
            WidgetHelper.adjustWindowSize(this);
            return;
         }
         ipAddress = new InetAddressEx(addr, maskBits);
      }
      catch (Exception e)
      {
         MessageDialog.openError(getShell(), "Error", String.format("Internal error: %s", e.getMessage()));
         return;
      }

      super.okPressed();
   }

   /**
    * @return the objectName
    */
   public String getObjectName()
   {
      return objectName;
   }

   /**
    * @return the objectAlias
    */
   public String getObjectAlias()
   {
      return objectAlias;
   }

   /**
    * @return the ipAddress
    */
   public InetAddressEx getIpAddress()
   {
      return ipAddress;
   }
}
