/**
 * 
 */
package org.netxms.ui.eclipse.charts.preferencepages;

import org.eclipse.jface.preference.BooleanFieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.jface.preference.RadioGroupFieldEditor;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.netxms.ui.eclipse.charts.Activator;
import org.swtchart.LineStyle;

/**
 * @author victor
 *
 */
public class GeneralChartPrefs extends FieldEditorPreferencePage implements IWorkbenchPreferencePage
{
	@Override
	public void init(IWorkbench workbench)
	{
		setPreferenceStore(Activator.getDefault().getPreferenceStore());
	}

	@Override
	protected void createFieldEditors()
	{
		addField(new BooleanFieldEditor("Chart.ShowTitle", "Show chart title", getFieldEditorParent()));
		addField(new BooleanFieldEditor("Chart.ShowToolTips", "Show tooltips when mouse hovers over plot area", getFieldEditorParent()));

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
	}
}
