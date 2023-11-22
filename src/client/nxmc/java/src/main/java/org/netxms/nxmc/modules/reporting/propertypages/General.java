/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.reporting.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.ScheduledTask;
import org.netxms.client.reporting.ReportingJob;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.ScheduleEditor;

/**
 * "General" property page for reporting job schedule
 */
public class General extends PropertyPage
{
   private ReportingJob job;
   private ScheduleEditor scheduleSelector;

   public General(ReportingJob job)
   {
      super("General");
      this.job = job;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      final Composite dialogArea = new Composite(parent, SWT.NONE);
      dialogArea.setLayout(new FillLayout());

      scheduleSelector = new ScheduleEditor(dialogArea, SWT.NONE);
      scheduleSelector.setSchedule(job.getTask());

		return dialogArea;
	}

	/**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
	protected boolean applyChanges(final boolean isApply)
	{
      ScheduledTask schedule = scheduleSelector.getSchedule();
      job.getTask().setSchedule(schedule.getSchedule());
      job.getTask().setExecutionTime(schedule.getExecutionTime());
		return true;
	}
}
