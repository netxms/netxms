package org.netxms.ui.eclipse.imagelibrary.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.imagelibrary.Activator;
import org.netxms.ui.eclipse.imagelibrary.Messages;

public class ImageSelectionDialog extends Dialog
{

	protected ImageSelectionDialog(Shell parentShell)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
	}

	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.getString("ImageSelectionDialog.title"));
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("SelectImage.cx"), settings.getInt("SelectImage.cy"));
		}
		catch(NumberFormatException e)
		{
			newShell.setSize(400, 350);
		}
	}

	@Override
	protected Control createDialogArea(Composite parent)
	{
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		dialogArea.setLayout(new FormLayout());

		// ...

		return dialogArea;
	}

	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

	@Override
	protected void okPressed()
	{
		saveSettings();
		super.okPressed();
	}

	private void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put("SelectImage.cx", size.x);
		settings.put("SelectImage.cy", size.y);
	}

}
