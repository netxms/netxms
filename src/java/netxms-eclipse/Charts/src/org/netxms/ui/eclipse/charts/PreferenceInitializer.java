/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts;

import org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer;
import org.eclipse.jface.preference.IPreferenceStore;
import org.swtchart.LineStyle;

/**
 * Preference initializer for charts plugin
 */
public class PreferenceInitializer extends AbstractPreferenceInitializer
{
	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer#initializeDefaultPreferences()
	 */
	@Override
	public void initializeDefaultPreferences()
	{
	   // Set default values for preferences
	   IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
	   
	   ps.setDefault("Chart.Colors.Background", "240,240,240");
	   ps.setDefault("Chart.Colors.Data.0", "64,105,156");
	   ps.setDefault("Chart.Colors.Data.1", "158,65,62");
	   ps.setDefault("Chart.Colors.Data.2", "127,154,72");
	   ps.setDefault("Chart.Colors.Data.3", "105,81,133");
	   ps.setDefault("Chart.Colors.Data.4", "60,141,163");
	   ps.setDefault("Chart.Colors.Data.5", "204,123,56");
	   ps.setDefault("Chart.Colors.Data.6", "79,129,189");
	   ps.setDefault("Chart.Colors.Data.7", "192,80,77");
	   ps.setDefault("Chart.Colors.Data.8", "155,187,89");
	   ps.setDefault("Chart.Colors.Data.9", "128,100,162");
	   ps.setDefault("Chart.Colors.Data.10", "75,172,198");
	   ps.setDefault("Chart.Colors.Data.11", "247,150,70");
	   ps.setDefault("Chart.Colors.Data.12", "170,186,215");
	   ps.setDefault("Chart.Colors.Data.13", "217,170,169");
	   ps.setDefault("Chart.Colors.Data.14", "198,214,172");
	   ps.setDefault("Chart.Colors.Data.15", "186,176,201");
	   ps.setDefault("Chart.Colors.PlotArea", "255,255,255");
	   ps.setDefault("Chart.Colors.Selection", "0,0,128");
	   ps.setDefault("Chart.Colors.Title", "0,0,0");
	   ps.setDefault("Chart.Axis.X.Color", "22,22,22");
	   ps.setDefault("Chart.Axis.Y.Color", "22,22,22");
	   ps.setDefault("Chart.Grid.X.Color", "232,232,232");
	   ps.setDefault("Chart.Grid.X.Style", LineStyle.DOT.label);
	   ps.setDefault("Chart.Grid.Y.Color", "232,232,232");
	   ps.setDefault("Chart.Grid.Y.Style", LineStyle.DOT.label);
	   ps.setDefault("Chart.EnableZoom", true);
	   ps.setDefault("Chart.ShowTitle", false);
	   ps.setDefault("Chart.ShowToolTips", true);
	}
}
