/**
 * 
 */
package org.netxms.client;

import com.google.gwt.user.client.ui.FlexTable;
import com.google.gwt.user.client.ui.SimplePanel;

/**
 * @author Victor
 *
 */
public class InfoBar extends SimplePanel
{
	/**
	 * 
	 */
	public InfoBar()
	{
		setWidth("100%");
		//setHeight("40px");
		setStylePrimaryName("infoBar");
		
		FlexTable layout = new FlexTable();
		layout.insertRow(0);
		layout.addCell(0);
		layout.setWidget(0, 0, new ToolButton(webui.mainImageBundle.iconLogout().createImage(), "Logout"));
		layout.addCell(0);
		layout.setText(0, 1, "hello");
		
		add(layout);
	}
}
