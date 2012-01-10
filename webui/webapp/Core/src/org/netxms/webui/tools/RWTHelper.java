/**
 * 
 */
package org.netxms.webui.tools;

import org.eclipse.rwt.RWT;
import org.eclipse.rwt.lifecycle.UICallBack;
import org.eclipse.swt.widgets.Display;

/**
 * RWT helper methods
 */
public class RWTHelper
{
	private static class AttributeReader implements Runnable
	{
		public String name;
		public Object value;
		
		public AttributeReader(String name)
		{
			this.name = name;
		}
		
		@Override
		public void run()
		{
			value = RWT.getSessionStore().getAttribute(name);
		}
	}
	
	/**
	 * Get session attribute from non-UI thread
	 * 
	 * @param name attribute's name
	 * @return attribute's value
	 */
	public static Object getSessionAttribute(final Display display, final String name)
	{
		AttributeReader r = new AttributeReader(name);
		UICallBack.runNonUIThreadWithFakeContext(display, r);
		return r.value;
	}
	
	/**
	 * Set session attribute from non-UI thread
	 * 
	 * @param name
	 * @param value
	 */
	public static void setSessionAttribute(final Display display, final String name, final Object value)
	{
		UICallBack.runNonUIThreadWithFakeContext(display, new Runnable() {
			@Override
			public void run()
			{
				RWT.getSessionStore().setAttribute(name, value);
			}
		});
	}
}
