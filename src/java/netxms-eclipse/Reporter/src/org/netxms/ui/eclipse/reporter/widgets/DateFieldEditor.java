package org.netxms.ui.eclipse.reporter.widgets;

import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.ui.eclipse.reporter.Activator;
import org.netxms.ui.eclipse.reporter.dialogs.CalendarDialog;
import org.netxms.ui.eclipse.tools.ImageCache;
import org.netxms.ui.eclipse.tools.WidgetHelper;

public class DateFieldEditor extends FieldEditor
{
	public final static int FIELD_YEAR = 0;
	public final static int FIELD_MONTH = 1;
	public final static int FIELD_DAY = 2;
	public final static int FIELD_HOUR = 3;
	public final static int FIELD_MINUTE = 4;
	
	private Composite parentArea;
	private Combo[] dateElements;
	private String[] dateElementNames;
	private ImageCache imageCache;
	
	private long offset = 0;
	
	/** 
	 * Constructor
	 * 
	 * @param parameter
	 * @param toolkit
	 * @param parent
	 */
	public DateFieldEditor(ReportParameter parameter, FormToolkit toolkit, Composite parent)
	{
		super(parameter, toolkit, parent);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createContent(Composite parent)
	{
		imageCache = new ImageCache(this);
		dateElementNames = new String[] {"Year", "Month", "Day", "Hours", "Minutes"};
		toolkit = new FormToolkit(getDisplay());
		parentArea = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.numColumns = 6;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.makeColumnsEqualWidth = true;
		parentArea.setLayout(layout);
		
		Calendar dateTime = Calendar.getInstance();
		try {
			dateTime.setTime(new Date(Long.parseLong(parameter.getDefaultValue()) * 1000));
		} catch (NumberFormatException e) {
			dateTime.setTime(new Date());
		}
		
		dateElements = new Combo[dateElementNames.length];
		for (int idx = 0; idx <  dateElementNames.length; idx++)
		{
			Combo cb = WidgetHelper.createLabeledCombo(parentArea, SWT.BORDER, dateElementNames[idx], WidgetHelper.DEFAULT_LAYOUT_DATA);
			cb.setText(getDateTimeText(idx, dateTime));
			if (idx != FIELD_MINUTE)
			{
				cb.add("current"); //$NON-NLS-1$
				cb.add("previous"); //$NON-NLS-1$
				cb.add("next"); //$NON-NLS-1$
			}
			dateElements[idx] = cb;
		}
		
		Button openCalendar = new Button(parentArea, SWT.PUSH);
		GridData gd = new GridData(22, 22);
		gd.verticalAlignment = SWT.DOWN;
		openCalendar.setLayoutData(gd);
		openCalendar.setImage(imageCache.add(Activator.getImageDescriptor("icons/calendar.png"))); //$NON-NLS-1$
		openCalendar.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected (SelectionEvent e) 
			{
				CalendarDialog dialog = new CalendarDialog(getShell());
				if(dialog.open() == Window.OK) {
					Calendar date = Calendar.getInstance();
					date.setTime(dialog.getDateValue());
					for (int idx = 0; idx <  dateElementNames.length; idx++)
						dateElements[idx].setText(getDateTimeText(idx, date));
				}
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#getValue()
	 */
	@Override
	public String getValue()
	{
		String value = null;
		for (int idx = 0; idx < 5; idx++)
		{
			if (idx == 0)
				value = dateElements[idx].getText();
			else
				value += ";" + dateElements[idx].getText();
		}
		return value;
	}

	/**
	 * Get dateTime text for Combo
	 * @param comboName
	 * @param dateTime
	 */
	private String getDateTimeText(int fieldIdx, Calendar dateTime)
	{
		int value;
		switch (fieldIdx)
		{
			case FIELD_YEAR:
				value = dateTime.get(Calendar.YEAR);
				break;
			case FIELD_MONTH:
				value = dateTime.get(Calendar.MONTH) + 1;
				break;
			case FIELD_DAY:
				value = dateTime.get(Calendar.DAY_OF_MONTH);
				break;
			case FIELD_HOUR:
				value = dateTime.get(Calendar.HOUR_OF_DAY);
				break;
			case FIELD_MINUTE:
				value = dateTime.get(Calendar.MINUTE);
				break;
			default:
				value = 0;
				break;
		}
		return String.valueOf(value);
	}
	
	/**
	 * @return the offset
	 */
	public long getOffset()
	{
		return offset;
	}
}
