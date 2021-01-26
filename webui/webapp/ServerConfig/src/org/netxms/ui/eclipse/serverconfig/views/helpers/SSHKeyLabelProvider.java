/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.SshKeyData;
import org.netxms.ui.eclipse.serverconfig.views.SshKeyConfigurator;

/**
 * SSH key label provider
 */
public class SSHKeyLabelProvider extends LabelProvider implements ITableLabelProvider
{

   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      SshKeyData key = (SshKeyData)element;

      switch(columnIndex)
      {
         case SshKeyConfigurator.COLUMN_ID:
            return Integer.toString(key.getId());
         case SshKeyConfigurator.COLUMN_NMAE:
            return key.getName();
      }
      return "";
   }

}
