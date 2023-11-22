package org.netxms.ui.eclipse.reporter.widgets.internal;

import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.reporting.ReportDefinition;
import org.netxms.ui.eclipse.reporter.Activator;

public class ReportTreeLabelProvider extends LabelProvider
{
   private Image validReportIcon;
   private Image invalidReportIcon;

	public ReportTreeLabelProvider()
	{
      validReportIcon = Activator.getImageDescriptor("icons/report.png").createImage(); //$NON-NLS-1$
      invalidReportIcon = Activator.getImageDescriptor("icons/invalid-report.png").createImage(); //$NON-NLS-1$
	}

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
	@Override
	public void dispose()
	{
      validReportIcon.dispose();
      invalidReportIcon.dispose();
	}

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
	@Override
	public Image getImage(Object element)
	{
      return ((ReportDefinition)element).isValid() ? validReportIcon : invalidReportIcon;
	}

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
	@Override
	public String getText(Object element)
	{
      return ((ReportDefinition)element).getName();
	}
}
