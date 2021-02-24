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

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.ServiceLoader;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.widgets.FilterText;
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
   private ImageCache imageCache;
   private FilterText filter;
   private TableViewer viewer;
   private Composite mainViewArea;
   private ConfigurationPerspectiveElement currentElement;
   private ConfigurationView currentView;

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
    * @see org.netxms.nxmc.base.views.Perspective#createMainArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createMainArea(Composite parent)
   {
      mainViewArea = new Composite(parent, SWT.BORDER);
      mainViewArea.setLayout(new FillLayout());
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#createNavigationArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createNavigationArea(Composite parent)
   {
      imageCache = new ImageCache(parent);

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
      viewer.setLabelProvider(new LabelProvider() {
         /**
          * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
          */
         @Override
         public Image getImage(Object element)
         {
            return imageCache.create(((ConfigurationPerspectiveElement)element).getImage());
         }

         /**
          * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
          */
         @Override
         public String getText(Object element)
         {
            return ((ConfigurationPerspectiveElement)element).getName();
         }
      });
      viewer.setInput(elements);

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            changeView();
         }
      });
   }

   private void changeView()
   {
      if (currentView != null)
      {
         currentView.dispose();
      }

      currentElement = (ConfigurationPerspectiveElement)viewer.getStructuredSelection().getFirstElement();
      if (currentElement == null)
      {
         currentView = null;
         return;
      }

      currentView = currentElement.createView();
      currentView.create(getWindow(), this, mainViewArea);
      mainViewArea.layout(true, true);
   }
}
