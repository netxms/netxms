/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Configuration perspective
 */
public class ConfigurationPerspective extends Perspective
{
   private static final I18n i18n = LocalizationHelper.getI18n(ConfigurationPerspective.class);

   private FilterText filter;
   private TableViewer viewer;

   /**
    * The constructor.
    */
   public ConfigurationPerspective()
   {
      super("Configuration", i18n.tr("Configuration"), ResourceManager.getImage("icons/perspective-configuration.png"));
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = true;
      configuration.multiViewNavigationArea = false;
      configuration.multiViewMainArea = false;
      configuration.hasSupplementalArea = false;
      configuration.priority = 250;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#createNavigationArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createNavigationArea(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = 0;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      parent.setLayout(layout);

      filter = new FilterText(parent, SWT.NONE, null, false);
      filter.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      viewer = new TableViewer(parent, SWT.NONE);
      viewer.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      viewer.setContentProvider(new ArrayContentProvider());
   }
}
