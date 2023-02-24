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
package org.netxms.nxmc.modules.objects.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog to create/edit maintenance journal entry
 */
public class MaintenanceJournalEditDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(MaintenanceJournalEditDialog.class);
   private String objectName;
   private String description;
   private LabeledText descriptionText;

   /**
    * Physical link add/edit dialog constructor
    * 
    * @param parentShell
    * @param link link to edit or null if new should be created
    */
   public MaintenanceJournalEditDialog(Shell parentShell, String objectName, String description)
   {
      super(parentShell);
      this.objectName = objectName;
      this.description = (description != null) ? description : "";
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(description.isEmpty() ? i18n.tr("Create Maintenance Journal Entry") : i18n.tr("Edit Maintenance Journal Entry"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.RESIZE);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      GridData gd = new GridData();
      gd.widthHint = 700;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;

      LabeledText objectText = new LabeledText(dialogArea, SWT.NONE);
      objectText.setLabel(i18n.tr("Object"));
      objectText.setText(objectName);
      objectText.setLayoutData(gd);
      objectText.getTextControl().setEditable(false);

      gd = new GridData();
      gd.heightHint = 400;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;

      descriptionText = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER | SWT.WRAP | SWT.V_SCROLL);
      descriptionText.setLabel(i18n.tr("Description"));
      descriptionText.setText(description);
      descriptionText.setLayoutData(gd);
      descriptionText.setFocus();

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (description.equals(descriptionText.getText()))
      {
         super.cancelPressed();
      }
      else
      {
         description = descriptionText.getText();
         super.okPressed();
      }
   }

   /**
    * Get description
    * 
    * @return description string
    */
   public String getDescription()
   {
      return description;
   }
}
