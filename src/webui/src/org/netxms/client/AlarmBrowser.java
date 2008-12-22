/**
 * 
 */
package org.netxms.client;

import com.google.gwt.user.client.ui.Grid;
import com.google.gwt.user.client.ui.VerticalPanel;

/**
 * @author Victor
 *
 */
public class AlarmBrowser extends VerticalPanel
{
	private Grid listView = new Grid(1, 7);
	
	/**
	 * 
	 */
	public AlarmBrowser()
	{
		super();
		
		listView.setText(0, 0, "Severity");
		listView.setText(0, 1, "State");
		listView.setText(0, 2, "Source");
		listView.setText(0, 3, "Message");
		listView.setText(0, 4, "Count");
		listView.setText(0, 5, "Created");
		listView.setText(0, 6, "Last change");
		
		listView.setStylePrimaryName("webuiTable");
//		listView.
		
		add(listView);
	}

}
