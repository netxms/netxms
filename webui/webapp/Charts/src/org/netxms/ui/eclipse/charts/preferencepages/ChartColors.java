/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.preferencepages;

import org.eclipse.jface.preference.ColorFieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.netxms.ui.eclipse.charts.Activator;

/**
 * Chart colors preference page
 *
 */
public class ChartColors extends FieldEditorPreferencePage implements IWorkbenchPreferencePage
{
	private static final long serialVersionUID = 1L;

	private Group lineColors;
	private Label filler;
	
	public ChartColors()
	{
		super(FieldEditorPreferencePage.GRID);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchPreferencePage#init(org.eclipse.ui.IWorkbench)
	 */
	@Override
	public void init(IWorkbench workbench)
	{
		setPreferenceStore(Activator.getDefault().getPreferenceStore());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.FieldEditorPreferencePage#createFieldEditors()
	 */
	@Override
	protected void createFieldEditors()
	{
		addField(new ColorFieldEditor("Chart.Colors.Background", "Background color:", getFieldEditorParent()));
		addField(new ColorFieldEditor("Chart.Colors.PlotArea", "Plot area background color:", getFieldEditorParent()));
		addField(new ColorFieldEditor("Chart.Colors.Title", "Title text color:", getFieldEditorParent()));
		
		filler = new Label(getFieldEditorParent(), SWT.NONE);

		addField(new ColorFieldEditor("Chart.Axis.X.Color", "X axis tick color:", getFieldEditorParent()));
		addField(new ColorFieldEditor("Chart.Axis.Y.Color", "Y axis tick color:", getFieldEditorParent()));
		addField(new ColorFieldEditor("Chart.Grid.X.Color", "X axis grid color:", getFieldEditorParent()));
		addField(new ColorFieldEditor("Chart.Grid.Y.Color", "Y axis grid color:", getFieldEditorParent()));

		lineColors = new Group(getFieldEditorParent(), SWT.NONE);
		lineColors.setText("Line colors");
			
		addField(new ColorFieldEditor("Chart.Colors.Data.0", "1st:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.1", "2nd:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.2", "3rd:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.3", "4th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.4", "5th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.5", "6th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.6", "7th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.7", "8th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.8", "9th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.9", "10th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.10", "11th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.11", "12th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.12", "13th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.13", "14th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.14", "15th:", lineColors));
		addField(new ColorFieldEditor("Chart.Colors.Data.15", "16th:", lineColors));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.FieldEditorPreferencePage#adjustGridLayout()
	 */
	@Override
	protected void adjustGridLayout()
	{
		((GridLayout)getFieldEditorParent().getLayout()).numColumns = 4;

		GridLayout layout = new GridLayout();
		layout.numColumns = 8;
		layout.makeColumnsEqualWidth = false;
		lineColors.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 4;
		lineColors.setLayoutData(gd);
		
		gd = new GridData();
		gd.horizontalSpan = 2;
		filler.setLayoutData(gd);
	}
}
