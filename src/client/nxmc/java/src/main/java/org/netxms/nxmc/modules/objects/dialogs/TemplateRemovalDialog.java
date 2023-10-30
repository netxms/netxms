/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Reden Solutions
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
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog used to select related object(s) of given template for removal 
 */
public class TemplateRemovalDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(TemplateRemovalDialog.class);

   private boolean removeDci;
   private Button radioRemove; 
   private Button radioUnbind;

   /**
    * Constructor
    * 
    * @param parentShell parent shell
    * @param seedObject object used for populating related object list
    * @param relationType relation type
    * @param classFilter class filter for object list
    */
   public TemplateRemovalDialog(Shell parentShell)
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
      newShell.setText(i18n.tr("Remove template from object"));
   }


   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      Composite controls = new Composite(dialogArea, SWT.NONE);
      controls.setLayout(new GridLayout());
      controls.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      final Label label = new Label(controls, SWT.WRAP);
      label.setText(i18n.tr("You are about to remove data collection template from a node. Please select how to deal with DCIs related to this template:"));
      label.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      radioRemove = new Button(controls, SWT.RADIO);
      radioRemove.setText(i18n.tr("&Remove DCIs from node"));
      radioRemove.setSelection(true);
      GridData gd = new GridData();
      gd.horizontalIndent = 10;
      radioRemove.setLayoutData(gd);

      radioUnbind = new Button(controls, SWT.RADIO);
      radioUnbind.setText(i18n.tr("&Unbind DCIs from template"));
      radioUnbind.setSelection(false);
      gd = new GridData();
      gd.horizontalIndent = 10;
      radioUnbind.setLayoutData(gd);
      
      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.dialogs.RelatedObjectSelectionDialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      removeDci = radioRemove.getSelection();
      super.okPressed();
   }

   /**
    * If DCIs should be removed on template removal
    * 
    * @return if DCIs should be removed on template removal
    */
   public boolean isRemoveDci()
   {
      return removeDci;
   }

}
