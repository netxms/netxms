/**
 * 
 */
package org.netxms.ui.eclipse.snmp.views.helpers;

import java.util.List;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.SshCredential;
import org.netxms.client.SshKeyPair;
import org.netxms.ui.eclipse.snmp.views.NetworkCredentials;

/**
 * Label provider for SshCredential class
 */
public class SshCredLabelProvider extends LabelProvider implements ITableLabelProvider
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
		SshCredential crd = (SshCredential)element;		
		switch(columnIndex)
      {
         case NetworkCredentials.SSH_CRED_LOGIN:
            return crd.getLogin();
         case NetworkCredentials.SSH_CRED_PASSWORD:
            return crd.getPassword();
         case NetworkCredentials.SSH_CRED_KEY:
            if (keyList != null)
            {
               for(SshKeyPair kp : keyList)
                  if (kp.getId() == crd.getKeyId())
                     return kp.getName();
            }
            else
               return "[" + Integer.toString(crd.getKeyId()) + "]";
      }
		return null;
	}
}
