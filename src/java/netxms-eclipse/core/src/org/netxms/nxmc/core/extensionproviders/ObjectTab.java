/**
 * Generic info tab for object browser
 */

package org.netxms.nxmc.core.extensionproviders;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.*;
import org.netxms.nxmc.core.Activator;

public abstract class ObjectTab
{
	private boolean isShown = false;
	
	protected CTabFolder tabFolder;
	protected Image image;
	protected CTabItem tabItem;
	protected NXCObject currentObject;
	

	/**
	 * Returns image for tab
	 */
	
	protected ImageDescriptor getImageDescriptor()
	{
		return Activator.getImageDescriptor("icons/alt_window_16.gif");
	}
	
	
	/**
	 * Returns name for tab
	 */
	
	protected String getTabName()
	{
		return "Default";
	}
	
	
	/**
	 * Returns control for the tab
	 */
	
	protected Composite getControl()
	{
		return null;
	}
	
	
	/**
	 * Create new object info tab
	 * 
	 * @param tabFolder parent tab folder
	 */
	
	protected ObjectTab(final CTabFolder tabFolder)
	{
		this.tabFolder = tabFolder;
		image = getImageDescriptor().createImage();
		currentObject = null;
	}
	
	
	/**
	 * Dispose tab
	 */
	
	public void dispose()
	{
		if (image != null)
		{
			image.dispose();
			image = null;
		}
	}
	
	
	/**
	 * Check if tab is appropriate for given object
	 */

	public boolean isValidForObject(final NXCObject object)
	{
		return true;
	}
	
	
	/**
	 * Set new active object
	 */
	
	final public void setObject(final NXCObject object)
	{
		currentObject = object;
		onObjectChange();
	}
	
	
	/**
	 * Handler for object change
	 */
	
	protected void onObjectChange()
	{		
	}
	
	
	/**
	 * Show tab
	 */

	public void show()
	{
		if (isShown)
			return;
		
		tabItem = new CTabItem(tabFolder, SWT.NONE);
		tabItem.setText(getTabName());
		tabItem.setImage(image);
		tabItem.setControl(getControl());
		isShown = true;
	}
	
	
	/**
	 * Hide tab
	 */
	
	public void hide()
	{
		if (!isShown)
			return;
		
		tabItem.setControl(null);
		tabItem.dispose();
		isShown = false;
	}
	
	
	/**
	 * Dispose object at garbage collection time if it was not disposed previously
	 */
	
	@Override
	protected void finalize()
	{
		dispose();
	}


	/**
	 * @return the image
	 */
	public final Image getImage()
	{
		return image;
	}


	/**
	 * @return the tabItem
	 */
	public final CTabItem getTabItem()
	{
		return tabItem;
	}
}
