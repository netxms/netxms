/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.EntityMIBView;
import org.netxms.nxmc.modules.objects.views.InterfacesView;
import org.netxms.nxmc.modules.objects.views.ObjectBrowser;
import org.netxms.nxmc.modules.objects.views.ObjectOverviewView;
import org.netxms.nxmc.modules.objects.views.PerformanceView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Object browser perspective
 */
public class ObjectsPerspective extends Perspective
{
   private static I18n i18n = LocalizationHelper.getI18n(ObjectsPerspective.class);

   /**
    * @param name
    */
   public ObjectsPerspective()
   {
      super("Objects", i18n.tr("Objects"), ResourceManager.getImage("icons/perspective-objects.png"));
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = true;
      configuration.hasSupplementalArea = false;
      configuration.multiViewNavigationArea = true;
      configuration.multiViewMainArea = true;
      configuration.priority = 20;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      addNavigationView(new ObjectBrowser(i18n.tr("Network"), ResourceManager.getImageDescriptor("icons/objecttree-network.png"), SubtreeType.NETWORK));
      addNavigationView(new ObjectBrowser(i18n.tr("Infrastructure"), ResourceManager.getImageDescriptor("icons/objecttree-infrastructure.png"), SubtreeType.INFRASTRUCTURE));
      addNavigationView(new ObjectBrowser(i18n.tr("Maps"), ResourceManager.getImageDescriptor("icons/objecttree-maps.png"), SubtreeType.MAPS));

      addMainView(new ObjectOverviewView());
      addMainView(new InterfacesView());
      addMainView(new EntityMIBView());
      addMainView(new PerformanceView());
   }
}
