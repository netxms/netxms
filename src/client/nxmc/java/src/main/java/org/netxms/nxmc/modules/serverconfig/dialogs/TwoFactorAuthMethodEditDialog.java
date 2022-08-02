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
package org.netxms.nxmc.modules.serverconfig.dialogs;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.util.Collections;
import java.util.List;
import java.util.Properties;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.NotificationChannel;
import org.netxms.client.users.TwoFactorAuthenticationMethod;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Two-factor authentication method edit dialog
 */
public class TwoFactorAuthMethodEditDialog extends Dialog
{
   private static final Logger logger = LoggerFactory.getLogger(TwoFactorAuthMethodEditDialog.class);

   private final I18n i18n = LocalizationHelper.getI18n(TwoFactorAuthMethodEditDialog.class);

   private TwoFactorAuthenticationMethod method;
   private LabeledText textName;
   private LabeledText textDescription;
   private Combo comboDriverName;
   private Composite driverConfigurationArea;
   private Combo comboChannelName;
   private boolean isNameChanged;
   private String newName;
   private Properties driverConfiguration = new Properties();
   private List<NotificationChannel> notificationChannels = null;

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
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(method != null ? i18n.tr("Edit Method") : i18n.tr("Create Method"));
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
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      
      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel(i18n.tr("Description"));
      textDescription.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textDescription.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      comboDriverName = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Driver"), gd);
      comboDriverName.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            updateDriverConfigurationWidgets();
         }
      });

      if (method != null)
      {
         textName.setText(method.getName());
         textDescription.setText(method.getDescription());
         try (ByteArrayInputStream in = new ByteArrayInputStream(method.getConfiguration().getBytes("UTF-8")))
         {
            driverConfiguration.load(in);
         }
         catch(Exception e)
         {
            logger.error("Cannot parse 2FA driver configuration", e);
         }
      }

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Get two-factor authentication method driver names"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<String> drivers = session.get2FADrivers();
            Collections.sort(drivers, String.CASE_INSENSITIVE_ORDER);

            notificationChannels = session.getNotificationChannels();
            notificationChannels.sort((NotificationChannel c1, NotificationChannel c2) -> c1.getName().compareTo(c2.getName()));

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
            return i18n.tr("Cannot get list of two-factor authentication method drivers");
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
         {
            comboDriverName.select(i);
            updateDriverConfigurationWidgets();
         }
      }

      if ((comboDriverName.getSelectionIndex() == -1) && (comboDriverName.getItemCount() > 0))
      {
         comboDriverName.select(0);
         updateDriverConfigurationWidgets();
      }
   }

   /**
    * Update configuration widget(s) for selected driver
    */
   private void updateDriverConfigurationWidgets()
   {
      if (driverConfigurationArea != null)
         driverConfigurationArea.dispose();

      String driverName = comboDriverName.getText();
      if (driverName.equals("Message"))
      {
         driverConfigurationArea = new Composite((Composite)dialogArea, SWT.NONE);
         driverConfigurationArea.setLayout(new FillLayout());
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         driverConfigurationArea.setLayoutData(gd);

         comboChannelName = WidgetHelper.createLabeledCombo(driverConfigurationArea, SWT.READ_ONLY, i18n.tr("Notification channel"), null);
         for(NotificationChannel n : notificationChannels)
         {
            comboChannelName.add(n.getName());
            if (n.getName().equals(driverConfiguration.getProperty("ChannelName", "")))
               comboChannelName.select(comboChannelName.getItemCount() - 1);
         }

         if ((comboChannelName.getSelectionIndex() == -1) && (comboChannelName.getItemCount() > 0))
            comboChannelName.select(0);
      }
      else
      {
         driverConfigurationArea = null;
      }

      getShell().layout(true, true);
      getShell().pack();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (textName.getText().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Method name should not be empty"));
         return;
      }

      String driverName = comboDriverName.getText();
      if (driverName.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Driver should be selected"));
         return;
      }

      // Process driver-specific configuration
      if (driverName.equals("Message"))
      {
         driverConfiguration.setProperty("ChannelName", comboChannelName.getText());
      }

      String configuration = "";
      try (ByteArrayOutputStream out = new ByteArrayOutputStream())
      {
         driverConfiguration.store(out, null);
         configuration = new String(out.toByteArray(), "UTF-8");
      }
      catch(Exception e)
      {
         logger.error("Error serializing 2FA driver configuration", e);
      }

      if (method == null)
      {
         method = new TwoFactorAuthenticationMethod(textName.getText(), textDescription.getText(), driverName, configuration);
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
         method.setConfiguration(configuration);
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
