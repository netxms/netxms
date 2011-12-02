/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.snmp.SnmpUsmCredential;

/**
 * Label provider for SnmpUsmCredentials class
 */
public class SnmpUsmLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final String[] authMethodName = { "NONE", "MD5", "SHA1" };
	private static final String[] privMethodName = { "NONE", "DES", "AES" };
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		SnmpUsmCredential c = (SnmpUsmCredential)element;
		StringBuilder sb = new StringBuilder(c.getName());
		sb.append(' ');
		sb.append(authMethodName[c.getAuthMethod()]);
		sb.append('/');
		sb.append(privMethodName[c.getPrivMethod()]);
		sb.append(' ');
		sb.append(c.getAuthPassword());
		sb.append('/');
		sb.append(c.getPrivPassword());
		return sb.toString();
	}
}
