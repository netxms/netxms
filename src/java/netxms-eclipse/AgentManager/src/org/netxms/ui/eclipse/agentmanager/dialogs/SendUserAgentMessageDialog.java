/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.DateTimeSelector;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for user agent message sending
 */
public class SendUserAgentMessageDialog extends Dialog
{
   private LabeledText textMessage;
   private Button radioOneTime;
   private Button radioInterval;
   private DateTimeSelector startDateSelector;
   private Group retentionGroup;
   private LabeledSpinner days;
   private LabeledSpinner hours;
   private LabeledSpinner minutes;
   private String message;
   private Date startTime;
   private Date endTime;   
	
	/**
	 * Constrictor
	 * 
	 * @param parentShell
	 * @param canSendUserMessage 
	 */
	public SendUserAgentMessageDialog(Shell parentShell)
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
		newShell.setText("Send user agent message");
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
		textMessage.setLabel("Message text:");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      //gd.widthHint = 200;
      gd.heightHint = 200;
      gd.horizontalSpan = 2;
      textMessage.setLayoutData(gd);
      textMessage.setFocus();
      
      final SelectionListener listener = new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            startDateSelector.setEnabled(radioInterval.getSelection());
            days.setEnabled(radioInterval.getSelection());
            hours.setEnabled(radioInterval.getSelection());
            minutes.setEnabled(radioInterval.getSelection());
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      };
      
      radioOneTime = new Button(dialogArea, SWT.RADIO);
      radioOneTime.setText("One time");
      radioOneTime.setSelection(true);
      
      radioInterval = new Button(dialogArea, SWT.RADIO);
      radioInterval.setText("Interval");
      radioInterval.setSelection(false);
      radioInterval.addSelectionListener(listener);
      
      final WidgetFactory factory = new WidgetFactory() {
         @Override
         public Control createControl(Composite parent, int style)
         {
            return new DateTimeSelector(parent, style);
         }
      };
      
      startDateSelector = (DateTimeSelector)WidgetHelper.createLabeledControl(dialogArea, SWT.NONE, factory, "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$
      startDateSelector.setValue(new Date());      

      
      retentionGroup = new Group(dialogArea, SWT.NONE);
      retentionGroup.setText(Messages.get().ScheduleSelector_Schedule);
      
      layout = new GridLayout();
      layout.marginWidth = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.OUTER_SPACING;
      layout.horizontalSpacing = 16;
      layout.makeColumnsEqualWidth = false;
      layout.numColumns = 3;
      retentionGroup.setLayout(layout);
      
      days = new LabeledSpinner(retentionGroup, SWT.NONE);
      days.setLabel("Days");
      GridData gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      days.setLayoutData(gridData);
      days.setRange(0, 999);
      days.setSelection(0);
      
      hours = new LabeledSpinner(retentionGroup, SWT.NONE);
      hours.setLabel("Hours");
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      hours.setLayoutData(gridData);
      hours.setRange(0, 23);
      hours.setSelection(0);
      
      minutes = new LabeledSpinner(retentionGroup, SWT.NONE);
      minutes.setLabel("Minutes");
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      minutes.setLayoutData(gridData);
      minutes.setRange(0, 59);
      minutes.setSelection(0);

      startDateSelector.setEnabled(false);
      days.setEnabled(false);
      hours.setEnabled(false);
      minutes.setEnabled(false);
      
      final NXCSession session = ConsoleSharedData.getSession();
      
      new ConsoleJob("Get default user agent message retention time", null, Activator.PLUGIN_ID, null) {
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {     
            final int defaultTime = session.getPublicServerVariableAsInt("UserAgent.DefaultMessageRetentionTime");
            
            runInUIThread(new Runnable() {
               
               @Override
               public void run()
               {
                  if(defaultTime > 0)
                  {
                     int d = defaultTime/60/24;
                     int h = (defaultTime)/60 - d*24;
                     int m = defaultTime - (d * 24 + h) * 60;
   
                     days.setSelection(d);
                     hours.setSelection(h);
                     minutes.setSelection(m);
                  }
               }
            });
            
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Error getting user agent default retention time";
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
	   if(radioInterval.getSelection())
	   {
   	   startTime = startDateSelector.getValue();
   	   endTime = new Date(new Date().getTime() + (days.getSelection() * 86400 + hours.getSelection() * 3600 + minutes.getSelection() * 60)*1000);
	   }
	   else
	   {
	      startTime = new Date(0);
	      endTime = new Date(0);
	   }
		super.okPressed();
	}

	/**
	 * Get message 
	 * 
	 * @return user agent message
	 */
   public String getMessage()
   {
      return message;
   }

   /**
    * Start time
    * 
    * @return user agent message start time
    */
   public Date getStartTime()
   {
      return startTime;
   }

   /**
    * End time
    * 
    * @return user agent message end time
    */
   public Date getEndTime()
   {
      return endTime;
   }
}
