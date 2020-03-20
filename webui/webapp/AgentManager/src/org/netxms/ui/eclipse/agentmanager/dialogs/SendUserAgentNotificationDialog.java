/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.ui.eclipse.agentmanager.dialogs;

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
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.DateTimeSelector;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for user agent notification sending
 */
public class SendUserAgentNotificationDialog extends Dialog
{
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Send user support application notification");
	}

	/* (non-Javadoc)
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
      textMessage.setLabel("Message text");
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
      radioInstant.setText("Instant");
      radioInstant.setSelection(true);
      radioInstant.addSelectionListener(listener);
      gd = new GridData();
      gd.horizontalSpan = 2;
      radioInstant.setLayoutData(gd);
      
      radioDelayed = new Button(dialogArea, SWT.RADIO);
      radioDelayed.setText("Delayed");
      radioDelayed.setSelection(false);
      radioDelayed.addSelectionListener(listener);
      gd = new GridData();
      gd.horizontalSpan = 2;
      radioDelayed.setLayoutData(gd);
      
      radioStartup = new Button(dialogArea, SWT.RADIO);
      radioStartup.setText("Startup");
      radioStartup.setSelection(false);
      radioStartup.addSelectionListener(listener);
      gd = new GridData();
      gd.horizontalSpan = 2;
      radioStartup.setLayoutData(gd);

      final WidgetFactory factory = new WidgetFactory() {
         @Override
         public Control createControl(Composite parent, int style)
         {
            return new DateTimeSelector(parent, style);
         }
      };

      startDateSelector = (DateTimeSelector)WidgetHelper.createLabeledControl(dialogArea, SWT.NONE, factory, "Valid from", WidgetHelper.DEFAULT_LAYOUT_DATA);
      startDateSelector.setValue(new Date());      
      startDateSelector.setEnabled(false);

      endDateSelector = (DateTimeSelector)WidgetHelper.createLabeledControl(dialogArea, SWT.NONE, factory, "Valid until", WidgetHelper.DEFAULT_LAYOUT_DATA);
      endDateSelector.setValue(new Date(System.currentTimeMillis() + 86400000L));
      endDateSelector.setEnabled(false);

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Get default user agent notification retention time", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {     
            final int defaultTime = session.getPublicServerVariableAsInt("UserAgent.DefaultMessageRetentionTime");
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (defaultTime > 0)
                     endDateSelector.setValue(new Date(System.currentTimeMillis() + defaultTime * 60000L));
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get default user agent notification retention time";
         }
      }.start();
      
		return dialogArea;
	}

	/* (non-Javadoc)
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
	 * @return user agent notification
	 */
   public String getMessage()
   {
      return message;
   }

   /**
    * Start time
    * 
    * @return user agent notification start time
    */
   public Date getStartTime()
   {
      return startTime;
   }

   /**
    * End time
    * 
    * @return user agent notification end time
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
