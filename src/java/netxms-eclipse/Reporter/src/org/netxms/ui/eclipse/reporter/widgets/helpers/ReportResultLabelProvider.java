package org.netxms.ui.eclipse.reporter.widgets.helpers;

import java.text.SimpleDateFormat;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.api.client.reporting.ReportResult;

public class ReportResultLabelProvider extends LabelProvider implements ITableLabelProvider
{
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
				return dateFormat.format(reportResult.getExecutionTime());
			case 1:
				return reportResult.getUserName();
		}
		return "<INTERNAL ERROR>";
	}
}
