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
package org.netxms.nxmc.modules.datacollection.dialogs;

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
 * Confirmation dialog for file deletion from file delivery policy
 */
public class FileDeleteConfirmationDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(FileDeleteConfirmationDialog.class);

   private boolean deleteFromAgents;
   private Button radioDeleteFromAgents;
   private Button radioPolicyOnly;

   /**
    * @param parentShell parent shell
    */
   public FileDeleteConfirmationDialog(Shell parentShell)
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
      newShell.setText(i18n.tr("Delete Files"));
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
      label.setText(i18n.tr("You are about to remove files from the file delivery policy.\nPlease select how to handle files already deployed to agents:"));
      label.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      radioPolicyOnly = new Button(controls, SWT.RADIO);
      radioPolicyOnly.setText(i18n.tr("Remove from &policy only"));
      radioPolicyOnly.setSelection(true);
      GridData gd = new GridData();
      gd.horizontalIndent = 10;
      radioPolicyOnly.setLayoutData(gd);

      radioDeleteFromAgents = new Button(controls, SWT.RADIO);
      radioDeleteFromAgents.setText(i18n.tr("Remove and &delete from agents"));
      radioDeleteFromAgents.setSelection(false);
      gd = new GridData();
      gd.horizontalIndent = 10;
      radioDeleteFromAgents.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      deleteFromAgents = radioDeleteFromAgents.getSelection();
      super.okPressed();
   }

   /**
    * Check if files should be deleted from agents.
    *
    * @return true if files should be deleted from agents
    */
   public boolean isDeleteFromAgents()
   {
      return deleteFromAgents;
   }
}
