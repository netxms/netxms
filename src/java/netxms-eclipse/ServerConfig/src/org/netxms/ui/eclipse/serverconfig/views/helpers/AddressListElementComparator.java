/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import java.net.InetAddress;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.InetAddressListElement;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.serverconfig.views.NetworkDiscoveryConfigurator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for address list elements
 */
public class AddressListElementComparator extends ViewerComparator
{
   private NXCSession session = ConsoleSharedData.getSession();
   private boolean isDiscoveryTarget;
   
   public AddressListElementComparator(boolean isDiscoveryTarget)
   {
      this.isDiscoveryTarget = isDiscoveryTarget; 
   }
   
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
      int result;
		InetAddressListElement a1 = (InetAddressListElement)e1;
		InetAddressListElement a2 = (InetAddressListElement)e2;
		
		 
      switch((Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
      {
         case NetworkDiscoveryConfigurator.RANGE:
            result = compareIpAddresses(a1.getBaseAddress(), a2.getBaseAddress());
            if (result == 0)
            {
               result = a1.getType() - a2.getType();
               if (result == 0)
               {
                  result = (a1.getType() == InetAddressListElement.SUBNET) ? a1.getMaskBits() - a2.getMaskBits() : compareIpAddresses(a1.getEndAddress(), a2.getEndAddress());
               }
            }
            break;
         case NetworkDiscoveryConfigurator.PROXY:
            if(isDiscoveryTarget)
            {
               String name1 = (a1.getProxyId() != 0) ? session.getObjectName(a1.getProxyId()) : session.getZoneName(a1.getZoneUIN());
               String name2 = (a2.getProxyId() != 0) ? session.getObjectName(a2.getProxyId()) : session.getZoneName(a2.getZoneUIN());
               result = name1.compareTo(name2);
            }
            else
               result = a1.getComment().compareTo(a2.getComment());
            break;
         case NetworkDiscoveryConfigurator.COMMENT:
            result = a1.getComment().compareTo(a2.getComment());
            break;
         default:
            result = 0;
            break;
      }
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
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
