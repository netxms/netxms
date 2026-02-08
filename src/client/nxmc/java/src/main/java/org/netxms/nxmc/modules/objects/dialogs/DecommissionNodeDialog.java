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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.Date;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.DateTimeSelector;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for decommissioning a node
 */
public class DecommissionNodeDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(DecommissionNodeDialog.class);

   private int defaultExpirationDays;
   private Date expirationTime;
   private boolean clearIpAddresses;
   private DateTimeSelector expirationTimeSelector;
   private Button checkClearIpAddresses;

   /**
    * Constructor
    *
    * @param parentShell parent shell
    * @param defaultExpirationDays default expiration time in days
    */
   public DecommissionNodeDialog(Shell parentShell, int defaultExpirationDays)
   {
      super(parentShell);
      this.defaultExpirationDays = defaultExpirationDays;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Decommission Node"));
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
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      Label labelExpirationTime = new Label(dialogArea, SWT.NONE);
      labelExpirationTime.setText(i18n.tr("Expiration time"));

      expirationTimeSelector = new DateTimeSelector(dialogArea, SWT.NONE);
      Date defaultExpiration = new Date(System.currentTimeMillis() + (long)defaultExpirationDays * 24 * 60 * 60 * 1000);
      expirationTimeSelector.setValue(defaultExpiration);
      expirationTimeSelector.setToolTipText(i18n.tr("Node will be automatically deleted after this time"));

      checkClearIpAddresses = new Button(dialogArea, SWT.CHECK);
      checkClearIpAddresses.setText(i18n.tr("Clear IP addresses from node and interfaces"));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      checkClearIpAddresses.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      expirationTime = expirationTimeSelector.getValue();
      clearIpAddresses = checkClearIpAddresses.getSelection();

      if (expirationTime.before(new Date()))
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Expiration time must be in the future!"));
         return;
      }

      super.okPressed();
   }

   /**
    * Get expiration time.
    *
    * @return expiration time
    */
   public Date getExpirationTime()
   {
      return expirationTime;
   }

   /**
    * Check if IP addresses should be cleared.
    *
    * @return true if IP addresses should be cleared from node and interfaces
    */
   public boolean isClearIpAddresses()
   {
      return clearIpAddresses;
   }
}
