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

import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.tools.ImageCache;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * UI perspective
 */
public abstract class Perspective
{
   private static Logger logger = LoggerFactory.getLogger(Perspective.class);

   private String id;
   private String name;
   private Image image;
   private PerspectiveConfiguration configuration = new PerspectiveConfiguration();
   private Window window;
   private Composite content;
   private SashForm verticalSplitter;
   private SashForm horizontalSpliter;
   private ViewStack navigationFolder;
   private ViewStack mainFolder;
   private ViewStack supplementaryFolder;
   private Composite navigationArea;
   private Composite headerArea;
   private ViewContainer mainArea;
   private Composite supplementalArea;
   private ISelectionProvider navigationSelectionProvider;
   private ISelectionChangedListener navigationSelectionListener;
   private ImageCache imageCache;

   /**
    * Create new perspective.
    *
    * @param id perspective ID
    * @param name perspective display name
    * @param image perspective image
    */
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
      logger.debug("Perspective \"" + name + "\" configuration: " + configuration);
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
      else if (mainArea != null)
         mainArea.setContext(selection.getFirstElement());
   }

   /**
    * Bind perspective to window.
    *
    * @param window owning window
    */
   public void bindToWindow(Window window)
   {
      this.window = window;
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
      {
         logger.debug("Creating content for perspective " + name);
         createWidgets(parent);
      }
      else
      {
         content.setVisible(true);
      }
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
            navigationFolder = new ViewStack(window, this, verticalSplitter, false, false);
            navigationFolder.addSelectionListener(new ViewStackSelectionListener() {
               @Override
               public void viewSelected(View view)
               {
                  logger.debug("New navigation view selected - " + view.getGlobalId());
                  setNavigationSelectionProvider(((view != null) && (view instanceof NavigationView)) ? ((NavigationView)view).getSelectionProvider() : null);
               }
            });
            navigationFolder.setAllViewsAaCloseable(configuration.allViewsAreCloseable);
         }
         else
         {
            navigationArea = new Composite(verticalSplitter, SWT.BORDER);
            navigationArea.setLayout(new FillLayout());
            createNavigationArea(navigationArea);
         }
      }
      if (configuration.hasSupplementalArea)
      {
         horizontalSpliter = new SashForm(configuration.hasNavigationArea ? verticalSplitter : content, SWT.VERTICAL);
      }

      Composite mainAreaHolder = new Composite(configuration.hasSupplementalArea ? horizontalSpliter : (configuration.hasNavigationArea ? verticalSplitter : content), SWT.NONE);
      if (configuration.hasHeaderArea)
      {
         GridLayout layout = new GridLayout();
         layout.marginHeight = 0;
         layout.marginWidth = 0;
         mainAreaHolder.setLayout(layout);

         headerArea = new Composite(mainAreaHolder, SWT.NONE);
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.verticalAlignment = SWT.CENTER;
         gd.grabExcessHorizontalSpace = true;
         headerArea.setLayoutData(gd);
         headerArea.setLayout(new FillLayout());

         createHeaderArea(headerArea);
      }
      else
      {
         mainAreaHolder.setLayout(new FillLayout());
      }

      if (configuration.multiViewMainArea)
      {
         mainFolder = new ViewStack(window, this, mainAreaHolder, configuration.enableViewExtraction, configuration.enableViewPinning);
         mainFolder.setAllViewsAaCloseable(configuration.allViewsAreCloseable);
         mainFolder.setUseGlobalViewId(configuration.useGlobalViewId);
         if (configuration.hasHeaderArea)
            mainFolder.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      }
      else
      {
         mainArea = new ViewContainer(window, this, mainAreaHolder, configuration.enableViewExtraction, configuration.enableViewPinning);
         if (configuration.hasHeaderArea)
            mainArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      }

      if (configuration.hasSupplementalArea)
      {
         if (configuration.multiViewSupplementalArea)
         {
            supplementaryFolder = new ViewStack(window, this, horizontalSpliter, configuration.enableViewExtraction, configuration.enableViewPinning);
            supplementaryFolder.setAllViewsAaCloseable(configuration.allViewsAreCloseable);
         }
         else
         {
            supplementalArea = new Composite(verticalSplitter, SWT.NONE);
            supplementalArea.setLayout(new FillLayout());
            createSupplementalArea(supplementalArea);
         }
      }

      if (verticalSplitter != null)
         verticalSplitter.setWeights(new int[] { 25, 75 });
      if (horizontalSpliter != null)
         horizontalSpliter.setWeights(new int[] { 80, 20 });

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
    * Create navigation area for single view perspectives
    *
    * @param parent
    */
   protected void createNavigationArea(Composite parent)
   {
   }

   /**
    * Create header area above main area
    *
    * @param parent parent composite for header area
    */
   protected void createHeaderArea(Composite parent)
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
    * Get owning window.
    *
    * @return owning window
    */
   public Window getWindow()
   {
      return window;
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
    * Add view to main folder or replace view for single-view perspectives.
    *
    * @param view view to add
    */
   public void addMainView(View view)
   {
      if (mainFolder != null)
         mainFolder.addView(view);
      else
         mainArea.setView(view);
   }

   /**
    * Add view to main folder or replace view for single-view perspectives.
    *
    * @param view view to add
    * @param activvate if true, view will be activated
    * @param ignoreContext set to true to ignore current context
    */
   public void addMainView(View view, boolean activate, boolean ignoreContext)
   {
      if (mainFolder != null)
         mainFolder.addView(view, activate, ignoreContext);
      else
         mainArea.setView(view);
   }

   /**
    * Set main view for single-view perspectives. Has no effect for multi-view perspectives.
    *
    * @param view new main view
    */
   public void setMainView(View view)
   {
      mainArea.setView(view);
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

   /**
    * Update trim (title, actions on toolbar, etc.) for given view.
    *
    * @param view view to update
    * @return true if view trim was updated
    */
   public boolean updateViewTrim(View view)
   {
      boolean updated = false;
      if (mainFolder != null)
         updated = mainFolder.updateViewTrim(view);
      if (!updated && (navigationFolder != null))
         updated = navigationFolder.updateViewTrim(view);
      if (!updated && (supplementaryFolder != null))
         updated = supplementaryFolder.updateViewTrim(view);
      return updated;
   }
}
