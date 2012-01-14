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

import org.eclipse.jface.preference.BooleanFieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.jface.preference.RadioGroupFieldEditor;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.netxms.ui.eclipse.charts.Activator;

/**
 * General charts preference page
 *
 */
public class GeneralChartPrefs extends FieldEditorPreferencePage implements IWorkbenchPreferencePage
{
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
		addField(new BooleanFieldEditor("Chart.ShowTitle", "Show chart title", getFieldEditorParent()));
		addField(new BooleanFieldEditor("Chart.ShowToolTips", "Show tooltips when mouse hovers over plot area", getFieldEditorParent()));

		/*
		final String[][] gridStyles =	new String[][] { 
				{ "None", LineStyle.NONE.label },
				{ "Solid", LineStyle.SOLID.label },
				{ "Dash", LineStyle.DASH.label },
				{ "Dot", LineStyle.DOT.label },
				{ "Dash-Dot", LineStyle.DASHDOT.label },
				{ "Dash-Dot-Dot", LineStyle.DASHDOTDOT.label },
			};
		addField(new RadioGroupFieldEditor("Chart.Grid.X.Style", "Style for X axis grid", 3, gridStyles, getFieldEditorParent(), true));
		addField(new RadioGroupFieldEditor("Chart.Grid.Y.Style", "Style for Y axis grid", 3, gridStyles, getFieldEditorParent(), true));
		*/
	}
}
