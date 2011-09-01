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
package org.netxms.ui.eclipse.perfview.views.helpers;

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.ui.IMemento;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.charts.api.ChartPluginSharedData;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Factory class for creating graph settings
 *
 */
public class GraphSettingsFactory
{
	/**
	 * Create default settings for line chart
	 * 
	 * @return graph settings object
	 */
	public static GraphSettings createDefault()
	{
		GraphSettings settings = new GraphSettings();
		
		IPreferenceStore preferenceStore = ChartPluginSharedData.getPreferenceStore();
		settings.setTooltipsEnabled(preferenceStore.getBoolean("Chart.ShowToolTips"));
		settings.setZoomEnabled(preferenceStore.getBoolean("Chart.EnableZoom"));
		settings.setBackgroundColor(ColorConverter.getColorFromPreferencesAsInt(preferenceStore, "Chart.Colors.Background"));
		settings.setSelectionColor(ColorConverter.getColorFromPreferencesAsInt(preferenceStore, "Chart.Colors.Selection"));

		// Create default item styles
		GraphItemStyle[] styles = new GraphItemStyle[GraphSettings.MAX_GRAPH_ITEM_COUNT];
		for(int i = 0; i < GraphSettings.MAX_GRAPH_ITEM_COUNT; i++)
			styles[i] = new GraphItemStyle(GraphItemStyle.LINE, ColorConverter.getColorFromPreferencesAsInt(preferenceStore, "Chart.Colors.Data." + i), 0, 0);
		settings.setItemStyles(styles);
		
		return settings;
	}
	
	/**
	 * Create graph settings object from memento
	 * 
	 * @param memento
	 * @return
	 */
	public static GraphSettings createFromMemento(IMemento memento)
	{
		GraphSettings settings = createDefault();
		
		return settings;
	}
}
