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
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.Messages;

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
		addField(new BooleanFieldEditor("Chart.ShowTitle", Messages.GeneralChartPrefs_ShowTitle, getFieldEditorParent())); //$NON-NLS-1$
		addField(new BooleanFieldEditor("Chart.ShowToolTips", Messages.GeneralChartPrefs_ShowTooltips, getFieldEditorParent())); //$NON-NLS-1$

		/*
		final String[][] gridStyles =	new String[][] { 
				{ Messages.GeneralChartPrefs_None, LineStyle.NONE.label },
				{ Messages.GeneralChartPrefs_Solid, LineStyle.SOLID.label },
				{ Messages.GeneralChartPrefs_Dash, LineStyle.DASH.label },
				{ Messages.GeneralChartPrefs_Dot, LineStyle.DOT.label },
				{ Messages.GeneralChartPrefs_DashDot, LineStyle.DASHDOT.label },
				{ Messages.GeneralChartPrefs_DashDotDot, LineStyle.DASHDOTDOT.label },
			};
		addField(new RadioGroupFieldEditor("Chart.Grid.X.Style", Messages.GeneralChartPrefs_XStyle, 3, gridStyles, getFieldEditorParent(), true)); //$NON-NLS-1$
		addField(new RadioGroupFieldEditor("Chart.Grid.Y.Style", Messages.GeneralChartPrefs_YStyle, 3, gridStyles, getFieldEditorParent(), true)); //$NON-NLS-1$
		*/
	}
}
