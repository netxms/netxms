/**
 * 
 */
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.GridLayout;
import org.eclipse.draw2d.Label;
import org.eclipse.swt.graphics.Color;
import org.netxms.client.datacollection.DciValue;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DciValueFormatter;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * Figure which shows DCI values
 */
public class NodeLastValuesFigure extends Figure
{
   private final I18n i18n = LocalizationHelper.getI18n(NodeLastValuesFigure.class);

	/**
	 * @param values
	 */
	public NodeLastValuesFigure(DciValue[] values)
	{
		GridLayout layout = new GridLayout(2, false);
		layout.horizontalSpacing = 10;
		layout.marginWidth = 0;
		setLayoutManager(layout);
		
      Color color = ThemeEngine.getForegroundColor("Map.LastValues");
		for(DciValue v : values)
		{
			Label descr = new Label(v.getDescription());
			descr.setForegroundColor(color);
			add(descr);

         Label value = new Label(v.isDataCollectionError() ? i18n.tr("<< ERROR >>") : DciValueFormatter.format(v, true, DateFormatFactory.getTimeFormatter()));
			value.setForegroundColor(color);
			add(value);
		}
	}
}
