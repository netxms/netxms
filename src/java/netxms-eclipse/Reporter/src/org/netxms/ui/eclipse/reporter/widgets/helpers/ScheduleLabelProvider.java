package org.netxms.ui.eclipse.reporter.widgets.helpers;

import java.util.Calendar;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.api.client.reporting.ReportingJob;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.reporter.widgets.ReportExecutionForm;

/**
 * Label provider for scheduled reports list
 */
public class ScheduleLabelProvider extends LabelProvider implements ITableLabelProvider 
{
	private String[] dayOfWeek = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };

	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final ReportingJob job = (ReportingJob)element;
		switch (columnIndex)
		{
			case ReportExecutionForm.SCHEDULE_START_TIME:
				Calendar dateTime = Calendar.getInstance();
				dateTime.setTime(job.getStartTime());

				String dateTimeStr = String.valueOf(dateTime.get(Calendar.HOUR_OF_DAY)) + ":"
						+ (dateTime.get(Calendar.MINUTE) < 10 ? "0" : "") + String.valueOf(dateTime.get(Calendar.MINUTE));
				switch (job.getType())
				{
					case ReportingJob.EXECUTE_ONCE:
						return RegionalSettings.getDateTimeFormat().format(job.getStartTime());
					case ReportingJob.EXECUTE_DAILY:
						return dateTimeStr;
					case ReportingJob.EXECUTE_MONTHLY:
						String result = "";
						for (int i = 0; i < 31; i++)
						{
							if (((job.getDaysOfMonth() >> i) & 0x01) != 0)
								result = String.valueOf(31 - i) + (result.length() > 0 ? "," : "") + result;
						}
						return dateTimeStr + " - " + result;
					case ReportingJob.EXECUTE_WEEKLY:
						String result1 = "";
						for (int i = 0; i < 7; i++)
						{
							if (((job.getDaysOfWeek() >> i) & 0x01) != 0)
								result1 = dayOfWeek[7 - (i + 1)] + (result1.length() > 0 ? "," : "") + result1;
						}
						return dateTimeStr + " - " + result1;
					default:
						return "<error>";
				}
			case ReportExecutionForm.SCHEDULE_TYPE:
				switch (job.getType())
				{
					case ReportingJob.EXECUTE_ONCE:
						return "once";
					case ReportingJob.EXECUTE_DAILY:
						return "daily";
					case ReportingJob.EXECUTE_MONTHLY:
						return "monthly";
					case ReportingJob.EXECUTE_WEEKLY:
						return "weekly";
					default:
						return "<error>";
				}
			case ReportExecutionForm.SCHEDULE_START_TIME_OFFSET:
				if (job.getStartTimeOffset() == 0 && job.getEndTimeOffset() == 0)
					return RegionalSettings.getDateTimeFormat().format(job.getTimeFrom());
				else
					return offsetToString(job.getStartTimeOffset());
			case ReportExecutionForm.SCHEDULE_END_TIME_OFFSET:
				if (job.getEndTimeOffset() == 0 && job.getStartTimeOffset() == 0)
					return RegionalSettings.getDateTimeFormat().format(job.getTimeTo());
				else
					return offsetToString(job.getEndTimeOffset());
		}

		return "<INTERNAL ERROR>";
	}

	/**
	 * Converter from ms to Strind (example: 0 Month, 7 days)
	 * 
	 * @return
	 */
	private String offsetToString(long offset)
	{
		boolean positive = (offset >= 0 ? true : false);
		offset = Math.abs(offset);
		String result = "";
		if (offset > 60000L * 60L * 24L * 365L) // year
		{
			int year = (int) Math.ceil(offset / (60000L * 60L * 24L * 365L));
			result += String.valueOf(year) + " years ";
			offset -= (year * 60000L * 60L * 24L * 365L);
		}
		if (offset > 60000L * 60L * 24L) // days
		{
			int days = (int) Math.ceil(offset / (60000L * 60L * 24L));
			result += String.valueOf(days) + " days ";
			offset = offset - (days * 60000L * 60L * 24L);
		}

		if (offset > 60000L) // minutes
		{
			int hours = (int) Math.ceil(offset / 60000L);
			result += String.valueOf(hours) + " minutes ";
			offset -= hours * 60000L;
		}

		return (!positive ? "-" : (offset != 0 ? "+" : "")) + result;
	}
}
