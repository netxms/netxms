/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

/**
 * @author victor
 *
 */
public class EditDashboardElement extends Dialog
{
	private Text text;
	private String config;
	
	public EditDashboardElement(Shell parentShell, String config)
	{
		super(parentShell);
		this.config = config;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		text = new Text(dialogArea, SWT.BORDER | SWT.MULTI);
		text.setText(config);
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.FILL;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.widthHint = 400;
		gd.heightHint = 300;
		text.setLayoutData(gd);

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		config = text.getText();
		super.okPressed();
	}

	/**
	 * @return the text
	 */
	public Text getText()
	{
		return text;
	}

	/**
	 * @return the config
	 */
	public String getConfig()
	{
		return config;
	}

}
