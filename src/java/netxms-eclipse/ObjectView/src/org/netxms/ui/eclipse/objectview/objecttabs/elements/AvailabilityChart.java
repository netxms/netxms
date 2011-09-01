/**
 * 
 */
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.GenericObject;

/**
 * Availability chart for business services
 */
public class AvailabilityChart extends OverviewPageElement
{

	/**
	 * @param parent
	 * @param object
	 */
	public AvailabilityChart(Composite parent, GenericObject object)
	{
		super(parent, object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return "Availability";
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
	 */
	@Override
	void onObjectChange()
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardElement#createClientArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createClientArea(Composite parent)
	{
		Composite clientArea = new Composite(parent, SWT.NONE);
		
		return clientArea;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public boolean isApplicableForObject(GenericObject object)
	{
		return object instanceof BusinessService;
	}
}
