/**
 * 
 */
package org.netxms.ui.eclipse.snmp.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.ui.eclipse.snmp.views.SnmpCredentials;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

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
	   int result;
		SnmpUsmCredential c1 = (SnmpUsmCredential)e1;
		SnmpUsmCredential c2 = (SnmpUsmCredential)e2;
		
		switch((Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
      {
         case SnmpCredentials.USM_CRED_USER_NAME:
            result = c1.getName().compareToIgnoreCase(c2.getName());
            break;
         case SnmpCredentials.USM_CRED_AUTHENTICATION:
            result = c1.getAuthMethod() - c2.getAuthMethod();
            break;
         case SnmpCredentials.USM_CRED_ENCRYPTION:
            result = c1.getPrivMethod() - c2.getPrivMethod();
            break;
         case SnmpCredentials.USM_CRED_AUTH_PASSWORD:
            result = c1.getAuthPassword().compareTo(c2.getAuthPassword());
            break;
         case SnmpCredentials.USM_CRED_ENC_PASSWORD:
            result = c1.getPrivPassword().compareTo(c2.getPrivPassword());
            break;
         case SnmpCredentials.USM_CRED_COMMENTS:
            result = c1.getComment().compareToIgnoreCase(c2.getComment());
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
