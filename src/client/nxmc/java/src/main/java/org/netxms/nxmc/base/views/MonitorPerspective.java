/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.base.views;

import java.util.ArrayList;
import java.util.List;
import java.util.ServiceLoader;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.services.MonitorDescriptor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Monitor perspective
 */
public class MonitorPerspective extends Perspective
{
   private static final Logger logger = LoggerFactory.getLogger(MonitorPerspective.class);
   private static final I18n i18n = LocalizationHelper.getI18n(MonitorPerspective.class);

   private List<MonitorDescriptor> monitors = new ArrayList<MonitorDescriptor>();

   /**
    * The constructor.
    */
   public MonitorPerspective()
   {
      super("Monitor", i18n.tr("Monitor"), ResourceManager.getImage("icons/perspective-monitor.png"));

      ServiceLoader<MonitorDescriptor> loader = ServiceLoader.load(MonitorDescriptor.class, getClass().getClassLoader());
      for(MonitorDescriptor e : loader)
         monitors.add(e);

      monitors.sort((MonitorDescriptor m1, MonitorDescriptor m2) -> m1.getDisplayName().compareToIgnoreCase(m2.getDisplayName()));
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
      configuration.priority = 150;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      NXCSession session = Registry.getSession();
      for(MonitorDescriptor e : monitors)
      {
         String componentId = e.getRequiredComponentId();
         if ((componentId == null) || session.isServerComponentRegistered(componentId))
         {
            AbstractTraceView view = e.createView();
            addMainView(view);
            logger.debug("Added monitor perspective view \"" + view.getName() + "\"");
         }
         else
         {
            logger.debug("Monitor perspective view \"" + e.getDisplayName() + "\" blocked by component filter");
         }
      }
   }
}
