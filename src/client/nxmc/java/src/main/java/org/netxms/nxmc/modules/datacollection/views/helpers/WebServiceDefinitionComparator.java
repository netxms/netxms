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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.datacollection.views.WebServiceManager;

/**
 * Comparator for DCI summary table descriptors
 */
public class WebServiceDefinitionComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
	   WebServiceDefinition d1 = (WebServiceDefinition)e1;
	   WebServiceDefinition d2 = (WebServiceDefinition)e2;
		
		int result;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
		   case WebServiceManager.COLUMN_AUTHENTICATION:
		      result = d1.getAuthenticationType().toString().compareToIgnoreCase(d2.getAuthenticationType().toString());
		      break;
		   case WebServiceManager.COLUMN_DESCRIPTION:
		      result = d1.getDescription().compareToIgnoreCase(d2.getDescription());
		      break;
		   case WebServiceManager.COLUMN_LOGIN:
		      result = d1.getLogin().compareToIgnoreCase(d2.getLogin());
		      break;
         case WebServiceManager.COLUMN_METHOD:
            result = d1.getHttpRequestMethod().toString().compareToIgnoreCase(d2.getHttpRequestMethod().toString());
            break;
		   case WebServiceManager.COLUMN_NAME:
		      result = d1.getName().compareToIgnoreCase(d2.getName());
		      break;
		   case WebServiceManager.COLUMN_RETENTION_TIME:
		      result = d1.getCacheRetentionTime() - d2.getCacheRetentionTime();
		      break;
		   case WebServiceManager.COLUMN_TIMEOUT:
		      result = d1.getRequestTimeout() - d2.getRequestTimeout();
		      break;
		   case WebServiceManager.COLUMN_URL:
		      result = d1.getUrl().compareToIgnoreCase(d2.getUrl());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
