/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.dialogs;

import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.NotificationChannel;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Notification channel dialog
 */
public class NotificationChannelPropertiesDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(NotificationChannelPropertiesDialog.class);

   private NotificationChannel nc;
   private LabeledText textName;
   private LabeledText textDescription;
   private LabeledText textConfiguraiton;
   private Combo comboDriverName;
   private boolean customName;

   /**
    * Create new notification channel properties dialog.
    *
    * @param parentShell parent shell
    * @param nc notification channel to edit
    */
   public NotificationChannelPropertiesDialog(Shell parentShell, NotificationChannel nc)
   {
      super(parentShell);
      this.nc = nc;
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

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      comboDriverName = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Driver"), gd);
      comboDriverName.addModifyListener(listener -> {
         final int selectionIndex = comboDriverName.getSelectionIndex();
         if (selectionIndex != -1 && !customName)
         {
            textName.setText(comboDriverName.getItem(selectionIndex));
            textName.getTextControl().selectAll();
         }
      });

      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
      textName.getTextControl().setTextLimit(63);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      textName.getTextControl().addModifyListener(listener -> {
         final String name = textName.getText();

         final int selectionIndex = comboDriverName.getSelectionIndex();
         if (selectionIndex != -1)
         {
            final String driver = comboDriverName.getItem(selectionIndex);
            customName = !name.equals(driver);
         }

         Control button = getButton(IDialogConstants.OK_ID);
         if (button != null)
         {
            button.setEnabled(!name.trim().isBlank());
         }
      });

      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel(i18n.tr("Description"));
      textDescription.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textDescription.setLayoutData(gd);
      
      textConfiguraiton = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER);
      textConfiguraiton.setLabel(i18n.tr("Driver configuration"));
      textConfiguraiton.getTextControl().setFont(JFaceResources.getTextFont());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 300;
      gd.widthHint = 900;
      textConfiguraiton.setLayoutData(gd);

      if (nc != null)
      {
         textName.setText(nc.getName());
         textDescription.setText(nc.getDescription());
         textConfiguraiton.setText(nc.getConfiguration());
      }
      
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Get driver names"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<String> ncList = session.getNotificationDrivers();
            Collections.sort(ncList, String.CASE_INSENSITIVE_ORDER);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  updateUI(ncList);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get driver names");
         }
      }.start(); 
      
      return dialogArea;
   }

   private void updateUI(List<String> ncList)
   {
      int index = 0;
      if (!ncList.isEmpty()) {
         for(int i = 0; i < ncList.size(); i++)
         {
            String item = ncList.get(i);
            comboDriverName.add(item);
            
            if (nc != null && item.equals(nc.getDriverName())) {
               customName = !textName.getText().equals(nc.getDriverName());
               index = i;
            }
         }
         comboDriverName.select(index);
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(nc != null ? i18n.tr("Update notification channel") : i18n.tr("Create notification channel"));
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if(textName.getText().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Notification channel name should not be empty"));
         return;
      }
      
      if(comboDriverName.getSelectionIndex() == -1)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Notification driver should be selected"));
         return;
      }
      
      if(nc == null)
      {
         nc = new NotificationChannel();
      }

      nc.setName(textName.getText());
      nc.setDescription(textDescription.getText());
      nc.setDriverName(comboDriverName.getItem(comboDriverName.getSelectionIndex()));
      nc.setConfiguration(textConfiguraiton.getText());      
      super.okPressed();
   }
   
   /**
    * Return updated notification channel
    */
   public NotificationChannel getNotificaiotnChannel()
   {
      return nc;
   }
}
