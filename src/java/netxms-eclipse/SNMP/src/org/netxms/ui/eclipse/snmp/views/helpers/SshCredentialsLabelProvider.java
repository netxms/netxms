/**
 * 
 */
package org.netxms.ui.eclipse.snmp.views.helpers;

import java.util.List;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.SSHCredentials;
import org.netxms.client.SshKeyPair;
import org.netxms.ui.eclipse.snmp.views.NetworkCredentials;

/**
 * Label provider for SshCredential class
 */
public class SshCredentialsLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private List<SshKeyPair> keyList;

   /**
    * @param keyList the keyList to set
    */
   public void setKeyList(List<SshKeyPair> keyList)
   {
      this.keyList = keyList;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		SSHCredentials crd = (SSHCredentials)element;		
		switch(columnIndex)
      {
         case NetworkCredentials.SSH_CRED_LOGIN:
            return crd.getLogin();
         case NetworkCredentials.SSH_CRED_PASSWORD:
            return crd.getPassword();
         case NetworkCredentials.SSH_CRED_KEY:
            if (crd.getKeyId() == 0)
               return "";

            if (keyList != null)
            {
               for(SshKeyPair kp : keyList)
                  if (kp.getId() == crd.getKeyId())
                     return kp.getName();
            }

            return "[" + Integer.toString(crd.getKeyId()) + "]";
      }
		return null;
	}
}
