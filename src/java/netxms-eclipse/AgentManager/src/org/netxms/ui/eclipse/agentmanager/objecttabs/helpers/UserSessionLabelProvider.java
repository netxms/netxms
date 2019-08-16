/**
 * NetXMS - open source network management system
 * Copyright (C) 2019 Raden Solutions
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
package org.netxms.ui.eclipse.agentmanager.objecttabs.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.TableRow;
import org.netxms.ui.eclipse.agentmanager.objecttabs.UserSessionsTab;

/**
 * User session label provider
 */
public class UserSessionLabelProvider extends LabelProvider implements ITableLabelProvider
{
   UserSessionsTab tab;
   
   /**
    * Constructor 
    * 
    * @param tab
    */
   public UserSessionLabelProvider(UserSessionsTab tab)
   {
      this.tab = tab;
   }
   
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      int index = tab.getTable().getColumnIndex(UserSessionsTab.COLUMNS[columnIndex]);
      if(index == -1)
         return "";
      return ((TableRow)element).get(index).getValue();
   }

}
