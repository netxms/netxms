package org.netxms.ui.web.client;

import org.netxms.ui.web.client.mvc.AppController;
import com.extjs.gxt.ui.client.GXT;
import com.extjs.gxt.ui.client.mvc.Dispatcher;
import com.extjs.gxt.ui.client.util.Theme;
import com.google.gwt.core.client.EntryPoint;

public class Launcher implements EntryPoint
{
	/* (non-Javadoc)
	 * @see com.google.gwt.core.client.EntryPoint#onModuleLoad()
	 */
	@Override
	public void onModuleLoad()
	{
		GXT.setDefaultTheme(Theme.GRAY, true);

		Dispatcher dispatcher = Dispatcher.get();
		dispatcher.addController(new AppController());

		dispatcher.dispatch(AppEvents.Login);

		GXT.hideLoadingPanel("loading");
	}
}
