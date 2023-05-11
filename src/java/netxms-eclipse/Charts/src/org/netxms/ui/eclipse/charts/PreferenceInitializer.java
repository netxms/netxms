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
package org.netxms.ui.eclipse.charts;

import org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer;
import org.eclipse.jface.preference.IPreferenceStore;
import org.swtchart.LineStyle;

/**
 * Preference initializer for charts plugin
 */
public class PreferenceInitializer extends AbstractPreferenceInitializer
{
   /**
    * @see org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer#initializeDefaultPreferences()
    */
   @Override
   public void initializeDefaultPreferences()
   {
      IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
      ps.setDefault("Chart.Colors.Background", "255,255,255"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.0", "51,160,44"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.1", "31,120,180"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.2", "255,127,0"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.3", "227,26,28"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.4", "106,61,154"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.5", "166,206,227"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.6", "178,223,138"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.7", "251,154,153"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.8", "253,191,111"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.9", "202,178,214"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.10", "177,89,40"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.11", "27,158,119"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.12", "217,95,2"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.13", "117,112,179"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.14", "230,171,2"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Data.15", "102,166,30"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.DialNeedle", "51,78,113"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.DialNeedlePin", "27,30,33"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.DialScale", "118,120,122"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Legend", "0,0,0"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.PlotArea", "255,255,255"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Selection", "0,0,128"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Colors.Title", "0,0,0"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Axis.X.Color", "22,22,22"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Axis.Y.Color", "22,22,22"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Grid.X.Color", "232,232,232"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Grid.X.Style", LineStyle.DOT.label); //$NON-NLS-1$
      ps.setDefault("Chart.Grid.Y.Color", "232,232,232"); //$NON-NLS-1$ //$NON-NLS-2$
      ps.setDefault("Chart.Grid.Y.Style", LineStyle.DOT.label); //$NON-NLS-1$
      ps.setDefault("Chart.EnableZoom", true); //$NON-NLS-1$
      ps.setDefault("Chart.ShowTitle", false); //$NON-NLS-1$
      ps.setDefault("Chart.ShowToolTips", true); //$NON-NLS-1$
   }
}
