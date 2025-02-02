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
package org.netxms.nxmc.modules.logviewer.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.TableRow;
import org.netxms.client.log.Log;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.views.helpers.LogLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for displaying event log record details
 */
public class NotificationLogRecordDetailsDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(NotificationLogRecordDetailsDialog.class);

   private TableRow record;
   private Log logHandle;
   private LabeledText timestamp;
   private LabeledText status;
   private LabeledText channel;
   private LabeledText recipient;
   private LabeledText subject;
   private LabeledText eventCode;
   private LabeledText eventId;
   private LabeledText ruleId;
   private LabeledText message;
   private LogLabelProvider labelProvider;

   /**
    * Create dialog
    * 
    * @param parentShell parent shell
    * @param data audit log record details
    */
   public NotificationLogRecordDetailsDialog(Shell parentShell, TableRow record, Log logHandle)
   {
      super(parentShell);
      this.logHandle = logHandle;
      this.record = record;
      this.labelProvider = new LogLabelProvider(logHandle, null);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Notification"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#isResizable()
    */
   @Override
   protected boolean isResizable()
   {
      return true;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.OK_ID, i18n.tr("Close"), true);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      timestamp = new LabeledText(dialogArea, SWT.NONE);
      timestamp.setLabel(i18n.tr("Timestamp"));
      timestamp.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("notification_timestamp")));
      timestamp.setEditable(false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      timestamp.setLayoutData(gd); 

      channel = new LabeledText(dialogArea, SWT.NONE);     
      channel.setLabel(i18n.tr("Channel"));
      channel.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("notification_channel")));
      channel.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      channel.setLayoutData(gd);

      status = new LabeledText(dialogArea, SWT.NONE);     
      status.setLabel(i18n.tr("Status"));
      status.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("success")));
      status.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      status.setLayoutData(gd);

      recipient = new LabeledText(dialogArea, SWT.NONE);     
      recipient.setLabel(i18n.tr("Recipient"));
      recipient.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("recipient")));
      recipient.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      recipient.setLayoutData(gd);

      subject = new LabeledText(dialogArea, SWT.NONE);     
      subject.setLabel(i18n.tr("Subject"));
      subject.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("subject")));
      subject.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      subject.setLayoutData(gd);

      eventCode = new LabeledText(dialogArea, SWT.NONE);
      eventCode.setLabel(i18n.tr("Event type"));
      eventCode.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("event_code")));
      eventCode.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      eventCode.setLayoutData(gd);

      eventId = new LabeledText(dialogArea, SWT.NONE);
      eventId.setLabel(i18n.tr("Event ID"));
      eventId.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("event_id")));
      eventId.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      eventId.setLayoutData(gd);

      ruleId = new LabeledText(dialogArea, SWT.NONE);
      ruleId.setLabel(i18n.tr("Rule ID"));
      ruleId.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("rule_id")));
      ruleId.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      ruleId.setLayoutData(gd);

      message = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      message.setLabel(i18n.tr("Message"));
      message.setFont(JFaceResources.getTextFont());
      message.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("message")));
      message.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 600;
      gd.heightHint = 300;
      gd.horizontalSpan = 3;
      message.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#close()
    */
   @Override
   public boolean close()
   {
      labelProvider.dispose();
      return super.close();
   }
}
