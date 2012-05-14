/**
 * 
 */
package org.netxms.ui.eclipse.perfview.dialogs;

import java.util.Date;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dialog for selecting historical data
 */
public class HistoricalDataSelectionDialog extends Dialog
{
	private int maxRecords;
	private Date timeFrom;
	private Date timeTo;
	
	private Button radioLastRecords;
	private Button radioTimeFrame;
	private Spinner spinnerRecords;
	
	/**
	 * @param parentShell
	 */
	public HistoricalDataSelectionDialog(Shell parentShell)
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
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		dialogArea.setLayout(layout);
		
		radioLastRecords = new Button(dialogArea, SWT.RADIO);
		radioLastRecords.setText("&Last records:");
		
		spinnerRecords = new Spinner(dialogArea, SWT.BORDER);
		spinnerRecords.setMinimum(1);
		spinnerRecords.setMaximum(65535);
		spinnerRecords.setSelection(maxRecords);
		
		radioTimeFrame = new Button(dialogArea, SWT.RADIO);
		radioTimeFrame.setText("&Time frame:");
		
		
		
		return dialogArea;
	}

	
}
