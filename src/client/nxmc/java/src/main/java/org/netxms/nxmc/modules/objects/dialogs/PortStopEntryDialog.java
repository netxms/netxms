/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.PortStopEntry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for editing port stop list entry
 */
public class PortStopEntryDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(PortStopEntryDialog.class);

   private PortStopEntry entry;
   private Spinner portSpinner;
   private Combo protocolCombo;

   /**
    * Create dialog for new entry or editing existing entry.
    *
    * @param parentShell parent shell
    * @param entry entry to edit (null for new entry)
    */
   public PortStopEntryDialog(Shell parentShell, PortStopEntry entry)
   {
      super(parentShell);
      this.entry = entry;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((entry == null) ? i18n.tr("Add Port Stop Entry") : i18n.tr("Edit Port Stop Entry"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Port:"));

      portSpinner = new Spinner(dialogArea, SWT.BORDER);
      portSpinner.setMinimum(1);
      portSpinner.setMaximum(65535);
      portSpinner.setSelection((entry != null) ? entry.getPort() : 1);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 100;
      portSpinner.setLayoutData(gd);

      label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Protocol:"));

      protocolCombo = new Combo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY);
      protocolCombo.add("TCP");
      protocolCombo.add("UDP");
      protocolCombo.add("TCP/UDP");
      if (entry != null)
      {
         switch(entry.getProtocol())
         {
            case PortStopEntry.PROTOCOL_TCP:
               protocolCombo.select(0);
               break;
            case PortStopEntry.PROTOCOL_UDP:
               protocolCombo.select(1);
               break;
            case PortStopEntry.PROTOCOL_BOTH:
               protocolCombo.select(2);
               break;
            default:
               protocolCombo.select(2);
               break;
         }
      }
      else
      {
         protocolCombo.select(2); // Default to TCP/UDP
      }
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      protocolCombo.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      int port = portSpinner.getSelection();
      if (port < 1 || port > 65535)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Port number must be between 1 and 65535"));
         return;
      }

      char protocol;
      switch(protocolCombo.getSelectionIndex())
      {
         case 0:
            protocol = PortStopEntry.PROTOCOL_TCP;
            break;
         case 1:
            protocol = PortStopEntry.PROTOCOL_UDP;
            break;
         default:
            protocol = PortStopEntry.PROTOCOL_BOTH;
            break;
      }

      if (entry == null)
      {
         entry = new PortStopEntry(port, protocol);
      }
      else
      {
         entry.setPort(port);
         entry.setProtocol(protocol);
      }

      super.okPressed();
   }

   /**
    * Get entry (result of editing).
    *
    * @return the entry
    */
   public PortStopEntry getEntry()
   {
      return entry;
   }
}
