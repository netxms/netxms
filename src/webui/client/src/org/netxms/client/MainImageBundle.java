package org.netxms.client;

import com.google.gwt.user.client.ui.AbstractImagePrototype;
import com.google.gwt.user.client.ui.ImageBundle;

public interface MainImageBundle extends ImageBundle
{
	/**
	 * Settings icon
	 */
	@Resource("org/netxms/public/icons/settings.png")
	public AbstractImagePrototype iconSettings();

	/**
	 * Logout icon
	 */
	@Resource("org/netxms/public/icons/logout.png")
	public AbstractImagePrototype iconLogout();
}
