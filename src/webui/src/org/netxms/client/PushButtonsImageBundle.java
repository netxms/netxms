/**
 * 
 */
package org.netxms.client;

import com.google.gwt.user.client.ui.AbstractImagePrototype;
import com.google.gwt.user.client.ui.ImageBundle;

/**
 * @author Victor
 *
 */
public interface PushButtonsImageBundle extends ImageBundle
{
	/**
	 * "Login" button normal
	 * @gwt.resource buttons/en/normal/login.png
	 */
	public AbstractImagePrototype normal_login();
	
	/**
	 * "Login" button pressed
	 * @gwt.resource buttons/en/pressed/login.png
	 */
	public AbstractImagePrototype pressed_login();
}
