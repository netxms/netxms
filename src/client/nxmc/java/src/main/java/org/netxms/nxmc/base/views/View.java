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

import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Base view class
 */
public abstract class View
{
   private static final Logger logger = LoggerFactory.getLogger(View.class);

   private String id;
   private String name;
   private ImageDescriptor imageDescriptor;
   private Image image;
   private Window window;
   private Perspective perspective;
   private Composite viewArea;
   private Set<ViewStateListener> stateListeners = new HashSet<ViewStateListener>();

   /**
    * Create new view with random ID. This will not create actual widgets that composes view - creation can be delayed by framework
    * until view actually has to be shown for the first time.
    *
    * @param name view name
    * @param image view image
    */
   public View(String name, ImageDescriptor image)
   {
      id = UUID.randomUUID().toString();
      this.name = name;
      this.imageDescriptor = image;
   }

   /**
    * Create new view with specific ID. This will not create actual widgets that composes view - creation can be delayed by
    * framework until view actually has to be shown for the first time.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    */
   public View(String name, ImageDescriptor image, String id)
   {
      this.id = id;
      this.name = name;
      this.imageDescriptor = image;
   }

   /**
    * Default constructor
    */
   protected View()
   {
   }

   /**
    * Clone this view. Returns new object of same class as this view, with ID, name, and image descriptor copied from this view.
    * Subclasses may override to copy additional information into new view.
    *
    * @return cloned view object or null if cloning operation fails
    */
   public View cloneView()
   {
      try
      {
         View view = (View)getClass().newInstance();
         view.id = id;
         view.name = name;
         view.imageDescriptor = imageDescriptor;
         return view;
      }
      catch(Exception e)
      {
         logger.error("Cannot clone view", e);
         return null;
      }
   }

   /**
    * Create view. Intended to be called only by framework.
    * 
    * @param window owning window
    * @param perspective owning perspective
    * @param parent parent composite
    */
   public void create(Window window, Perspective perspective, Composite parent)
   {
      this.window = window;
      this.perspective = perspective;
      if (imageDescriptor != null)
         image = imageDescriptor.createImage();
      viewArea = new Composite(parent, SWT.NONE);
      viewArea.setLayout(new FillLayout());
      createContent(viewArea);
      postContentCreate();
   }

   /**
    * Create view content
    *
    * @param parent parent control
    */
   protected abstract void createContent(Composite parent);

   /**
    * Called after view content was created. Default implementation does nothing. Can be used by subclasses to run initial refresh.
    */
   protected void postContentCreate()
   {
   }

   /**
    * Activate view. Called by framework when view is activated.
    */
   public void activate()
   {
      viewArea.moveAbove(null);

      // sometimes recursive activation can happen when tab selection changed - it's OK here
      try
      {
         viewArea.setFocus();
      }
      catch(RuntimeException e)
      {
      }
   }

   /**
    * Dispose view 
    */
   public void dispose()
   {
      for(ViewStateListener listener : stateListeners)
         listener.viewClosed(this);
      if ((viewArea != null) && !viewArea.isDisposed())
         viewArea.dispose();
      if (image != null)
         image.dispose();
   }

   /**
    * Check if view content is already created.
    * 
    * @return true if view content is already created
    */
   public boolean isCreated()
   {
      return viewArea != null;
   }

   /**
    * Get view's name
    * 
    * @return view's name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get view's image
    * 
    * @return view's image
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
    * Get main composite that contains all view widgets
    *
    * @return main composite that contains all view widgets
    */
   public Composite getViewArea()
   {
      return viewArea;
   }

   /**
    * Set visibility flag.
    * 
    * @param visible true if view should be visible
    */
   public void setVisible(boolean visible)
   {
      if ((viewArea != null) && !viewArea.isDisposed())
      {
         viewArea.setVisible(visible);
         if (visible)
            for(ViewStateListener listener : stateListeners)
               listener.viewActivated(this);
         else
            for(ViewStateListener listener : stateListeners)
               listener.viewDeactivated(this);
      }
   }

   /**
    * Refresh view. Called by framework when user press "Refresh" button in view toolbar. Default implementation does nothing.
    */
   public void refresh()
   {
   }

   /**
    * Check if this view is currently visible.
    * 
    * @return true if visible
    */
   public boolean isVisible()
   {
      return ((viewArea != null) && !viewArea.isDisposed()) ? viewArea.isVisible() : false;
   }

   /**
    * Get view unique ID.
    *
    * @return view unique ID
    */
   public String getId()
   {
      return id;
   }

   /**
    * Get display this view relates to.
    *
    * @return display object
    */
   public Display getDisplay()
   {
      return (viewArea != null) ? viewArea.getDisplay() : Display.getCurrent();
   }

   /**
    * Add state listener.
    *
    * @param listener state listener to add
    */
   public void addStateListener(ViewStateListener listener)
   {
      stateListeners.add(listener);
   }

   /**
    * Remove state listener.
    *
    * @param listener state listener to remove
    */
   public void removeStateListener(ViewStateListener listener)
   {
      stateListeners.remove(listener);
   }
}
