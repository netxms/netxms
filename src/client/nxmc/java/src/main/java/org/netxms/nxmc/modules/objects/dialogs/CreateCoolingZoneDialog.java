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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.CoolingZoneType;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.helpers.CoolingZoneTypeLabels;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Cooling zone object creation dialog
 */
public class CreateCoolingZoneDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(CreateCoolingZoneDialog.class);

   private LabeledText nameField;
   private LabeledText aliasField;
   private LabeledCombo zoneTypeField;
   private LabeledSpinner ratedCapacityField;

   private String name;
   private String alias;
   private CoolingZoneType zoneType;
   private int ratedCapacity;

   /**
    * @param parentShell parent shell
    */
   public CreateCoolingZoneDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Cooling Zone"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      nameField = new LabeledText(dialogArea, SWT.NONE);
      nameField.setLabel(i18n.tr("Name"));
      nameField.getTextControl().setTextLimit(255);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      nameField.setLayoutData(gd);

      aliasField = new LabeledText(dialogArea, SWT.NONE);
      aliasField.setLabel(i18n.tr("Alias"));
      aliasField.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      aliasField.setLayoutData(gd);

      zoneTypeField = new LabeledCombo(dialogArea, SWT.NONE);
      zoneTypeField.setLabel(i18n.tr("Zone type"));
      zoneTypeField.setContent(CoolingZoneTypeLabels.getAll());
      zoneTypeField.select(CoolingZoneType.OTHER.getValue());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      zoneTypeField.setLayoutData(gd);

      ratedCapacityField = new LabeledSpinner(dialogArea, SWT.NONE);
      ratedCapacityField.setLabel(i18n.tr("Rated capacity (W, 0 = undeclared)"));
      ratedCapacityField.setRange(0, Integer.MAX_VALUE);
      ratedCapacityField.setSelection(0);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      ratedCapacityField.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      name = nameField.getText().trim();
      alias = aliasField.getText().trim();
      zoneType = CoolingZoneType.getByValue(zoneTypeField.getSelectionIndex());
      ratedCapacity = ratedCapacityField.getSelection();
      if (name.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please provide non-empty object name"));
         return;
      }
      super.okPressed();
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the alias
    */
   public String getAlias()
   {
      return alias;
   }

   /**
    * @return the zone type
    */
   public CoolingZoneType getZoneType()
   {
      return zoneType;
   }

   /**
    * @return the rated capacity in watts
    */
   public int getRatedCapacity()
   {
      return ratedCapacity;
   }
}
