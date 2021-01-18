package org.netxms.ui.eclipse.reporter.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.ScheduledTask;
import org.netxms.client.reporting.ReportingJob;
import org.netxms.ui.eclipse.widgets.ScheduleSelector;

/**
 * "General" property page for reporting job schedule
 */
public class General extends PropertyPage
{
	public static final String ID = "org.netxms.ui.eclipse.reporter.propertypages.General"; //$NON-NLS-1$
	
   private ReportingJob job = null;
   private ScheduleSelector scheduleSelector;
	
   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		job = (ReportingJob)getElement().getAdapter(ReportingJob.class);

      final Composite dialogArea = new Composite(parent, SWT.NONE);
      dialogArea.setLayout(new FillLayout());

      scheduleSelector = new ScheduleSelector(dialogArea, SWT.NONE);
      scheduleSelector.setSchedule(job.getTask());

		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
      ScheduledTask schedule = scheduleSelector.getSchedule();
      job.getTask().setSchedule(schedule.getSchedule());
      job.getTask().setExecutionTime(schedule.getExecutionTime());
		return true;
	}
	
   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
