/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.agentmanagement.dialogs;

import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.DateTimeSelector;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetFactory;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for user support application notification sending
 */
public class SendUserAgentNotificationDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SendUserAgentNotificationDialog.class);

   private LabeledText textMessage;
   private Button radioInstant;
   private Button radioDelayed;
   private Button radioStartup;
   private DateTimeSelector startDateSelector;
   private DateTimeSelector endDateSelector;
   private String message;
   private Date startTime;
   private Date endTime;
   private boolean startupNotification;

   /**
    * Constrictor
    * 
    * @param parentShell
    * @param canSendUserMessage
    */
   public SendUserAgentNotificationDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Send user support application notification"));
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      textMessage = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER);
      textMessage.setLabel(i18n.tr("Message text"));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 200;
      gd.widthHint = 600;
      gd.horizontalSpan = 2;
      textMessage.setLayoutData(gd);
      textMessage.setFocus();

      final SelectionListener listener = new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            boolean enable = radioDelayed.getSelection() || radioStartup.getSelection();
            startDateSelector.setEnabled(enable);
            endDateSelector.setEnabled(enable);
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      };

      radioInstant = new Button(dialogArea, SWT.RADIO);
      radioInstant.setText(i18n.tr("Instant"));
      radioInstant.setSelection(true);
      radioInstant.addSelectionListener(listener);
      gd = new GridData();
      gd.horizontalSpan = 2;
      radioInstant.setLayoutData(gd);

      radioDelayed = new Button(dialogArea, SWT.RADIO);
      radioDelayed.setText(i18n.tr("Delayed"));
      radioDelayed.setSelection(false);
      radioDelayed.addSelectionListener(listener);
      gd = new GridData();
      gd.horizontalSpan = 2;
      radioDelayed.setLayoutData(gd);

      radioStartup = new Button(dialogArea, SWT.RADIO);
      radioStartup.setText(i18n.tr("Startup"));
      radioStartup.setSelection(false);
      radioStartup.addSelectionListener(listener);
      gd = new GridData();
      gd.horizontalSpan = 2;
      radioStartup.setLayoutData(gd);

      final WidgetFactory factory = (p, s) -> new DateTimeSelector(p, s);

      startDateSelector = (DateTimeSelector)WidgetHelper.createLabeledControl(dialogArea, SWT.NONE, factory, i18n.tr("Valid from"),
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      startDateSelector.setValue(new Date());
      startDateSelector.setEnabled(false);

      endDateSelector = (DateTimeSelector)WidgetHelper.createLabeledControl(dialogArea, SWT.NONE, factory, i18n.tr("Valid until"),
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      endDateSelector.setValue(new Date(System.currentTimeMillis() + 86400000L));
      endDateSelector.setEnabled(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Get default user support application notification retention time"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final int defaultTime = session.getPublicServerVariableAsInt("UserAgent.DefaultMessageRetentionTime");
            runInUIThread(() -> {
               if (defaultTime > 0)
                  endDateSelector.setValue(new Date(System.currentTimeMillis() + defaultTime * 60000L));
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get default user support application notification retention time");
         }
      }.start();

      return dialogArea;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      message = textMessage.getText();
      if (radioDelayed.getSelection() || radioStartup.getSelection())
      {
         startTime = startDateSelector.getValue();
         endTime = endDateSelector.getValue();
         startupNotification = radioStartup.getSelection();
      }
      else
      {
         startTime = new Date(0);
         endTime = new Date(0);
         startupNotification = false;
      }
      super.okPressed();
   }

   /**
    * Get message
    * 
    * @return user support application notification
    */
   public String getMessage()
   {
      return message;
   }

   /**
    * Start time
    * 
    * @return user support application notification start time
    */
   public Date getStartTime()
   {
      return startTime;
   }

   /**
    * End time
    * 
    * @return user support application notification end time
    */
   public Date getEndTime()
   {
      return endTime;
   }

   public boolean isStartupNotification()
   {
      return startupNotification;
   }
}
