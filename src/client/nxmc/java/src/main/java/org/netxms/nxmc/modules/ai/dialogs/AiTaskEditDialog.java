/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.ai.dialogs;

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
 * AI Task edit dialog
 */
public class AiTaskEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(AiTaskEditDialog.class);
   
   private LabeledText textDescription;
   private LabeledText textPrompt;
   private String description;
   private String prompt;
   private boolean isNew;

   /**
    * Create new AI task dialog.
    *
    * @param parentShell parent shell
    * @param description task description (null for new task)
    * @param prompt task prompt (null for new task)
    * @param isNew true if creating new task
    */
   public AiTaskEditDialog(Shell parentShell, String description, String prompt, boolean isNew)
   {
      super(parentShell);
      this.description = description;
      this.prompt = prompt;
      this.isNew = isNew;
   }

   /**
    * Create new AI task dialog for creating new task.
    *
    * @param parentShell parent shell
    */
   public AiTaskEditDialog(Shell parentShell)
   {
      this(parentShell, null, null, true);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(isNew ? i18n.tr("Create AI Task") : i18n.tr("Edit AI Task"));
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

      // Description field
      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel(i18n.tr("Description"));
      textDescription.getTextControl().setTextLimit(256);
      if (description != null)
      {
         textDescription.setText(description);
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      textDescription.setLayoutData(gd);

      // Prompt field (multiline)
      textPrompt = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.WRAP);
      textPrompt.setLabel(i18n.tr("Prompt"));
      if (prompt != null)
      {
         textPrompt.setText(prompt);
      }
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      gd.widthHint = 400;
      textPrompt.setLayoutData(gd);

      if (description == null)
         textDescription.setFocus();
      else
         textPrompt.setFocus();

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      description = textDescription.getText().trim();
      prompt = textPrompt.getText().trim();
      super.okPressed();
   }

   /**
    * Get task description.
    *
    * @return task description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * Get task prompt.
    *
    * @return task prompt
    */
   public String getPrompt()
   {
      return prompt;
   }
}
