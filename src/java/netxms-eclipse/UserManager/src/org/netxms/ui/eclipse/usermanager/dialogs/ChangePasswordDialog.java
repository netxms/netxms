package org.netxms.ui.eclipse.usermanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.shared.IUIConstants;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Activator;

public class ChangePasswordDialog extends Dialog
{
	private Text textPassword1;
	private Text textPassword2;
	private String password;

	public ChangePasswordDialog(Shell parentShell)
	{
		super(parentShell);
	}

	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite) super.createDialogArea(parent);

		final GridLayout layout = new GridLayout();

		layout.marginWidth = IUIConstants.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = IUIConstants.DIALOG_HEIGHT_MARGIN;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		Label pic = new Label(dialogArea, SWT.NONE);
		pic.setImage(Activator.getImageDescriptor("icons/password.png").createImage());
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		pic.setLayoutData(gd);

		Composite editArea = new Composite(dialogArea, SWT.NONE);
		FillLayout editAreaLayout = new FillLayout(SWT.VERTICAL);
		editAreaLayout.marginWidth = 0;
		editAreaLayout.marginHeight = 0;
		editArea.setLayout(editAreaLayout);
		
		textPassword1 = WidgetHelper.createLabeledText(editArea, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD, "New password:", "", null);
		textPassword2 = WidgetHelper.createLabeledText(editArea, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD, "Confirm new password:", "", null);

		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.widthHint = 250;
		editArea.setLayoutData(gd);
		
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
		if (password1.equals(password2))
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
