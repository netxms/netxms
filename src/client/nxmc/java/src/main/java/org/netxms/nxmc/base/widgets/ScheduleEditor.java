/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.base.widgets;

import java.util.Date;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.ScheduledTask;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Editor for task schedule
 */
public class ScheduleEditor extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(ScheduleEditor.class);

   private Button radioOneTimeSchedule;
   private Button radioRecurrent;
   private DateTimeSelector execDateSelector;
   private Text textSchedule; 

	/**
	 * @param parent
	 * @param style
	 */
	public ScheduleEditor(Composite parent, int style)
	{
		super(parent, style);

		GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.horizontalSpacing = 16;
      layout.makeColumnsEqualWidth = false;
      layout.numColumns = 2;
      setLayout(layout);

      final SelectionListener listener = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            execDateSelector.setEnabled(radioOneTimeSchedule.getSelection());
            textSchedule.setEnabled(radioRecurrent.getSelection());
         }
      };

      radioOneTimeSchedule = new Button(this, SWT.RADIO);
      radioOneTimeSchedule.setText(i18n.tr("One time execution"));
      radioOneTimeSchedule.setSelection(true);
      radioOneTimeSchedule.addSelectionListener(listener);
      
      execDateSelector = new DateTimeSelector(this, SWT.NONE);
      execDateSelector.setValue(new Date());

      radioRecurrent = new Button(this, SWT.RADIO);
      radioRecurrent.setText(i18n.tr("Recurrent"));
      radioRecurrent.setSelection(false);
      radioRecurrent.addSelectionListener(listener);

      textSchedule = new Text(this, SWT.BORDER);
      textSchedule.setTextLimit(255);
      textSchedule.setText("");
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
	   radioRecurrent.setEnabled(enabled);
      if (enabled)
	   {
         execDateSelector.setEnabled(radioOneTimeSchedule.getSelection());
         textSchedule.setEnabled(radioRecurrent.getSelection());
	   }
	   else 
	   {
         execDateSelector.setEnabled(false);
         textSchedule.setEnabled(false);
	   }
	}

   /**
    * Get configured schedule as dummy task
    *
    * @return configured schedule as dummy task
    */
   public ScheduledTask getSchedule()
   {
      return new ScheduledTask("Dummy", radioRecurrent.getSelection() ? textSchedule.getText() : "", "", "", execDateSelector.getValue(), 0, 0); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
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
         radioRecurrent.setSelection(true);
         textSchedule.setEnabled(true);
         execDateSelector.setEnabled(false);
         textSchedule.setText(scheduledTask.getSchedule());
      }
   }
}
