/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.reporter.widgets;

import java.util.Calendar;
import java.util.Date;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.ui.eclipse.reporter.Activator;
import org.netxms.ui.eclipse.tools.ImageCache;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Field editor for START_DATE and END_DATE field types
 */
public class DateFieldEditor extends FieldEditor
{
	private final static int FIELD_YEAR = 0;
	private final static int FIELD_MONTH = 1;
	private final static int FIELD_DAY = 2;
	
	private Combo[] dateElements;
	
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
	protected Control createContent(Composite parent)
	{
		final ImageCache imageCache = new ImageCache(this);
		
		final Composite content = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.numColumns = 4;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.makeColumnsEqualWidth = true;
		content.setLayout(layout);
		
		Calendar dateTime = Calendar.getInstance();
		try 
		{
			dateTime.setTime(new Date(Long.parseLong(parameter.getDefaultValue()) * 1000));
		} 
		catch (NumberFormatException e) 
		{
			dateTime.setTime(new Date());
		}
		
      final String[] dateElementNames = { "Year", "Month", "Day" };
		dateElements = new Combo[dateElementNames.length];
		for(int idx = 0; idx <  dateElementNames.length; idx++)
		{
			Combo cb = WidgetHelper.createLabeledCombo(content, SWT.BORDER, dateElementNames[idx], WidgetHelper.DEFAULT_LAYOUT_DATA);
			cb.setText(getDateTimeText(idx, dateTime));
			cb.add("current"); //$NON-NLS-1$
			cb.add("previous"); //$NON-NLS-1$
			cb.add("next"); //$NON-NLS-1$
			dateElements[idx] = cb;
		}
		
		final ImageHyperlink link = toolkit.createImageHyperlink(content, SWT.NONE);
		link.setImage(imageCache.add(Activator.getImageDescriptor("icons/calendar.png"))); //$NON-NLS-1$
		link.setToolTipText("Calendar");
		link.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            createPopupCalendar(link);
         }
		});
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.LEFT;
      link.setLayoutData(gd);
      
      return content;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#getValue()
	 */
	@Override
	public String getValue()
	{
		String value = null;
		for (int idx = 0; idx < 3; idx++)
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
			default:
				value = 0;
				break;
		}
		return String.valueOf(value);
	}
	
	/**
	 * Create popup calendar widget
	 */
	private void createPopupCalendar(Control anchor)
	{
	   final Shell popup = new Shell(getShell(), SWT.NO_TRIM | SWT.ON_TOP);
	   final DateTime calendar = new DateTime(popup, SWT.CALENDAR | SWT.SHORT);
	   
	   Point size = calendar.computeSize(SWT.DEFAULT, SWT.DEFAULT);
	   popup.setSize(size);
	   calendar.setSize(size);
	   
	   calendar.addFocusListener(new FocusListener() {
         @Override
         public void focusLost(FocusEvent e)
         {
            popup.close();
         }
         
         @Override
         public void focusGained(FocusEvent e)
         {
         }
      });
	   
	   calendar.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            Calendar date = Calendar.getInstance();
            date.set(calendar.getYear(), calendar.getMonth(), calendar.getDay(), calendar.getHours(), calendar.getMinutes(), calendar.getSeconds());
            for (int idx = 0; idx <  dateElements.length; idx++)
               dateElements[idx].setText(getDateTimeText(idx, date));
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            popup.close();
         }
      });
	   
	   Rectangle rect = getDisplay().map(anchor.getParent(), null, anchor.getBounds());
	   popup.setLocation(rect.x, rect.y + rect.height);
	   popup.open();
	}
}
