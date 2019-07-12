/**
 * 
 */
package org.netxms.ui.eclipse.snmp.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.views.SnmpCredentials;

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
		switch(columnIndex)
      {
		   case SnmpCredentials.USM_CRED_USER_NAME:
		      return c.getName();
		   case SnmpCredentials.USM_CRED_AUTHENTICATION:
		      return authMethodName[c.getAuthMethod()];
		   case SnmpCredentials.USM_CRED_ENCRYPTION:
		      return privMethodName[c.getPrivMethod()];
		   case SnmpCredentials.USM_CRED_AUTH_PASSWORD:
		      return c.getAuthPassword();
		   case SnmpCredentials.USM_CRED_ENC_PASSWORD:
		      return c.getPrivPassword();
		   case SnmpCredentials.USM_CRED_COMMENTS:
		      return c.getComment();
      }
		return null;
	}
}
