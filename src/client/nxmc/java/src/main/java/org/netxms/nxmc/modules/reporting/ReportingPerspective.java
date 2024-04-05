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
package org.netxms.nxmc.modules.reporting;

import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.reporting.views.ReportNavigationView;
import org.netxms.nxmc.modules.reporting.views.ReportView;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Reporting perspective
 */
public class ReportingPerspective extends Perspective
{
   /**
    * Create reporting perspective
    */
   public ReportingPerspective()
   {
      super("reporting", LocalizationHelper.getI18n(ReportingPerspective.class).tr("Reporting"), ResourceManager.getImage("icons/perspective-reporting.png"));
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
      configuration.multiViewNavigationArea = false;
      configuration.multiViewMainArea = false;
      configuration.priority = 60;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      setNavigationView(new ReportNavigationView());
      setMainView(new ReportView());
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#navigationSelectionChanged(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   protected void navigationSelectionChanged(IStructuredSelection selection)
   {
      while(!(getMainView() instanceof ReportView))
         popMainView();
      super.navigationSelectionChanged(selection);
   }
}
