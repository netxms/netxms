package org.netxms.ui.eclipse.reporter.widgets.internal;

import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.reporting.ReportDefinition;
import org.netxms.ui.eclipse.reporter.Activator;

public class ReportTreeLabelProvider extends LabelProvider
{
	private Image icon;

	public ReportTreeLabelProvider()
	{
		icon = Activator.getImageDescriptor("icons/report.png").createImage(); //$NON-NLS-1$
	}

	@Override
	public void dispose()
	{
		if (icon != null)
		{
			icon.dispose();
		}
	}

	@Override
	public Image getImage(Object element)
	{
		return icon;
	}

	@Override
	public String getText(Object element)
	{
		ReportDefinition definition = (ReportDefinition)element;
		return definition.getName();
	}
}
