/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.ScheduledTask;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Date/time selection widget
 */
public class ScheduleSelector extends Composite
{
   private Button radioOneTimeSchedule;
   private Button radioCronSchedule;
   private Group scheduleGroup;
   private DateTimeSelector execDateSelector;
   private Text textSchedule; 
	
	/**
	 * @param parent
	 * @param style
	 */
	public ScheduleSelector(Composite parent, int style)
	{
		super(parent, style);
      
      setLayout(new FillLayout());
		
		scheduleGroup = new Group(parent, SWT.NONE);
		scheduleGroup.setText("Schedule");
      
		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.OUTER_SPACING;
      layout.horizontalSpacing = 16;
      layout.makeColumnsEqualWidth = false;
      layout.numColumns = 2;
      scheduleGroup.setLayout(layout);
      
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

      radioOneTimeSchedule = new Button(scheduleGroup, SWT.RADIO);
      radioOneTimeSchedule.setText("One time execution");
      radioOneTimeSchedule.setSelection(true);
      radioOneTimeSchedule.addSelectionListener(listener);
      
      final WidgetFactory factory = new WidgetFactory() {
         @Override
         public Control createControl(Composite parent, int style)
         {
            return new DateTimeSelector(parent, style);
         }
      };
      
      execDateSelector = (DateTimeSelector)WidgetHelper.createLabeledControl(scheduleGroup, SWT.NONE, factory, "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      execDateSelector.setValue(new Date());
      
      radioCronSchedule = new Button(scheduleGroup, SWT.RADIO);
      radioCronSchedule.setText("Cron schedule");
      radioCronSchedule.setSelection(false);
      radioCronSchedule.addSelectionListener(listener);
      
      textSchedule = new Text(scheduleGroup, SWT.BORDER);
      textSchedule.setTextLimit(255);
      textSchedule.setText("");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textSchedule.setLayoutData(gd);
	}

	/* (non-Javadoc)
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


   /* (non-Javadoc)
    * @see org.eclipse.swt.widgets.Control#setVisible(boolean)
    */
   @Override
   public void setVisible(boolean visible)
   {
      super.setVisible(visible);
      scheduleGroup.setVisible(visible);
   }

   public ScheduledTask getSchedule()
   {
      return new ScheduledTask("Upload.File", radioCronSchedule.getSelection() ? textSchedule.getText() : "","", execDateSelector.getValue(), 0, 0);
   }

   public void setSchedule(ScheduledTask scheduledTask)
   {
      if(scheduledTask.getSchedule().isEmpty())
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
