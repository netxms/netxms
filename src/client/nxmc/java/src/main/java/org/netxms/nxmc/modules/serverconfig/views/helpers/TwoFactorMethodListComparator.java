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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.users.TwoFactorAuthenticationMethod;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.serverconfig.views.TwoFactorAuthenticationMethods;

/**
 * Comparator for two-factor authentication method list
 */
public class TwoFactorMethodListComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

      TwoFactorAuthenticationMethod m1 = (TwoFactorAuthenticationMethod)e1;
      TwoFactorAuthenticationMethod m2 = (TwoFactorAuthenticationMethod)e2;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
         case TwoFactorAuthenticationMethods.COLUMN_NAME:
            result = m1.getName().compareToIgnoreCase(m2.getName());
				break;
         case TwoFactorAuthenticationMethods.COLUMN_DRIVER:
            result = m1.getDriver().compareToIgnoreCase(m2.getDriver());
            break;
         case TwoFactorAuthenticationMethods.COLUMN_DESCRIPTION:
            result = m1.getDescription().compareToIgnoreCase(m2.getDescription());
				break;
         case TwoFactorAuthenticationMethods.COLUMN_STATUS:
            result = Boolean.compare(m1.isLoaded(), m2.isLoaded());
            break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
