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
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dialog for edit element's XML
 */
public class EditElementXmlDlg extends Dialog
{
	private String value;
	private Text text;
	
	/**
	 * @param parentShell
	 * @param xml
	 */
	public EditElementXmlDlg(Shell parentShell, String xml)
	{
		super(parentShell);
		value = xml;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Edit Element XML");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		
		final Label label = new Label(dialogArea, SWT.NONE);
		label.setText("XML Configuration");
	
		text = new Text(dialogArea, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.minimumHeight = 300;
		gd.minimumWidth = 400;
		text.setLayoutData(gd);
		text.setText(value);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		value = text.getText();
		super.okPressed();
	}

	/**
	 * @return the value
	 */
	public String getValue()
	{
		return value;
	}
}
