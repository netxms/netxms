package org.netxms.client;

import com.google.gwt.core.client.EntryPoint;
import com.google.gwt.core.client.GWT;
import com.google.gwt.http.client.Request;
import com.google.gwt.http.client.RequestBuilder;
import com.google.gwt.http.client.RequestCallback;
import com.google.gwt.http.client.RequestException;
import com.google.gwt.http.client.Response;
import com.google.gwt.json.client.JSONNumber;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONParser;
import com.google.gwt.json.client.JSONString;
import com.google.gwt.json.client.JSONValue;
import com.google.gwt.user.client.Command;
import com.google.gwt.user.client.DeferredCommand;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.ui.DockPanel;
import com.google.gwt.user.client.ui.HorizontalSplitPanel;
import com.google.gwt.user.client.ui.RootPanel;
import com.google.gwt.user.client.ui.SimplePanel;

/**
 * Entry point classes define <code>onModuleLoad()</code>.
 */
public class webui implements EntryPoint 
{
	private DockPanel mainPanel = new DockPanel();
	private SimplePanel infoBar;
	private ToolPanel toolPanel;
	private SimplePanel workarea;
	
	public static MainImageBundle mainImageBundle;

	/**
	 * Redirect browser to specific URL.
	 * 
	 * @param url URL to redirect to.
	 */
	public static native void redirect(String url)
	/*-{
   	$wnd.location = url;
	}-*/;
	
	/**
	 * This is the entry point method.
	 */
	public void onModuleLoad() 
	{
		mainImageBundle = GWT.create(MainImageBundle.class);
		
		mainPanel.setSize("100%", "100%");
		RootPanel.get().add(mainPanel);

		// Show login dialog
		final LoginDialog dlg = new LoginDialog();
		dlg.execute(new Command()
			{
				public void execute()
				{
					doLogin(dlg);
				}
	      }, new Command()
	      {
	      	public void execute()
	      	{
	      		redirect("cancel.html");
	      	}
	      });
	}
	
	
	/**
	 * Login to server
	 */
	private void doLogin(LoginDialog dlg)
	{
		// Show waiting popup
		final WaitingPopup wp = new WaitingPopup("Connecting to NetXMS server...");
		wp.center();
		
		RequestBuilder builder = new RequestBuilder(RequestBuilder.GET, "/jsonlogin.app?user=" + dlg.getLogin() + "&passwd=" + dlg.getPassword());
		builder.setRequestData("");
		builder.setCallback(new RequestCallback()
			{
				public void onError(Request request, Throwable exception)
				{
					wp.hide();
					Window.alert("Request failed");
				}

				public void onResponseReceived(Request request, Response response)
				{
					String errorText = "Internal error";
					boolean success = false;
					
					wp.hide();
					
					if (response.getStatusCode() == Response.SC_OK)
					{
						JSONValue rawData = JSONParser.parse(response.getText());
						JSONObject data = rawData.isObject();
						if ((data != null) && data.containsKey("rcc"))
						{
							JSONNumber rcc = data.get("rcc").isNumber();
							if ((rcc != null) && (rcc.doubleValue() == 0))
							{
								DeferredCommand.addCommand(new Command()
									{
										public void execute()
										{
											loadMainPage();
										}
									});
								success = true;
							}
							else
							{
								JSONString serverError = data.get("errorText").isString();
								errorText = (serverError != null) ? serverError.stringValue() : "Internal error";
							}
						}
						else
						{
							errorText = "Invalid server response";
						}
					}
					else
					{
						errorText = "HTTP error: " + response.getStatusText();
					}
					
					// TODO: show error page
					if (!success)
					{
						Window.alert(errorText);
						redirect("webui.html");
					}
				}
			});
		try
		{
			builder.send();
		}
		catch(RequestException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	/**
	 * Load main application page
	 */
	private void loadMainPage()
	{
		infoBar = new InfoBar();
		mainPanel.add(infoBar, DockPanel.NORTH);
		mainPanel.setCellHeight(infoBar, "40px");
		
		workarea = new SimplePanel();
		workarea.setStylePrimaryName("workarea");
		
		toolPanel = new ToolPanel();
		
		HorizontalSplitPanel workPanel = new HorizontalSplitPanel();
		workPanel.setSplitPosition("200px");
		workPanel.setSize("100%", "100%");
		workPanel.setLeftWidget(toolPanel);
		workPanel.setRightWidget(workarea);
		toolPanel.setWorkarea(workarea);
		mainPanel.add(workPanel, DockPanel.CENTER);
	}
}
