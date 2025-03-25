/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.logviewer.views;

import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.LogDescriptorRegistry;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.services.LogDescriptor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Log viewer perspective
 */
public class LogViewerPerspective extends Perspective
{
   private static final Logger logger = LoggerFactory.getLogger(LogViewerPerspective.class);

   /**
    * The constructor.
    */
   public LogViewerPerspective()
   {
      super("logs", LocalizationHelper.getI18n(LogViewerPerspective.class).tr("Logs"), ResourceManager.getImage("icons/perspective-logs.png"));
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = false;
      configuration.multiViewMainArea = true;
      configuration.hasSupplementalArea = false;
      configuration.priority = 100;
      configuration.ignoreViewContext = true; // There is no context in log perspective
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      NXCSession session = Registry.getSession();
      for(LogDescriptor log : Registry.getSingleton(LogDescriptorRegistry.class).getDescriptors())
      {
         if (log.isValidForSession(session))
         {
            LogViewer view = log.createView();
            addMainView(view);
            logger.debug("Added logs perspective view \"" + view.getName() + "\"");
         }
         else
         {
            logger.debug("Logs perspective view \"" + log.getViewTitle() + "\" blocked by filter");
         }
      }
   }
}
