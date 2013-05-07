/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.GridLayout;
import org.eclipse.draw2d.Label;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.console.resources.SharedColors;

/**
 * Figure which shows DCI values
 */
public class NodeLastValuesFigure extends Figure
{
	/**
	 * @param values
	 */
	public NodeLastValuesFigure(DciValue[] values)
	{
		GridLayout layout = new GridLayout(2, false);
		layout.horizontalSpacing = 10;
		setLayoutManager(layout);
		
		Color color = SharedColors.getColor(SharedColors.MAP_LAST_VALUES_TEXT, Display.getCurrent());
		for(DciValue v : values)
		{
			Label descr = new Label(v.getDescription());
			descr.setForegroundColor(color);
			add(descr);
			
			Label value = new Label(v.getValue());
			value.setForegroundColor(color);
			add(value);
		}
	}
}
