/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

/**
 * @author victor
 *
 */
public class AddDashboardElementDlg extends Dialog
{
	private Combo elementTypeSelector; 
	private int elementType;
	
	public AddDashboardElementDlg(Shell parentShell)
	{
		super(parentShell);
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
		
		elementTypeSelector = new Combo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY);
		elementTypeSelector.add("Label");
		elementTypeSelector.add("Line Chart");
		elementTypeSelector.add("Bar Chart");
		elementTypeSelector.add("Pie Chart");
		elementTypeSelector.add("Tube Chart");
		elementTypeSelector.add("Status Chart");
		elementTypeSelector.add("Status Indicator");
		elementTypeSelector.add("Dashboard");
		elementTypeSelector.add("Network Map");
		elementTypeSelector.add("Custom Widget");
		elementTypeSelector.add("Geo Map");
		elementTypeSelector.add("Alarm Viewer");
		elementTypeSelector.add("Availability Chart");
		elementTypeSelector.select(1);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		elementType = elementTypeSelector.getSelectionIndex();
		super.okPressed();
	}

	/**
	 * @return the elementType
	 */
	public int getElementType()
	{
		return elementType;
	}
}
