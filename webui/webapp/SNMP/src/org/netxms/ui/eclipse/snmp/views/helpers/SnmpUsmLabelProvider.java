/**
 * 
 */
package org.netxms.ui.eclipse.snmp.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.ui.eclipse.snmp.Messages;

/**
 * Label provider for SnmpUsmCredentials class
 */
public class SnmpUsmLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final String[] authMethodName = { Messages.get().SnmpUsmLabelProvider_AuthNone, Messages.get().SnmpUsmLabelProvider_AuthMD5, Messages.get().SnmpUsmLabelProvider_AuthSHA1 };
	private static final String[] privMethodName = { Messages.get().SnmpUsmLabelProvider_EncNone, Messages.get().SnmpUsmLabelProvider_EncDES, Messages.get().SnmpUsmLabelProvider_EncAES };
	
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
