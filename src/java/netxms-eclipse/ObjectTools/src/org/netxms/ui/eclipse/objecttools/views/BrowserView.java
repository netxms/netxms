/**
 * 
 */
package org.netxms.ui.eclipse.objecttools.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.LocationEvent;
import org.eclipse.swt.browser.LocationListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;

/**
 * Web browser view
 */
public class BrowserView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objecttools.views.BrowserView";
	
	private Browser browser;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		browser = new Browser(parent, SWT.NONE);
		browser.addLocationListener(new LocationListener() {
			@Override
			public void changing(LocationEvent event)
			{
				setPartName("Web Browser - [" + event.location + "]");
			}
			
			@Override
			public void changed(LocationEvent event)
			{
				setPartName("Web Browser - " + event.location);
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		browser.setFocus();
	}

	/**
	 * @param url
	 */
	public void openUrl(final String url)
	{
		browser.setUrl(url);
	}
}
