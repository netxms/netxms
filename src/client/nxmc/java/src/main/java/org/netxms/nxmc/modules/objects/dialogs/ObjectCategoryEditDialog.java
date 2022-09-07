/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Victor Kirhenshtein
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
import org.netxms.client.objects.MutableObjectCategory;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.widgets.ImageSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for editing object category
 */
public class ObjectCategoryEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectCategoryEditDialog.class);

   private MutableObjectCategory category;
   private LabeledText name;
   private ImageSelector icon;
   private ImageSelector mapImage;

   /**
    * Create new dialog.
    *
    * @param parentShell parent shell
    * @param category category object to edit or null if new category to be created
    */
   public ObjectCategoryEditDialog(Shell parentShell, MutableObjectCategory category)
   {
      super(parentShell);
      this.category = category;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((category != null) ? i18n.tr("Edit Object Category") : i18n.tr("Create Object Category"));
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

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Name"));
      name.getTextControl().setTextLimit(63);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      name.setLayoutData(gd);

      icon = new ImageSelector(dialogArea, SWT.NONE);
      icon.setLabel(i18n.tr("Icon"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      icon.setLayoutData(gd);
      icon.setParentShell(getShell());

      mapImage = new ImageSelector(dialogArea, SWT.NONE);
      mapImage.setLabel(i18n.tr("Map image"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      mapImage.setLayoutData(gd);
      mapImage.setParentShell(getShell());

      if (category != null)
      {
         name.setText(category.getName());
         icon.setImageGuid(category.getIcon(), false);
         mapImage.setImageGuid(category.getMapImage(), false);
      }

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (name.getText().trim().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Category name cannot be empty!"));
         return;
      }

      if (category == null)
      {
         category = new MutableObjectCategory(name.getText().trim(), icon.getImageGuid(), mapImage.getImageGuid());
      }
      else
      {
         category.setName(name.getText().trim());
         category.setIcon(icon.getImageGuid());
         category.setMapImage(mapImage.getImageGuid());
      }

      super.okPressed();
   }

   /**
    * @return the category
    */
   public MutableObjectCategory getCategory()
   {
      return category;
   }
}
