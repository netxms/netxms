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
package org.netxms.nxmc.base.views;

import java.util.ArrayList;
import java.util.List;
import java.util.ServiceLoader;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.services.ToolDescriptor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Tools perspective
 */
public class ToolsPerspective extends Perspective
{
   private static final Logger logger = LoggerFactory.getLogger(ToolsPerspective.class);

   private List<ToolDescriptor> elements = new ArrayList<ToolDescriptor>();

   /**
    * The constructor.
    */
   public ToolsPerspective()
   {
      super("tools", LocalizationHelper.getI18n(ToolsPerspective.class).tr("Tools"), ResourceManager.getImage("icons/perspective-tools.png"));

      ServiceLoader<ToolDescriptor> loader = ServiceLoader.load(ToolDescriptor.class, getClass().getClassLoader());
      for(ToolDescriptor e : loader)
      {
         logger.debug("Adding tools element " + e.getName());
         elements.add(e);
      }
      elements.sort((e1, e2) -> e1.getName().compareToIgnoreCase(e2.getName()));
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
      configuration.useGlobalViewId = true;
      configuration.ignoreViewContext = true;
      configuration.priority = 200;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      NXCSession session = Registry.getSession();
      for(ToolDescriptor e : elements)
      {
         if ((e.getRequiredComponentId() == null) || session.isServerComponentRegistered(e.getRequiredComponentId()))
         {
            View view = e.createView();
            addMainView(view);
            logger.debug("Added tools perspective view \"" + view.getName() + "\"");
         }
         else
         {
            logger.debug("Skipping tools perspective element \"" + e.getName() + "\" (component " + e.getRequiredComponentId() + " not available)");
         }
      }
   }
}
