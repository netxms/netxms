/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.ui.eclipse.charts.widgets.internal;

import org.eclipse.birt.chart.model.component.Axis;
import org.eclipse.birt.chart.model.component.Label;
import org.eclipse.birt.chart.script.ChartEventHandlerAdapter;
import org.eclipse.birt.chart.script.IChartScriptContext;
import org.netxms.client.datacollection.DataFormatter;

public class AxisLabelFormatter extends ChartEventHandlerAdapter
{
   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.birt.chart.script.ChartEventHandlerAdapter#beforeDrawAxisLabel(org.eclipse.birt.chart.model.component.Axis,
    * org.eclipse.birt.chart.model.component.Label, org.eclipse.birt.chart.script.IChartScriptContext)
    */
   @Override
   public void beforeDrawAxisLabel(Axis axis, Label label, IChartScriptContext icsc)
   {
      double value = Double.parseDouble(label.getCaption().getValue());
      label.getCaption().setValue(DataFormatter.roundDecimalValue(value, 0.05, 2));

   }
}
