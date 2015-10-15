/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages.helpers;

import java.net.InetAddress;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.base.InetAddressEx;

/**
 * Comparator for subnets
 */
public class SubnetComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		InetAddressEx a1 = (InetAddressEx)e1;
		InetAddressEx a2 = (InetAddressEx)e2;
		
		int rc = compareIpAddresses(a1.address, a2.address);
		if (rc == 0)
		{
			rc = a1.mask - a2.mask;
		}
		
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
	
	/**
	 * Compare two IP addresses
	 * 
	 * @param a1
	 * @param a2
	 * @return
	 */
	private int compareIpAddresses(InetAddress a1, InetAddress a2)
	{
		byte[] b1 = a1.getAddress();
		byte[] b2 = a2.getAddress();
		
		for(int i = 0; i < b1.length; i++)
		{
			int rc = b1[i] - b2[i];
			if (rc != 0)
				return Integer.signum(rc);
		}
		return 0;
	}
}
