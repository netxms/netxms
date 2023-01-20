/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.dashboards.propertypages.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.dashboards.config.ObjectToolsConfig.Tool;
import org.netxms.nxmc.modules.objecttools.ObjectToolsCache;

/**
 * Label provider for tool list
 */
public class ToolListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private NXCSession session;
   private ObjectToolsCache objectToolsCache;
   
   /**
    * Constructor
    */
   public ToolListLabelProvider()
   {
      session = Registry.getSession();
      objectToolsCache = ObjectToolsCache.getInstance();
   }

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
      Tool t = (Tool)element;
      switch(columnIndex)
      {
         case 0:
            return t.name;
         case 1:
            return session.getObjectName(t.objectId);
         case 2:
            ObjectTool ot = objectToolsCache.findTool(t.toolId);
            return (ot != null) ? ot.getName() : ("[" + t.toolId + "]");
      }
      return null;
   }
}
