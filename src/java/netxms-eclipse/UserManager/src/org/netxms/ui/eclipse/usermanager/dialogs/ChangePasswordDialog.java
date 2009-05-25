package org.netxms.ui.eclipse.usermanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.shared.IUIConstants;

public class ChangePasswordDialog extends Dialog
{

	private Shell shell;
	private Text textPassword1;
	private Text textPassword2;
	private String password;

	public ChangePasswordDialog(Shell parentShell)
	{
		super(parentShell);
		this.shell = parentShell;
	}

	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite) super.createDialogArea(parent);

		final FillLayout layout = new FillLayout(SWT.VERTICAL);

		layout.marginWidth = IUIConstants.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = IUIConstants.DIALOG_HEIGHT_MARGIN;
		dialogArea.setLayout(layout);

		new Label(dialogArea, SWT.NONE).setText("New password:");
		textPassword1 = new Text(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD);
		new Label(dialogArea, SWT.NONE).setText("Confirm new password:");
		textPassword2 = new Text(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD);

		final ModifyListener listener = new ModifyListener()
		{

			@Override
			public void modifyText(ModifyEvent e)
			{
				Control button = getButton(IDialogConstants.OK_ID);
				button.setEnabled(validate());
			}

		};
		textPassword1.addModifyListener(listener);
		textPassword2.addModifyListener(listener);

		return dialogArea;
	}

	protected boolean validate()
	{
		final String password1 = textPassword1.getText();
		final String password2 = textPassword2.getText();

		final boolean ret;
		if (password1.length() > 0 && password1.equals(password2))
		{
			ret = true;
		}
		else
		{
			ret = false;
		}

		return ret;
	}

	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Change password");
	}

	@Override
	protected Button createButton(Composite parent, int id, String label, boolean defaultButton)
	{
		final Button button = super.createButton(parent, id, label, defaultButton);

		if (id == OK)
		{
			button.setEnabled(false);
		}

		return button;
	}

	@Override
	protected void okPressed()
	{
		password = textPassword1.getText();
		super.okPressed();
	}

	public String getPassword()
	{
		return password;
	}

}
