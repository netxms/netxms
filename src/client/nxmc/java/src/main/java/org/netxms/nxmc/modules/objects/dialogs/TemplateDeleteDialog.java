/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Reden Solutions
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
 * Dialog for confirming template deletion and selecting DCI handling
 */
public class TemplateDeleteDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(TemplateDeleteDialog.class);

   private String templateName;
   private boolean removeDci;
   private Button radioRemove;
   private Button radioUnbind;

   /**
    * Constructor
    *
    * @param parentShell parent shell
    * @param templateName name of template being deleted
    */
   public TemplateDeleteDialog(Shell parentShell, String templateName)
   {
      super(parentShell);
      this.templateName = templateName;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Delete Template"));
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
      label.setText(String.format(i18n.tr("You are about to delete template \"%s\". Please select how to handle DCIs created from this template on all bound objects:"), templateName));
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 400;
      label.setLayoutData(gd);

      radioRemove = new Button(controls, SWT.RADIO);
      radioRemove.setText(i18n.tr("&Remove DCIs from all objects"));
      radioRemove.setSelection(true);
      gd = new GridData();
      gd.horizontalIndent = 10;
      radioRemove.setLayoutData(gd);

      radioUnbind = new Button(controls, SWT.RADIO);
      radioUnbind.setText(i18n.tr("&Unbind DCIs from template (keep as standalone)"));
      radioUnbind.setSelection(false);
      gd = new GridData();
      gd.horizontalIndent = 10;
      radioUnbind.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      removeDci = radioRemove.getSelection();
      super.okPressed();
   }

   /**
    * Check if DCIs should be removed on template deletion
    *
    * @return true if DCIs should be removed, false if they should be detached
    */
   public boolean isRemoveDci()
   {
      return removeDci;
   }
}
