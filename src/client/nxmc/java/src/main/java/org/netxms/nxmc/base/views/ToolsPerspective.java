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
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.services.ToolDescriptor;
import org.netxms.nxmc.tools.ImageCache;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Tools perspective
 */
public class ToolsPerspective extends Perspective
{
   private static final Logger logger = LoggerFactory.getLogger(ToolsPerspective.class);
   private static final I18n i18n = LocalizationHelper.getI18n(ToolsPerspective.class);

   private List<ToolDescriptor> elements = new ArrayList<ToolDescriptor>();
   private ToolDescriptor previousSelectedElement = null;
   private NavigationView navigationView;

   /**
    * The constructor.
    */
   public ToolsPerspective()
   {
      super("Tools", i18n.tr("Tools"), ResourceManager.getImage("icons/perspective-tools.png"));

      ServiceLoader<ToolDescriptor> loader = ServiceLoader.load(ToolDescriptor.class, getClass().getClassLoader());
      for(ToolDescriptor e : loader)
      {
         logger.debug("Adding tools element " + e.getName());
         elements.add(e);
      }
      elements.sort(new Comparator<ToolDescriptor>() {
         @Override
         public int compare(ToolDescriptor e1, ToolDescriptor e2)
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
      configuration.priority = 200;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      navigationView = new NavigationView(i18n.tr("Tools"), null, "Tools", true, false, false) {
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
                  return imageCache.create(((ToolDescriptor)element).getImage());
               }

               @Override
               public String getText(Object element)
               {
                  return ((ToolDescriptor)element).getName();
               }
            });
            viewer.addFilter(new ViewerFilter() {
               @Override
               public boolean select(Viewer viewer, Object parentElement, Object element)
               {
                  String filter = getFilterText();
                  return (filter == null) || filter.isEmpty() || ((ToolDescriptor)element).getName().toLowerCase().contains(filter.toLowerCase());
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

         @Override
         public void setSelection(Object selection)
         {
            if (selection != null)
            {
               viewer.setSelection(new StructuredSelection(selection), true);
            }
         }

         /**
          * @see org.netxms.nxmc.base.views.View#onFilterModify()
          */
         @Override
         protected void onFilterModify()
         {
            viewer.refresh();
         }
      };
      addNavigationView(navigationView);
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#navigationSelectionChanged(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   protected void navigationSelectionChanged(IStructuredSelection selection)
   {
      ToolDescriptor currentElement = (ToolDescriptor)selection.getFirstElement();
      
      if (previousSelectedElement == currentElement)
         return; //do nothing for reselection

      if (currentElement != null)
      {
         setMainView(currentElement.createView());
      }
      else
      {
         setMainView(null);
      }
      previousSelectedElement = currentElement;
   }
}
