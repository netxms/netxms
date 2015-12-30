/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import java.net.InetAddress;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.InetAddressListElement;

/**
 * Comparator for address list elements
 */
public class AddressListElementComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		InetAddressListElement a1 = (InetAddressListElement)e1;
		InetAddressListElement a2 = (InetAddressListElement)e2;
		
		int rc = compareIpAddresses(a1.getBaseAddress(), a2.getBaseAddress());
		if (rc == 0)
		{
		   rc = a1.getType() - a2.getType();
		   if (rc == 0)
		   {
		      rc = (a1.getType() == InetAddressListElement.SUBNET) ? a1.getMaskBits() - a2.getMaskBits() : compareIpAddresses(a1.getEndAddress(), a2.getEndAddress());
		   }
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

		int rc = b1.length - b2.length;
		if (rc != 0)
		   return rc;
		
		for(int i = 0; i < b1.length; i++)
		{
			rc = ((int)b1[i] & 0xFF) - ((int)b2[i] & 0xFF);
			if (rc != 0)
				return Integer.signum(rc);
		}
		return 0;
	}
}
