/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.nxmc.modules.datacollection.views.WebServiceManager;

/**
 * Label provider for web service definitions
 */
public class WebServiceDefinitionLabelProvider extends LabelProvider implements ITableLabelProvider
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
      WebServiceDefinition d = (WebServiceDefinition)element;
      switch(columnIndex)
      {
         case WebServiceManager.COLUMN_AUTHENTICATION:
            return d.getAuthenticationType().toString();
         case WebServiceManager.COLUMN_DESCRIPTION:
            return d.getDescription();
         case WebServiceManager.COLUMN_LOGIN:
            return d.getLogin();
         case WebServiceManager.COLUMN_METHOD:
            return d.getHttpRequestMethod().toString();
         case WebServiceManager.COLUMN_NAME:
            return d.getName();
         case WebServiceManager.COLUMN_RETENTION_TIME:
            return Integer.toString(d.getCacheRetentionTime());
         case WebServiceManager.COLUMN_TIMEOUT:
            return Integer.toString(d.getRequestTimeout());
         case WebServiceManager.COLUMN_URL:
            return d.getUrl();
      }
      return null;
   }
}
