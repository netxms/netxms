/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
package org.netxms.ui.eclipse.usermanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.users.AuthCertificate;
import org.netxms.ui.eclipse.usermanager.views.CertificateView;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for certificate objects
 */
public class CertificateComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

		switch((Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case CertificateView.COLUMN_ID:
				result = Long.signum(((AuthCertificate)e1).getId() - ((AuthCertificate)e2).getId());
				break;
			case CertificateView.COLUMN_TYPE:
				result = Integer.signum(((AuthCertificate)e1).getType() - ((AuthCertificate)e2).getType());
				break;
			case CertificateView.COLUMN_COMMENTS:
				result = ((AuthCertificate)e1).getComments().compareToIgnoreCase(((AuthCertificate)e2).getComments());
				break;
			case CertificateView.COLUMN_SUBJECT:
				result = ((AuthCertificate)e1).getSubject().compareToIgnoreCase(((AuthCertificate)e2).getSubject());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
