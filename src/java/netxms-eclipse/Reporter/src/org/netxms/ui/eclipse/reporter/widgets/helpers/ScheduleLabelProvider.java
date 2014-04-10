package org.netxms.ui.eclipse.reporter.widgets.helpers;

import java.text.SimpleDateFormat;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.api.client.reporting.ReportingJob;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.reporter.widgets.ReportExecutionForm;

public class ScheduleLabelProvider extends LabelProvider implements ITableLabelProvider {
	private String[] dayOfWeek = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$ //$NON-NLS-6$ //$NON-NLS-7$

	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final ReportingJob job = (ReportingJob) element;
		switch (columnIndex)
		{
			case ReportExecutionForm.SCHEDULE_START_TIME:
				SimpleDateFormat timeFormat = new SimpleDateFormat("HH:mm");
				String HHmm = timeFormat.format(job.getStartTime().getTime() / 1000);
				
				switch (job.getType())
				{
					case ReportingJob.TYPE_ONCE:
						return RegionalSettings.getDateTimeFormat().format(job.getStartTime().getTime() / 1000);
					case ReportingJob.TYPE_DAILY:
						return HHmm;
					case ReportingJob.TYPE_MONTHLY:
						String result = ""; //$NON-NLS-1$
						for (int i = 0; i < 31; i++)
						{
							if (((job.getDaysOfMonth() >> i) & 0x01) != 0)
								result = String.valueOf(31 - i) + (result.length() > 0 ? "," : "") + result; //$NON-NLS-1$ //$NON-NLS-2$
						}
						return HHmm + " - " + result; //$NON-NLS-1$
					case ReportingJob.TYPE_WEEKLY:
						String result1 = ""; //$NON-NLS-1$
						for (int i = 0; i < 7; i++)
						{
							if (((job.getDaysOfWeek() >> i) & 0x01) != 0)
								result1 = dayOfWeek[7 - (i + 1)] + (result1.length() > 0 ? "," : "") + result1; //$NON-NLS-1$ //$NON-NLS-2$
						}
						return HHmm + " - " + result1; //$NON-NLS-1$
					default:
						return "<error>";
				}
			case ReportExecutionForm.SCHEDULE_TYPE:
				switch (job.getType())
				{
					case ReportingJob.TYPE_ONCE:
						return "once";
					case ReportingJob.TYPE_DAILY:
						return "daily";
					case ReportingJob.TYPE_MONTHLY:
						return "monthly";
					case ReportingJob.TYPE_WEEKLY:
						return "weekly";
					default:
						return "<error>";
				}
			case ReportExecutionForm.SCHEDULE_TIME_FROM:
				if (job.getTimeFrom() != null)
				{
					String val[] = job.getTimeFrom().split(";");
					if (val.length == 5)
					{
						System.out.print(val[2]);
						return String.format("%s.%s.%s %s:%s", (val[2].length() < 2 ? "0" + val[2] : val[2]), 
								(val[1].length() < 2 ? "0" + val[1] : val[1]), val[0], (val[3].length() < 2 ? "0" + val[3] : val[3]), 
								(val[4].length() < 2 ? "0" + val[4] : val[4]));
					}
				}
				break;
			case ReportExecutionForm.SCHEDULE_TIME_TO:
				if (job.getTimeTo() != null)
				{
					String val[] = job.getTimeTo().split(";");
					if (val.length == 5)
					{
						return String.format("%s.%s.%s %s:%s", (val[2].length() < 2 ? "0" + val[2] : val[2]), 
								(val[1].length() < 2 ? "0" + val[1] : val[1]), val[0], (val[3].length() < 2 ? "0" + val[3] : val[3]), 
								(val[4].length() < 2 ? "0" + val[4] : val[4]));
					}
				}
				break;
		}

		return "<INTERNAL ERROR>";
	}
}
