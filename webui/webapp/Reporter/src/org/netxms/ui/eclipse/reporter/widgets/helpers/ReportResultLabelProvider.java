package org.netxms.ui.eclipse.reporter.widgets.helpers;

import java.text.SimpleDateFormat;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.reports.ReportResult;

public class ReportResultLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final long serialVersionUID = 1L;

	private final SimpleDateFormat dateFormat = new SimpleDateFormat();

	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final ReportResult reportResult = (ReportResult)element;
		switch(columnIndex)
		{
			case 0:
				return String.valueOf(reportResult.getJobId());
			case 1:
				return dateFormat.format(reportResult.getExecutionTime());
		}
		return "<INTERNAL ERROR>";
	}
}
