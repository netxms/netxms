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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.api.client.users.AuthCertificate;
import org.netxms.ui.eclipse.usermanager.Messages;
import org.netxms.ui.eclipse.usermanager.views.CertificateView;

/**
 * Label provider for certificate manager
 */
public class CertificateLabelProvider extends WorkbenchLabelProvider implements ITableLabelProvider
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return (columnIndex == 0) ? getImage(element) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
		   case CertificateView.COLUMN_ID:
				return (element instanceof AuthCertificate) ? Long.toString(((AuthCertificate)element).getId()) : null;
		   case CertificateView.COLUMN_TYPE:
				return Messages.get().CertificateLabelProvider_CERT_TYPE_TRUSTED_CA;
		   case CertificateView.COLUMN_COMMENTS:
				return (element instanceof AuthCertificate) ? ((AuthCertificate) element).getComments() : null;
		   case CertificateView.COLUMN_SUBJECT:
				return (element instanceof AuthCertificate) ? ((AuthCertificate) element).getSubject() : null;
		}
		return null;
	}
}
