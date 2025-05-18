/**
 * NetXMS - open source network management system
 * Copyright (C) 2021-2025 Raden Solutions
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
package org.netxms.nxmc.modules.snmp;

import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.snmp.views.MibExplorer;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.services.ToolDescriptor;
import org.xnap.commons.i18n.I18n;

/**
 * Tools perspective element for MIB explorer
 */
public class MibExplorerToolDescriptor implements ToolDescriptor
{
   private final I18n i18n = LocalizationHelper.getI18n(MibExplorerToolDescriptor.class);

   /**
    * @see org.netxms.nxmc.services.ToolDescriptor#getName()
    */
   @Override
   public String getName()
   {
      return i18n.tr("MIB explorer");
   }

   /**
    * @see org.netxms.nxmc.services.ToolDescriptor#getImage()
    */
   @Override
   public ImageDescriptor getImage()
   {
      return ResourceManager.getImageDescriptor("icons/object-views/mibexplorer.gif");
   }

   /**
    * @see org.netxms.nxmc.services.ToolDescriptor#createView()
    */
   @Override
   public View createView()
   {
      return new MibExplorer(0, 0, true);
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
