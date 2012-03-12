/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import java.net.InetAddress;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.IpAddressListElement;

/**
 * Comparator for address list elements
 */
public class AddressListElementComparator extends ViewerComparator
{
	private static final long serialVersionUID = 1L;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		IpAddressListElement a1 = (IpAddressListElement)e1;
		IpAddressListElement a2 = (IpAddressListElement)e2;
		
		int rc = compareIpAddresses(a1.getAddr1(), a2.getAddr1());
		if (rc == 0)
		{
			rc = compareIpAddresses(a1.getAddr2(), a2.getAddr2());
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
