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
package org.netxms.ui.eclipse.perfview.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.perfview.views.TemplateGraphView;

/**
 * Label provider for historical data view
 */
public class TemplateGraphLabelProvider extends LabelProvider implements ITableLabelProvider
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case TemplateGraphView.COLUMN_NAME:
				return ((GraphSettings)element).getName();
			case TemplateGraphView.COLUMN_DCI_NAME:
			   StringBuilder names = new StringBuilder();
			   ChartDciConfig[] configs = ((GraphSettings)element).getDciList();
			   for(int i = 0; i < configs.length; i++)
			   {
			      names.append(configs[i].dciName);
			      if(i+1 != configs.length)
			         names.append(", ");
			   }
            return names.toString();
         case TemplateGraphView.COLUMN_DCI_DESCRIPTION:
            StringBuilder description = new StringBuilder();
            ChartDciConfig[] config = ((GraphSettings)element).getDciList();
            for(int i = 0; i < config.length; i++)
            {
               description.append(config[i].dciDescription);
               if(i+1 != config.length)
                  description.append(", ");
            }
            return description.toString();
		}
		return null;
	}
}
