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
package org.netxms.nxmc.modules.nxsl.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.mt.MappingTable;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Mapping table properties dialog
 */
public class MappingTablePropertiesDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(MappingTablePropertiesDialog.class);

   private LabeledText textName;
   private LabeledText textDescription;
   private Button checkNumericKeys;
   private String name;
   private String description;
   private int flags;

   /**
    * @param parentShell parent shell
    * @param name current name
    * @param description current description
    * @param flags current flags
    */
   public MappingTablePropertiesDialog(Shell parentShell, String name, String description, int flags)
   {
      super(parentShell);
      this.name = name;
      this.description = description;
      this.flags = flags;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Mapping Table Properties"));
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
      textName.getTextControl().setTextLimit(63);
      textName.setText(name);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);

      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel(i18n.tr("Description"));
      textDescription.setText(description);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textDescription.setLayoutData(gd);

      checkNumericKeys = new Button(dialogArea, SWT.CHECK);
      checkNumericKeys.setText(i18n.tr("Numeric keys"));
      checkNumericKeys.setSelection((flags & MappingTable.NUMERIC_KEYS) != 0);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      name = textName.getText();
      description = textDescription.getText();
      flags = checkNumericKeys.getSelection() ? MappingTable.NUMERIC_KEYS : 0;
      super.okPressed();
   }

   /**
    * Get table name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get table description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * Get table flags
    */
   public int getFlags()
   {
      return flags;
   }
}
