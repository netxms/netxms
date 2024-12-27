/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.nxmc.modules.datacollection.views.TemplateGraphView;

/**
 * Label provider for historical data view
 */
public class TemplateGraphLabelProvider extends LabelProvider implements ITableLabelProvider
{
   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
      ChartDciConfig[] configs;
		switch(columnIndex)
		{
			case TemplateGraphView.COLUMN_NAME:
				return ((GraphDefinition)element).getName();
			case TemplateGraphView.COLUMN_DCI_NAMES:
			   StringBuilder names = new StringBuilder();
            configs = ((GraphDefinition)element).getDciList();
			   for(int i = 0; i < configs.length; i++)
			   {
			      names.append(configs[i].dciName);
               if (i + 1 != configs.length)
			         names.append(", ");
			   }
            return names.toString();
         case TemplateGraphView.COLUMN_DCI_DESCRIPTIONS:
            StringBuilder description = new StringBuilder();
            configs = ((GraphDefinition)element).getDciList();
            for(int i = 0; i < configs.length; i++)
            {
               description.append(configs[i].dciDescription);
               if (i + 1 != configs.length)
                  description.append(", ");
            }
            return description.toString();
         case TemplateGraphView.COLUMN_DCI_TAGS:
            StringBuilder tags = new StringBuilder();
            configs = ((GraphDefinition)element).getDciList();
            for(int i = 0; i < configs.length; i++)
            {
               tags.append(configs[i].dciTag);
               if (i + 1 != configs.length)
                  tags.append(", ");
            }
            return tags.toString();
		}
		return null;
	}
}
