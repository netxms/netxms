/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.logviewer.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewPart;
import org.netxms.client.TableRow;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogRecordDetails;
import org.netxms.ui.eclipse.logviewer.views.helpers.LogLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for displaying event log record details
 */
public class NotificationLogRecordDetailsDialog extends Dialog
{
   private TableRow record;
   private Log logHandle;
   private LabeledText timestamp;
   private LabeledText status;
   private LabeledText channel;
   private LabeledText recipient;
   private LabeledText subject;   
   private StyledText message;
   private LogLabelProvider labelProvider;

   /**
    * Create dialog
    * 
    * @param parentShell parent shell
    * @param data audit log record details
    */
   public NotificationLogRecordDetailsDialog(Shell parentShell, LogRecordDetails data, TableRow record, Log logHandle, IViewPart viewPart)
   {
      super(parentShell);
      labelProvider = new LogLabelProvider(logHandle, null);
      this.logHandle = logHandle;
      this.record = record;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Notification Message");
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
      createButton(parent, IDialogConstants.OK_ID, "Close", true);
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
      dialogArea.setLayout(layout);
      
      timestamp = new LabeledText(dialogArea, SWT.NONE);
      timestamp.setLabel("Timestamp");
      timestamp.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("notification_timestamp")));
      timestamp.setEditable(false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      timestamp.setLayoutData(gd); 

      channel = new LabeledText(dialogArea, SWT.NONE);     
      channel.setLabel("Channel");
      channel.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("notification_channel")));
      channel.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      channel.setLayoutData(gd);

      status = new LabeledText(dialogArea, SWT.NONE);     
      status.setLabel("Status");
      status.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("success")));
      status.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      status.setLayoutData(gd);

      recipient = new LabeledText(dialogArea, SWT.NONE);     
      recipient.setLabel("Recipient");
      recipient.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("recipient")));
      recipient.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      recipient.setLayoutData(gd);

      subject = new LabeledText(dialogArea, SWT.NONE);     
      subject.setLabel("Subject");
      subject.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("subject")));
      subject.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      subject.setLayoutData(gd);

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Message:");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      label.setLayoutData(gd);      
      
      message = new StyledText(dialogArea, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY);
      message.setFont(JFaceResources.getTextFont());
      message.setText(labelProvider.getColumnText(record, logHandle.getColumnIndex("message")));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 500;
      gd.heightHint = 500;
      gd.horizontalSpan = 3;
      message.setLayoutData(gd);

      return dialogArea;
   }

   @Override
   public boolean close()
   {
      labelProvider.dispose();
      return super.close();
   }
   
   
}
