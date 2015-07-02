/**
 * 
 */
package org.netxms.ui.eclipse.snmp.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.netxms.client.snmp.SnmpUsmCredential;

/**
 * Comparator for SnmpUsmCredentials class
 */
public class SnmpUsmComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		SnmpUsmCredential c1 = (SnmpUsmCredential)e1;
		SnmpUsmCredential c2 = (SnmpUsmCredential)e2;
		
		int result = c1.getName().compareToIgnoreCase(c2.getName());
		if (result == 0)
		{
			result = c1.getAuthMethod() - c2.getAuthMethod();
			if (result == 0)
			{
				result = c1.getPrivMethod() - c2.getPrivMethod();
			}
		}
		return result;
	}
}
