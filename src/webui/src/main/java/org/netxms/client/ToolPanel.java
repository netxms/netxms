/**
 * 
 */
package org.netxms.client;

import com.google.gwt.user.client.Event;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.Panel;
import com.google.gwt.user.client.ui.StackPanel;
import com.google.gwt.user.client.ui.Widget;

/**
 * @author Victor
 *
 */
public class ToolPanel extends StackPanel 
{
	private Panel workarea;
	
	/**
	 * 
	 */
	public ToolPanel()
	{
		super();
		add(new Label("Dashboards"), "<table><tr valign=\"middle\"><td><img src=\"toolpad/dashboard.png\"></td><td>Dashboards</td></tr></table>", true);
		add(new Label("Objects"), "<table><tr valign=\"middle\"><td><img src=\"toolpad/objects.png\"></td><td>Objects</td></tr></table>", true);
		add(new Label("Objects"), "<table><tr valign=\"middle\"><td><img src=\"toolpad/alarms.png\"></td><td>Alarms</td></tr></table>", true);
		add(new Label("Objects"), "<table><tr valign=\"middle\"><td><img src=\"toolpad/maps.png\"></td><td>Maps</td></tr></table>", true);
		add(new Label("Objects"), "<table><tr valign=\"middle\"><td><img src=\"toolpad/config.png\"></td><td>Configuration</td></tr></table>", true);
	}

	/* (non-Javadoc)
	 * @see com.google.gwt.user.client.ui.StackPanel#onBrowserEvent(com.google.gwt.user.client.Event)
	 */
	@Override
	public void onBrowserEvent(Event event)
	{
		super.onBrowserEvent(event);
		workarea.clear();
		Widget widget = null;
		switch(getSelectedIndex())
		{
			case 0:		// Dashboard
				break;
			case 1:		// Objects
				break;
			case 2:		// Alarms
				widget = new AlarmBrowser();
				break;
			default:
				break;
		}
		if (widget != null)
			workarea.add(widget);
	}

	/**
	 * @param workarea the workarea to set
	 */
	public void setWorkarea(Panel workarea)
	{
		this.workarea = workarea;
	}
}

