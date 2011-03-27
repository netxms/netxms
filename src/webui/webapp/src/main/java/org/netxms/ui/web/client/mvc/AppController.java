package org.netxms.ui.web.client.mvc;

import org.netxms.ui.web.client.AppEvents;
import com.extjs.gxt.ui.client.event.EventType;
import com.extjs.gxt.ui.client.mvc.AppEvent;
import com.extjs.gxt.ui.client.mvc.Controller;

public class AppController extends Controller
{

	private AppView appView;

	public AppController()
	{
		registerEventTypes(AppEvents.Init);
		registerEventTypes(AppEvents.Login);
		registerEventTypes(AppEvents.Error);
	}

	@Override
	public void handleEvent(final AppEvent event)
	{
		final EventType type = event.getType();
		if (type == AppEvents.Init)
		{
			onInit(event);
		}
		else if (type == AppEvents.Login)
		{
			onLogin(event);
		}
		else if (type == AppEvents.Error)
		{
			onError(event);
		}
	}

	@Override
	public void initialize()
	{
		appView = new AppView(this);
	}

	private void onError(final AppEvent event)
	{
		System.out.println("error: " + event.<Object> getData());
	}

	private void onInit(final AppEvent event)
	{
		forwardToView(appView, event);
	}

	private void onLogin(final AppEvent event)
	{
		forwardToView(appView, event);
	}

}
