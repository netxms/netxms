package org.netxms.ui.eclipse.reporter.dialogs;

import java.util.Calendar;
import java.util.Date;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;

public class CalendarDialog extends Dialog 
{

	public final static String DATETIME_FORMAT ="dd.MM.yyyy HH:mm:ss"; //$NON-NLS-1$
	
	private DateTime calendar;
	private DateTime time;
	
	private String strValue;
	private Date dateValue;
	
	public CalendarDialog(Shell parentShell) 
	{
		super(parentShell);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Calendar");
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) 
	{
		final Composite dialogArea = (Composite)super.createDialogArea(parent);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);		
		calendar = new DateTime (dialogArea, SWT.CALENDAR);
		GridData calendarData = new GridData();
		calendarData.horizontalSpan = 2;
		calendar.setLayoutData(calendarData);
		Label label = new Label(dialogArea, SWT.NONE);
		label.setText("Time");
		label.setLayoutData(new GridData());
		time = new DateTime (dialogArea, SWT.TIME | SWT.LONG | SWT.BORDER);
		GridData timeData = new GridData(80, 18);
		timeData.verticalIndent = 5;
		time.setLayoutData(timeData);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed() 
	{
		strValue = calendar.getDay() + "." + (calendar.getMonth() + 1) + "." + calendar.getYear() //$NON-NLS-1$ //$NON-NLS-2$
		+ "\\" + time.getHours() + ":" + time.getMinutes() + ":" + time.getSeconds(); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
		
		Calendar dateTime = Calendar.getInstance();
		dateTime.set(calendar.getYear(), calendar.getMonth(), calendar.getDay(), time.getHours(), time.getMinutes(), time.getSeconds());
		dateValue = dateTime.getTime();
				
		super.okPressed();
	}
	
	public String getStringValue() 
	{
		return strValue;
	}
	
	public Date getDateValue() 
	{
		return dateValue;
	}

	protected void createButtonsForButtonBar(Composite parent) 
	{
		// create OK and Cancel buttons by default
		createButton(parent, IDialogConstants.OK_ID, IDialogConstants.OK_LABEL, true);
		createButton(parent, IDialogConstants.CANCEL_ID, "Cancel", false);
	}

	
}
