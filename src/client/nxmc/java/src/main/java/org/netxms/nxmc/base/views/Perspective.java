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

import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.tools.ImageCache;

/**
 * UI perspective
 */
public abstract class Perspective
{
   private String id;
   private String name;
   private Image image;
   private PerspectiveConfiguration configuration = new PerspectiveConfiguration();
   private Composite content;
   private SashForm verticalSplitter;
   private SashForm horizontalSpliter;
   private ViewStack navigationFolder;
   private ViewStack mainFolder;
   private ViewStack supplementaryFolder;
   private ISelectionProvider navigationSelectionProvider;
   private ISelectionChangedListener navigationSelectionListener;
   private ImageCache imageCache;

   protected Perspective(String id, String name, Image image)
   {
      this.id = id;
      this.name = name;
      this.image = image;

      navigationSelectionListener = new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            navigationSelectionChanged(event.getStructuredSelection());
         }
      };

      configurePerspective(configuration);
   }

   /**
    * Called by constructor to allow modification of default configuration.
    */
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
   }

   /**
    * Called when navigation selection is changed. Default implementation changes context for main view stack.
    *
    * @param selection new selection
    */
   protected void navigationSelectionChanged(IStructuredSelection selection)
   {
      if (mainFolder != null)
         mainFolder.setContext(selection.getFirstElement());
   }

   /**
    * Hide perspective
    */
   public void hide()
   {
      if ((content != null) && !content.isDisposed())
         content.setVisible(false);
   }

   /**
    * Show perspective. Will create perspective's widgets if this was not done already.
    *
    * @param parent perspective area
    */
   public void show(Composite parent)
   {
      if ((content == null) || content.isDisposed())
         createWidgets(parent);
      else
         content.setVisible(true);
   }

   /**
    * Check if this perspective is visible.
    *
    * @return true if this perspective is visible
    */
   public boolean isVisible()
   {
      return (content != null) && !content.isDisposed() && content.isVisible();
   }

   /**
    * Create perspective's widgets
    *
    * @param parent parent area
    */
   public void createWidgets(Composite parent)
   {
      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FillLayout());
      imageCache = new ImageCache(content);

      if (configuration.hasNavigationArea)
      {
         verticalSplitter = new SashForm(content, SWT.HORIZONTAL);
         if (configuration.multiViewNavigationArea)
         {
            navigationFolder = new ViewStack(null, this, verticalSplitter, false);
            navigationFolder.addSelectionListener(new ViewStackSelectionListener() {
               @Override
               public void viewSelected(View view)
               {
                  setNavigationSelectionProvider(((view != null) && (view instanceof NavigationView)) ? ((NavigationView)view).getSelectionProvider() : null);
               }
            });
         }
         else
         {
            createNavigationArea(verticalSplitter);
         }
      }
      if (configuration.hasSupplementalArea)
      {
         horizontalSpliter = new SashForm(configuration.hasNavigationArea ? verticalSplitter : content, SWT.VERTICAL);
      }
      if (configuration.multiViewMainArea)
      {
         mainFolder = new ViewStack(null, this, configuration.hasSupplementalArea ? horizontalSpliter
               : (configuration.hasNavigationArea ? verticalSplitter : content), true);
      }
      else
      {
         createMainArea(configuration.hasSupplementalArea ? horizontalSpliter : (configuration.hasNavigationArea ? verticalSplitter : content));
      }
      if (configuration.hasSupplementalArea)
      {
         if (configuration.multiViewSupplementalArea)
            supplementaryFolder = new ViewStack(null, this, horizontalSpliter, true);
         else
            createSupplementalArea(horizontalSpliter);
      }

      if (verticalSplitter != null)
         verticalSplitter.setWeights(new int[] { 3, 7 });
      if (horizontalSpliter != null)
         horizontalSpliter.setWeights(new int[] { 8, 2 });

      configureViews();

      // Set initially selected navigation view as selection provider
      if (navigationFolder != null)
      {
         View selectedView = navigationFolder.getSelection();
         if ((selectedView != null) && (selectedView instanceof NavigationView))
            setNavigationSelectionProvider(((NavigationView)selectedView).getSelectionProvider());
      }
   }

   /**
    * Dispose all widgets within perspective
    */
   public void disposeWidgets()
   {
      if (!content.isDisposed())
         content.dispose();
   }

   /**
    * Create main area for single view perspectives
    *
    * @param parent
    */
   protected void createMainArea(Composite parent)
   {
   }

   /**
    * Create navigation area for single view perspectives
    *
    * @param parent
    */
   protected void createNavigationArea(Composite parent)
   {
   }

   /**
    * Create supplemental area for single view perspectives
    *
    * @param parent
    */
   protected void createSupplementalArea(Composite parent)
   {
   }

   /**
    * Called during perspective creation after all widgets and view stacks are created and intended for configuring views inside
    * perspective. Default implementation does nothing.
    */
   protected void configureViews()
   {
   }

   /**
    * Get this perspective's priority
    *
    * @return this perspective's priority
    */
   public int getPriority()
   {
      return configuration.priority;
   }

   /**
    * Get perspective ID
    * 
    * @return perspective ID
    */
   public String getId()
   {
      return id;
   }

   /**
    * Get perspective name
    * 
    * @return perspective name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get perspective image
    * 
    * @return perspective image
    */
   public Image getImage()
   {
      return image;
   }

   /**
    * Get image cache for this perspective.
    *
    * @return image cache for this perspective
    */
   protected ImageCache getImageCache()
   {
      return imageCache;
   }

   /**
    * @return the navigationSelectionProvider
    */
   protected ISelectionProvider getNavigationSelectionProvider()
   {
      return navigationSelectionProvider;
   }

   /**
    * @param navigationSelectionProvider the navigationSelectionProvider to set
    */
   protected void setNavigationSelectionProvider(ISelectionProvider navigationSelectionProvider)
   {
      if (this.navigationSelectionProvider != null)
         this.navigationSelectionProvider.removeSelectionChangedListener(navigationSelectionListener);
      this.navigationSelectionProvider = navigationSelectionProvider;
      if (navigationSelectionProvider != null)
         navigationSelectionProvider.addSelectionChangedListener(navigationSelectionListener);
   }

   /**
    * Add view to main folder.
    *
    * @param view view to add
    */
   public void addMainView(View view)
   {
      if (mainFolder != null)
         mainFolder.addView(view);
   }

   /**
    * Add view to main folder.
    *
    * @param view view to add
    * @param ignoreContext set to true to ignore current context
    */
   public void addMainView(View view, boolean ignoreContext)
   {
      if (mainFolder != null)
         mainFolder.addView(view, ignoreContext);
   }

   /**
    * Remove view from main folder.
    * 
    * @param id ID of view to remove
    */
   public void removeMainView(String id)
   {
      if (mainFolder != null)
         mainFolder.removeView(id);
   }

   /**
    * Add view to navigation folder.
    *
    * @param view view to add
    */
   public void addNavigationView(NavigationView view)
   {
      if (navigationFolder != null)
         navigationFolder.addView(view);
   }

   /**
    * Remove view from navigation folder.
    * 
    * @param id ID of view to remove
    */
   public void removeNavigationView(String id)
   {
      if (navigationFolder != null)
         navigationFolder.removeView(id);
   }

   /**
    * Add view to supplementary folder.
    *
    * @param view view to add
    */
   public void addSupplementaryView(View view)
   {
      if (supplementaryFolder != null)
         supplementaryFolder.addView(view);
   }

   /**
    * Remove view from supplementary folder.
    * 
    * @param id ID of view to remove
    */
   public void removeSupplementaryView(String id)
   {
      if (supplementaryFolder != null)
         supplementaryFolder.removeView(id);
   }
}
