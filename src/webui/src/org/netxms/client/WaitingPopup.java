/**
 * 
 */
package org.netxms.client;

import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Image;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.PopupPanel;

/**
 * @author Victor
 *
 */
public class WaitingPopup extends PopupPanel
{
	/**
	 * 
	 */
	public WaitingPopup(String text)
	{
		super(false, true);
		
		HorizontalPanel hp = new HorizontalPanel();
		hp.setVerticalAlignment(HorizontalPanel.ALIGN_MIDDLE);
		hp.setSpacing(4);
		hp.add(new Image("images/loading.gif"));
		hp.add(new Label(text));
		setWidget(hp);
	}
}
