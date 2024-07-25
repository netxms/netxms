/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Remote control view (embedded VNC viewer)
 */
public class RemoteControlView extends AdHocObjectView
{
   /**
    * Create VNC viewer
    *
    * @param node target node
    * @param contextId context ID
    */
   public RemoteControlView(AbstractNode node, long contextId)
   {
      super(LocalizationHelper.getI18n(ScreenshotView.class).tr("Remote Control"), ResourceManager.getImageDescriptor("icons/object-views/remote-desktop.png"), "objects.vncviewer", node.getObjectId(), contextId, false);
   }

   /**
    * Create VNC viewer
    */
   public RemoteControlView()
   {
      super(null, null, null, 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      new Label(parent, SWT.NONE).setText("Not implemented yet for web client");
   }
}
