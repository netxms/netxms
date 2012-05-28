/**
 * 
 */
package org.netxms.ui.eclipse.perfview.dialogs;

import java.util.Date;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.DateTimeSelector;

/**
 * Dialog for selecting historical data
 */
public class HistoricalDataSelectionDialog extends Dialog
{
	private static final long serialVersionUID = 1L;

	private int maxRecords;
	private Date timeFrom;
	private Date timeTo;
	
	private Button radioLastRecords;
	private Button radioTimeFrame;
	private Spinner spinnerRecords;
	private DateTimeSelector dtsFrom;
	private DateTimeSelector dtsTo;
	
	/**
	 * @param parentShell
	 */
	public HistoricalDataSelectionDialog(Shell parentShell, int maxRecords, Date timeFrom, Date timeTo)
	{
		super(parentShell);
		this.maxRecords = maxRecords;
		this.timeFrom = timeFrom;
		this.timeTo = timeTo;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Select Data Range");
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
		layout.numColumns = 3;
		dialogArea.setLayout(layout);
		
		radioLastRecords = new Button(dialogArea, SWT.RADIO);
		radioLastRecords.setText("&Last n records");
		GridData gd = new GridData();
		gd.horizontalSpan = 3;
		radioLastRecords.setLayoutData(gd);
		radioLastRecords.setSelection(maxRecords != 0);
		
		spinnerRecords = new Spinner(dialogArea, SWT.BORDER);
		spinnerRecords.setMinimum(1);
		spinnerRecords.setMaximum(65535);
		spinnerRecords.setSelection(maxRecords);
		gd = new GridData();
		gd.horizontalSpan = 3;
		spinnerRecords.setLayoutData(gd);
		spinnerRecords.setEnabled(radioLastRecords.getSelection());

		radioTimeFrame = new Button(dialogArea, SWT.RADIO);
		radioTimeFrame.setText("&Time frame:");
		gd = new GridData();
		gd.horizontalSpan = 3;
		gd.verticalIndent = 5;
		radioTimeFrame.setLayoutData(gd);
		radioTimeFrame.setSelection(maxRecords == 0);
		
		dtsFrom = new DateTimeSelector(dialogArea, SWT.NONE);
		dtsFrom.setValue(timeFrom);
		dtsFrom.setEnabled(radioTimeFrame.getSelection());
		
		new Label(dialogArea, SWT.NONE).setText("  -  ");
		
		dtsTo = new DateTimeSelector(dialogArea, SWT.NONE);
		dtsTo.setValue(timeTo);
		dtsTo.setEnabled(radioTimeFrame.getSelection());
		
		final SelectionListener listener = new SelectionListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				spinnerRecords.setEnabled(radioLastRecords.getSelection());
				dtsFrom.setEnabled(radioTimeFrame.getSelection());
				dtsTo.setEnabled(radioTimeFrame.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		}; 
		radioLastRecords.addSelectionListener(listener);
		radioTimeFrame.addSelectionListener(listener);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		if (radioLastRecords.getSelection())
		{
			maxRecords = spinnerRecords.getSelection();
			timeFrom = null;
			timeTo = null;
		}
		else
		{
			maxRecords = 0;
			timeFrom = dtsFrom.getValue();
			timeTo = dtsTo.getValue();
		}
		super.okPressed();
	}

	/**
	 * @return the maxRecords
	 */
	public int getMaxRecords()
	{
		return maxRecords;
	}

	/**
	 * @return the timeFrom
	 */
	public Date getTimeFrom()
	{
		return timeFrom;
	}

	/**
	 * @return the timeTo
	 */
	public Date getTimeTo()
	{
		return timeTo;
	}
}
