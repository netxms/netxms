/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.users.TwoFactorAuthenticationMethod;
import org.netxms.nxmc.modules.serverconfig.views.TwoFactorAuthenticationMethods;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Label provider for two factor authentication method list
 */
public class TwoFactorMethodLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private Image imageInactive;
   private Image imageActive;
   
   public TwoFactorMethodLabelProvider()
   {
      imageInactive = ResourceManager.getImage("icons/inactive.gif");
      imageActive = ResourceManager.getImage("icons/active.gif");
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
      if (columnIndex == 0)
         return ((TwoFactorAuthenticationMethod)element).isLoaded() ? imageActive : imageInactive;
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
         case TwoFactorAuthenticationMethods.COLUMN_NAME:
            return ((TwoFactorAuthenticationMethod)element).getName();
         case TwoFactorAuthenticationMethods.COLUMN_DESCRIPTION:
            return ((TwoFactorAuthenticationMethod)element).getDescription();
         case TwoFactorAuthenticationMethods.COLUMN_DRIVER:
            return ((TwoFactorAuthenticationMethod)element).getDriver();
         case TwoFactorAuthenticationMethods.COLUMN_STATUS:
            return ((TwoFactorAuthenticationMethod)element).isLoaded() ? "OK" : "FAIL";
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      imageInactive.dispose();
      imageActive.dispose();
   }
}
