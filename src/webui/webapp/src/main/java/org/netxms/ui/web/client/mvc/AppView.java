package org.netxms.ui.web.client.mvc;

import org.netxms.ui.web.client.AppEvents;
import com.extjs.gxt.ui.client.mvc.AppEvent;
import com.extjs.gxt.ui.client.mvc.Controller;
import com.extjs.gxt.ui.client.mvc.View;
import com.extjs.gxt.ui.client.widget.Viewport;
import com.extjs.gxt.ui.client.widget.layout.BorderLayout;
import com.google.gwt.user.client.ui.RootPanel;

public class AppView extends View
{

	private Viewport viewport;

	public AppView(final Controller controller)
	{
		super(controller);
	}

	@Override
	protected void handleEvent(final AppEvent event)
	{
		if (event.getType() == AppEvents.Init)
		{
			initUI();
		}
	}

	private void initUI()
	{
		viewport = new Viewport();
		viewport.setLayout(new BorderLayout());

		RootPanel.get().add(viewport);
	}

}
