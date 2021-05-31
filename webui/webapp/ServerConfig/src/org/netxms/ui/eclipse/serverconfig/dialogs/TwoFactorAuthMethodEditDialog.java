/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.ui.eclipse.serverconfig.dialogs;

import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.users.TwoFactorAuthenticationMethod;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Two-factor authentication method edit dialog
 */
public class TwoFactorAuthMethodEditDialog extends Dialog
{
   private TwoFactorAuthenticationMethod method;
   private LabeledText textName;
   private LabeledText textDescription;
   private LabeledText textConfiguraiton;
   private Combo comboDriverName;
   private boolean isNameChanged;
   private String newName;

   /**
    * Create notification channel edit dialog.
    *
    * @param parentShell parent shell
    * @param channel notification channel to edit
    */
   public TwoFactorAuthMethodEditDialog(Shell parentShell, TwoFactorAuthenticationMethod method)
   {
      super(parentShell);
      this.method = method;
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
      textName.setLabel("Name");
      textName.getTextControl().setTextLimit(63);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      
      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel("Description");
      textDescription.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textDescription.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      comboDriverName = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Driver name", gd);
      
      textConfiguraiton = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER);
      textConfiguraiton.setLabel("Driver Configuration");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 300;
      gd.widthHint = 900;
      textConfiguraiton.setLayoutData(gd);          

      if (method != null)
      {
         textName.setText(method.getName());
         textDescription.setText(method.getDescription());
         textConfiguraiton.setText(method.getConfiguration());
      }

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Get driver names", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<String> drivers = session.get2FADrivers();
            Collections.sort(drivers, String.CASE_INSENSITIVE_ORDER);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  updateDriverList(drivers);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of two-factor authentication drivers";
         }
      }.start(); 
      
      return dialogArea;
   }

   /**
    * Update driver list.
    *
    * @param drivers new driver list
    */
   private void updateDriverList(List<String> drivers)
   {
      comboDriverName.removeAll();
      for(int i = 0; i < drivers.size(); i++)
      {
         comboDriverName.add(drivers.get(i));
         if (method != null && drivers.get(i).equals(method.getDriver()))
            comboDriverName.select(i);
      }
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(method != null ? "Edit Method" : "Create Method");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (textName.getText().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Method name should not be empty");
         return;
      }

      if (comboDriverName.getSelectionIndex() == -1)
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Driver should be selected");
         return;
      }

      if (method == null)
      {
         method = new TwoFactorAuthenticationMethod(textName.getText(), textDescription.getText(), comboDriverName.getItem(comboDriverName.getSelectionIndex()), textConfiguraiton.getText());
      }
      else
      {
         if (!method.getName().equals(textName.getText()))
         {
            isNameChanged = true;
            newName = textName.getText();
         }
         method.setDescription(textDescription.getText());
         method.setDriver(comboDriverName.getItem(comboDriverName.getSelectionIndex()));
         method.setConfiguration(textConfiguraiton.getText());
      }

      super.okPressed();
   }

   /**
    * Return updated notification channel
    */
   public TwoFactorAuthenticationMethod getMethod()
   {
      return method;
   }

   /**
    * Returns if name is changed
    */
   public boolean isNameChanged()
   {
      return isNameChanged;
   }

   /**
    * Get new name
    */
   public String getNewName()
   {
      return newName;
   }
}
