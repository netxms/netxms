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
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.services.MonitorPerspectiveElement;
import org.xnap.commons.i18n.I18n;

/**
 * Monitor perspective
 */
public class MonitorPerspective extends Perspective
{
   private static final I18n i18n = LocalizationHelper.getI18n(MonitorPerspective.class);

   private List<MonitorPerspectiveElement> elements = new ArrayList<MonitorPerspectiveElement>();

   /**
    * The constructor.
    */
   public MonitorPerspective()
   {
      super("Monitor", i18n.tr("Monitor"), ResourceManager.getImage("icons/perspective-monitor.png"));

      ServiceLoader<MonitorPerspectiveElement> loader = ServiceLoader.load(MonitorPerspectiveElement.class, getClass().getClassLoader());
      for(MonitorPerspectiveElement e : loader)
      {
         elements.add(e);
      }
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
      for(MonitorPerspectiveElement e : elements)
         addMainView(e.createView());
   }
}
