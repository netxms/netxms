package org.netxms.client;

import com.google.gwt.user.client.Command;
import com.google.gwt.user.client.DeferredCommand;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.ui.HasHorizontalAlignment;
import com.google.gwt.user.client.ui.PushButton;
import com.google.gwt.user.client.ui.ClickListener;
import com.google.gwt.user.client.ui.DialogBox;
import com.google.gwt.user.client.ui.FlexTable;
import com.google.gwt.user.client.ui.Image;
import com.google.gwt.user.client.ui.PasswordTextBox;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.Widget;

public class LoginDialog extends DialogBox
{
	private TextBox textLogin = new TextBox();
	private PasswordTextBox textPassword = new PasswordTextBox();
	private PushButton okButton = new PushButton("Login");
	private PushButton cancelButton = new PushButton("Cancel");
	private String dataLogin;
	private String dataPassword;
	private Command okCommand = null;
	private Command cancelCommand = null;
	
	/**
	 * 
	 */
	public LoginDialog()
	{
		super(false, true);

		VerticalPanel vp = new VerticalPanel();
		vp.add(new Image("images/login.png"));
		
		FlexTable layout = new FlexTable();
		layout.setWidth("100%");

		layout.insertRow(0);
		layout.addCell(0);
		layout.setText(0, 0, "Login:");
		layout.addCell(0);
		textLogin.setWidth("100%");
		layout.setWidget(0, 1, textLogin);
		layout.getFlexCellFormatter().setWidth(0, 1, "100%");
		
		layout.insertRow(1);
		layout.addCell(1);
		layout.setText(1, 0, "Password:");
		layout.addCell(1);
		textPassword.setWidth("100%");
		layout.setWidget(1, 1, textPassword);
		layout.getFlexCellFormatter().setWidth(1, 1, "100%");

		FlexTable buttons = new FlexTable();
		okButton.setWidth("90px");
		cancelButton.setWidth("90px");
		layout.addCell(0);
		buttons.setWidget(0, 0, okButton);
		layout.addCell(0);
		buttons.setWidget(0, 1, cancelButton);

		layout.insertRow(2);
		layout.addCell(2);
		layout.setWidget(2, 0, buttons);
		layout.getFlexCellFormatter().setColSpan(2, 0, 2);
		layout.getCellFormatter().setHorizontalAlignment(2, 0, HasHorizontalAlignment.ALIGN_RIGHT);

		vp.add(layout);
		
		setWidget(vp);
		setText("Login");
	}
	
	public void execute(final Command cmdOK, final Command cmdCancel)
	{
		okCommand = cmdOK;
		cancelCommand = cmdCancel;
		
		okButton.addClickListener(new ClickListener()
			{
				public void onClick(Widget sender)
				{
					LoginDialog.this.onOK();
				}
			});
		cancelButton.addClickListener(new ClickListener()
		{
			public void onClick(Widget sender)
			{
				LoginDialog.this.onCancel();
			}
		});
		center();
		textLogin.setFocus(true);
	}

	/**
	 * @return Login name
	 */
	public String getLogin()
	{
		return dataLogin;
	}

	/**
	 * @return Password
	 */
	public String getPassword()
	{
		return dataPassword;
	}
	
	/**
	 * OK button handler
	 */
	protected void onOK()
	{
		dataLogin = textLogin.getText();
		dataPassword = textPassword.getText();
		hide();
		if (okCommand != null)
			DeferredCommand.addCommand(okCommand);
	}

	/**
	 * Cancel button handler
	 */
	protected void onCancel()
	{
		hide();
		if (cancelCommand != null)
			DeferredCommand.addCommand(cancelCommand);
	}

	/* (non-Javadoc)
	 * @see com.google.gwt.user.client.ui.PopupPanel#onKeyPressPreview(char, int)
	 */
	@Override
	public boolean onKeyPressPreview(char key, int modifiers)
	{
		if (key == 13)
		{
			onOK();
		}
		else if (key == 27)
		{
			onCancel();
		}
		return super.onKeyPressPreview(key, modifiers);
	}
}
