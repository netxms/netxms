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
package org.netxms.nxmc.modules.actions.views.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.ServerActionType;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Base label provider for actions
 */
public class BaseActionLabelProvider extends LabelProvider
{
   private Map<ServerActionType, Image> images;

   /**
    * Default constructor
    */
   public BaseActionLabelProvider()
   {
      images = new HashMap<ServerActionType, Image>();
      images.put(ServerActionType.LOCAL_COMMAND, ResourceManager.getImageDescriptor("icons/actions/exec_local.png").createImage());
      images.put(ServerActionType.AGENT_COMMAND, ResourceManager.getImageDescriptor("icons/actions/exec_remote.png").createImage());
      images.put(ServerActionType.SSH_COMMAND, ResourceManager.getImageDescriptor("icons/actions/exec_remote.png").createImage());
      images.put(ServerActionType.NOTIFICATION, ResourceManager.getImageDescriptor("icons/actions/email.png").createImage());
      images.put(ServerActionType.FORWARD_EVENT, ResourceManager.getImageDescriptor("icons/actions/fwd_event.png").createImage());
      images.put(ServerActionType.NXSL_SCRIPT, ResourceManager.getImageDescriptor("icons/actions/exec_script.gif").createImage());
   }
   
   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      for(Image img : images.values())
         img.dispose();
      super.dispose();
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
   @Override
   public Image getImage(Object element)
   {
      return images.get(((ServerAction)element).getType());
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
   @Override
   public String getText(Object element)
   {
      return ((ServerAction)element).getName();
   }
}
