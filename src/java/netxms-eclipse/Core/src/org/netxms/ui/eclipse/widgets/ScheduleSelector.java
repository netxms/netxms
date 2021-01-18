/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.widgets;

import java.util.Date;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.ScheduledTask;
import org.netxms.ui.eclipse.console.Messages;

/**
 * Date/time selection widget
 */
public class ScheduleSelector extends Composite
{
   private Button radioOneTimeSchedule;
   private Button radioCronSchedule;
   private DateTimeSelector execDateSelector;
   private Text textSchedule; 
	
	/**
	 * @param parent
	 * @param style
	 */
	public ScheduleSelector(Composite parent, int style)
	{
		super(parent, style);

		GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.horizontalSpacing = 16;
      layout.makeColumnsEqualWidth = false;
      layout.numColumns = 2;
      setLayout(layout);

      final SelectionListener listener = new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            execDateSelector.setEnabled(radioOneTimeSchedule.getSelection());
            textSchedule.setEnabled(radioCronSchedule.getSelection());
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      };

      radioOneTimeSchedule = new Button(this, SWT.RADIO);
      radioOneTimeSchedule.setText(Messages.get().ScheduleSelector_OneTimeExecution);
      radioOneTimeSchedule.setSelection(true);
      radioOneTimeSchedule.addSelectionListener(listener);
      
      execDateSelector = new DateTimeSelector(this, SWT.NONE);
      execDateSelector.setValue(new Date());
      
      radioCronSchedule = new Button(this, SWT.RADIO);
      radioCronSchedule.setText(Messages.get().ScheduleSelector_CronSchedule);
      radioCronSchedule.setSelection(false);
      radioCronSchedule.addSelectionListener(listener);
      
      textSchedule = new Text(this, SWT.BORDER);
      textSchedule.setTextLimit(255);
      textSchedule.setText(""); //$NON-NLS-1$
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textSchedule.setLayoutData(gd);
	}

   /**
    * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
    */
	@Override
	public void setEnabled(boolean enabled)
	{
		super.setEnabled(enabled);
	   radioOneTimeSchedule.setEnabled(enabled);
	   radioCronSchedule.setEnabled(enabled);
	   if(enabled == true)
	   {
         execDateSelector.setEnabled(radioOneTimeSchedule.getSelection());
         textSchedule.setEnabled(radioCronSchedule.getSelection());
	   }
	   else 
	   {
	      execDateSelector.setEnabled(enabled);
	      textSchedule.setEnabled(enabled);
	   }
	}

   /**
    * Get configured schedule as dummy task
    *
    * @return configured schedule as dummy task
    */
   public ScheduledTask getSchedule()
   {
      return new ScheduledTask("Upload.File", radioCronSchedule.getSelection() ? textSchedule.getText() : "","", "", execDateSelector.getValue(), 0, 0); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
   }

   /**
    * Set schedule configuration from task object.
    *
    * @param scheduledTask task object to get schedule configuration from
    */
   public void setSchedule(ScheduledTask scheduledTask)
   {
      if (scheduledTask.getSchedule().isEmpty())
      {
         radioOneTimeSchedule.setSelection(true);
         textSchedule.setEnabled(false); 
         execDateSelector.setEnabled(true);
         execDateSelector.setValue(scheduledTask.getExecutionTime());        
      }
      else
      {
         radioOneTimeSchedule.setSelection(false);
         radioCronSchedule.setSelection(true);
         textSchedule.setEnabled(true);
         execDateSelector.setEnabled(false);
         textSchedule.setText(scheduledTask.getSchedule());
      }
   }
}
