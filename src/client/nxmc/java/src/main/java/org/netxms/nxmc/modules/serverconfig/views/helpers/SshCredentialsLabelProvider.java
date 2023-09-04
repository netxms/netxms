/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import java.util.List;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.SSHCredentials;
import org.netxms.client.SshKeyPair;
import org.netxms.nxmc.modules.serverconfig.views.NetworkCredentialsEditor;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Label provider for SshCredential class
 */
public class SshCredentialsLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private List<SshKeyPair> keyList;
   private boolean maskMode = true;

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
         case NetworkCredentialsEditor.COLUMN_SSH_LOGIN:
            return crd.getLogin();
         case NetworkCredentialsEditor.COLUMN_SSH_PASSWORD:
            return maskMode ? WidgetHelper.maskPassword(crd.getPassword()) : crd.getPassword();
         case NetworkCredentialsEditor.COLUMN_SSH_KEY:
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

   /**
    * @return the maskMode
    */
   public boolean isMaskMode()
   {
      return maskMode;
   }

   /**
    * @param maskMode the maskMode to set
    */
   public void setMaskMode(boolean maskMode)
   {
      this.maskMode = maskMode;
   }
}
