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
package org.netxms.nxmc.modules.incidents;

import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.incidents.views.IncidentsView;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Incident management perspective
 */
public class IncidentsPerspective extends Perspective
{
   /**
    * Create incidents perspective
    */
   public IncidentsPerspective()
   {
      super("incidents", LocalizationHelper.getI18n(IncidentsPerspective.class).tr("Incidents"), ResourceManager.getImage("icons/perspective-incidents.png"));
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = false;
      configuration.hasSupplementalArea = false;
      configuration.multiViewNavigationArea = false;
      configuration.multiViewMainArea = false;
      configuration.priority = 27; // After alarms (26)
      configuration.ignoreViewContext = true;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      setMainView(new IncidentsView());
   }
}
