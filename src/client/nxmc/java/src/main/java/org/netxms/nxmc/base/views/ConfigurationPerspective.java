/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
import java.util.Comparator;
import java.util.List;
import java.util.ServiceLoader;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.services.ConfigurationPerspectiveElement;
import org.netxms.nxmc.tools.ImageCache;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Configuration perspective
 */
public class ConfigurationPerspective extends Perspective
{
   private static final Logger logger = LoggerFactory.getLogger(ConfigurationPerspective.class);
   private static final I18n i18n = LocalizationHelper.getI18n(ConfigurationPerspective.class);

   private List<ConfigurationPerspectiveElement> elements = new ArrayList<ConfigurationPerspectiveElement>();

   /**
    * The constructor.
    */
   public ConfigurationPerspective()
   {
      super("Configuration", i18n.tr("Configuration"), ResourceManager.getImage("icons/perspective-configuration.png"));

      ServiceLoader<ConfigurationPerspectiveElement> loader = ServiceLoader.load(ConfigurationPerspectiveElement.class);
      for(ConfigurationPerspectiveElement e : loader)
      {
         logger.debug("Adding configuration element " + e.getName());
         elements.add(e);
      }
      elements.sort(new Comparator<ConfigurationPerspectiveElement>() {
         @Override
         public int compare(ConfigurationPerspectiveElement e1, ConfigurationPerspectiveElement e2)
         {
            return e1.getName().compareToIgnoreCase(e2.getName());
         }
      });
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
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      addNavigationView(new NavigationView("Configuration", null, "Configuration", true, false, false) {
         private ImageCache imageCache;
         private TableViewer viewer;

         @Override
         protected void createContent(Composite parent)
         {
            imageCache = new ImageCache(parent);

            viewer = new TableViewer(parent, SWT.NONE);
            viewer.setContentProvider(new ArrayContentProvider());
            viewer.setLabelProvider(new LabelProvider() {
               @Override
               public Image getImage(Object element)
               {
                  return imageCache.create(((ConfigurationPerspectiveElement)element).getImage());
               }

               @Override
               public String getText(Object element)
               {
                  return ((ConfigurationPerspectiveElement)element).getName();
               }
            });
            viewer.setInput(elements);
         }

         @Override
         public ISelectionProvider getSelectionProvider()
         {
            return viewer;
         }

         /**
          * @see org.netxms.nxmc.base.views.View#dispose()
          */
         @Override
         public void dispose()
         {
            imageCache.dispose();
            super.dispose();
         }
      });
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#navigationSelectionChanged(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   protected void navigationSelectionChanged(IStructuredSelection selection)
   {
      ConfigurationPerspectiveElement currentElement = (ConfigurationPerspectiveElement)selection.getFirstElement();
      if (currentElement != null)
      {
         setMainView(currentElement.createView());
      }
      else
      {
         setMainView(null);
      }
   }
}
