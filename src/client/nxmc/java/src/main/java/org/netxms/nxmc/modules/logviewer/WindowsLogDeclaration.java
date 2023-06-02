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
package org.netxms.nxmc.modules.logviewer;

import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Windows event log declaration object
 */
public class WindowsLogDeclaration extends LogDeclaration
{
   private static final I18n i18n = LocalizationHelper.getI18n(WindowsLogDeclaration.class);

   /**
    * Constructor 
    * 
    * @param viewName
    * @param logName
    * @param filterColumn
    */
   public WindowsLogDeclaration()
   {
      super(i18n.tr("Windows Events"), "WindowsEventLog", "node_id");
   }
   
   /**
    * @see org.netxms.nxmc.modules.logviewer.LogDeclaration#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return ObjectTool.isContainerObject(object) || ((object instanceof Node) && (((Node)object).getPlatformName().startsWith("windows")));
   }

}
