/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.objects;

import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.LogDescriptorRegistry;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.services.LogDescriptor;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for building object context menu
 */
public class ObjectLogMenuManager extends MenuManager
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectLogMenuManager.class);

   private AbstractObject object;
   private long contextId;
   private ViewPlacement viewPlacement;

   /**
    * Create new menu manager for object's "Logs" menu
    *
    * @param view parent view
    * @param viewPlacement view placement
    * @param parent object to create menu for
    */
   public ObjectLogMenuManager(AbstractObject object, long contextId, ViewPlacement viewPlacement)
   {
      this.contextId = contextId;
      this.viewPlacement = viewPlacement;
      this.object = object;

      setMenuText(i18n.tr("&Logs"));

      List<LogDescriptor> descriptors = Registry.getSingleton(LogDescriptorRegistry.class).getDescriptors();
      for(LogDescriptor d : descriptors)
         addAction(this, d);
   }

   /**
    * Add action to given menu manager
    *
    * @param manager menu manager
    * @param descriptor log descriptor
    */
   protected void addAction(IMenuManager manager, LogDescriptor descriptor)
   {
      if (descriptor.isApplicableForObject(object))
      {
         Action action = new Action(descriptor.getMenuItemText(), ResourceManager.getImageDescriptor("icons/log-viewer/" + descriptor.getLogName() + ".png")) {
            @Override
            public void run()
            {
               viewPlacement.openView(descriptor.createContextView(object, contextId));
            }
         };
         manager.add(action);
      }
   }
}
