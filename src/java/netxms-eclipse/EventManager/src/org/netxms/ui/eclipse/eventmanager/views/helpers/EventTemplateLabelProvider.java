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
package org.netxms.ui.eclipse.eventmanager.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.eventmanager.widgets.EventTemplateList;

/**
 * Label provider for event template objects
 */
public class EventTemplateLabelProvider extends WorkbenchLabelProvider implements ITableLabelProvider
{
   boolean isDialog;

   /**
    * Create label provider.
    *
    * @param isDialog true if owning widget is part of dialog
    */
	public EventTemplateLabelProvider(boolean isDialog)
   {
	   this.isDialog = isDialog;
   }

   /**
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
      return (columnIndex == 0) ? getImage(element) : null;
	}

	/**
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
      EventTemplate eventTemplate = (EventTemplate)element;
		switch(columnIndex)
		{
			case EventTemplateList.COLUMN_CODE:
            return Long.toString(eventTemplate.getCode());
			case EventTemplateList.COLUMN_NAME:
            return eventTemplate.getName();
			case EventTemplateList.COLUMN_SEVERITY:
            if (isDialog)
               return eventTemplate.getTagList();
			   else
               return StatusDisplayInfo.getStatusText(eventTemplate.getSeverity());
			case EventTemplateList.COLUMN_FLAGS:
            return buildFlagString(eventTemplate);
			case EventTemplateList.COLUMN_MESSAGE:
            return eventTemplate.getMessage();
         case EventTemplateList.COLUMN_TAGS:
            return eventTemplate.getTagList();
		}
		return null;
	}

   /**
    * Build flag string for given event template.
    *
    * @param eventTemplate event template
    * @return flag string
    */
   private static String buildFlagString(EventTemplate eventTemplate)
   {
      StringBuilder sb = new StringBuilder();
      if ((eventTemplate.getFlags() & EventTemplate.FLAG_WRITE_TO_LOG) != 0)
      {
         sb.append("log");
      }
      if ((eventTemplate.getFlags() & EventTemplate.FLAG_DO_NOT_MONITOR) != 0)
      {
         if (sb.length() > 0)
            sb.append(", ");
         sb.append("hide");
      }
      return sb.toString();
   }
}
