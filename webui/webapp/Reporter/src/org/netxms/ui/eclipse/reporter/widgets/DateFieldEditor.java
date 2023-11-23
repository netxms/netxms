/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.ui.eclipse.reporter.Activator;
import org.netxms.ui.eclipse.reporter.Messages;
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
   public DateFieldEditor(ReportParameter parameter, Composite parent)
	{
      super(parameter, parent);
	}

   /**
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

      final String[] dateElementNames = { Messages.get().DateFieldEditor_Year, Messages.get().DateFieldEditor_Month, Messages.get().DateFieldEditor_Day };
		dateElements = new Combo[dateElementNames.length];
		for(int idx = 0; idx <  dateElementNames.length; idx++)
		{
			Combo cb = WidgetHelper.createLabeledCombo(content, SWT.BORDER, dateElementNames[idx], WidgetHelper.DEFAULT_LAYOUT_DATA);
			cb.setText(getDateTimeText(idx, dateTime));
			cb.add("current"); //$NON-NLS-1$
         cb.add("first"); //$NON-NLS-1$
         cb.add("last"); //$NON-NLS-1$
         cb.add("next"); //$NON-NLS-1$
			cb.add("previous"); //$NON-NLS-1$
			dateElements[idx] = cb;
		}

      final ImageHyperlink link = new ImageHyperlink(content, SWT.NONE);
      link.setImage(imageCache.add(Activator.getImageDescriptor("icons/calendar-large.png"))); //$NON-NLS-1$
		link.setToolTipText(Messages.get().DateFieldEditor_Calendar);
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
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalIndent = 5;
      link.setLayoutData(gd);

      return content;
	}

   /**
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
				value += ";" + dateElements[idx].getText(); //$NON-NLS-1$
		}
		return value;
	}

	/**
    * Get dateTime text for Combo
    *
    * @param fieldIndex field index
    * @param calendar calendar
    */
	private String getDateTimeText(int fieldIndex, Calendar calendar)
	{
		int value;
		switch (fieldIndex)
		{
			case FIELD_YEAR:
				value = calendar.get(Calendar.YEAR);
				break;
			case FIELD_MONTH:
				value = calendar.get(Calendar.MONTH) + 1;
				break;
			case FIELD_DAY:
				value = calendar.get(Calendar.DAY_OF_MONTH);
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
      WidgetHelper.createPopupCalendar(getShell(), anchor, null, (date) -> {
         for(int idx = 0; idx < dateElements.length; idx++)
            dateElements[idx].setText(getDateTimeText(idx, date));
      });
   }
}
