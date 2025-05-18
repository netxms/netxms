/**
 * NetXMS - open source network management system
 * Copyright (C) 2021-2023 Raden Solutions
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
package org.netxms.nxmc.modules.tools;

import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.tools.views.IPAddressSearchView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.services.ToolDescriptor;
import org.xnap.commons.i18n.I18n;

/**
 * Tools perspective element for find by IP address tool 
 */
public class FindByIpAddressDescriptor implements ToolDescriptor
{
   private final I18n i18n = LocalizationHelper.getI18n(FindByIpAddressDescriptor.class);

   /**
    * @see org.netxms.nxmc.services.ToolDescriptor#getName()
    */
   @Override
   public String getName()
   {
      return i18n.tr("IP Address Search");
   }

   /**
    * @see org.netxms.nxmc.services.ToolDescriptor#getImage()
    */
   @Override
   public ImageDescriptor getImage()
   {
      return ResourceManager.getImageDescriptor("icons/tool-views/search_history.png");
   }

   /**
    * @see org.netxms.nxmc.services.ToolDescriptor#createView()
    */
   @Override
   public View createView()
   {
      return new IPAddressSearchView();
   }

   /**
    * @see org.netxms.nxmc.services.ToolDescriptor#getRequiredComponentId()
    */
   @Override
   public String getRequiredComponentId()
   {
      return null;
   }
}
