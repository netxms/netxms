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
import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Base view class
 */
public abstract class View implements MessageAreaHolder
{
   private static final Logger logger = LoggerFactory.getLogger(View.class);

   private String baseId;
   private String name;
   private ImageDescriptor imageDescriptor;
   private Image image;
   private Window window;
   private Perspective perspective;
   private Composite viewArea;
   private MessageArea messageArea;
   protected FilterText filterText;
   private Composite clientArea;
   private boolean hasFilter;
   private boolean filterEnabled;
   private ViewerFilterInternal filter;
   private StructuredViewer viewer;
   private Set<ViewStateListener> stateListeners = new HashSet<ViewStateListener>();

   /**
    * Create new view with specific base ID. Actual view ID will be derived from base ID, possibly by classes derived from base view
    * class. This will not create actual widgets that composes view - creation can be delayed by framework until view actually has
    * to be shown for the first time.
    *
    * @param name view name
    * @param image view image
    * @param baseId view base ID
    * @param hasFileter true if view should contain filter
    */
   public View(String name, ImageDescriptor image, String baseId, boolean hasFilter)
   {
      this.baseId = baseId;
      this.name = name;
      this.imageDescriptor = image;
      this.hasFilter = hasFilter;
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
         View view = getClass().getDeclaredConstructor().newInstance();
         view.baseId = baseId;
         view.name = name;
         view.imageDescriptor = imageDescriptor;
         view.hasFilter = hasFilter;
         view.filterEnabled = filterEnabled;
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
    * @param onFilterCloseCallback 
    */
   public void create(Window window, Perspective perspective, Composite parent, Runnable onFilterCloseCallback)
   {
      this.window = window;
      this.perspective = perspective;
      if (imageDescriptor != null)
         image = imageDescriptor.createImage();

      viewArea = new Composite(parent, SWT.NONE);
      viewArea.setLayout(new FormLayout());

      messageArea = new MessageArea(viewArea, SWT.NONE);
      if (hasFilter)
      {  
         filterText = new FilterText(viewArea, SWT.NONE);
         filterText.addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e)
            {
               onFilterModify();
            }
         });               
         filterText.setCloseCallback(onFilterCloseCallback);
      }

      clientArea = new Composite(viewArea, SWT.NONE);
      clientArea.setLayout(new FillLayout());
      createContent(clientArea);
      postContentCreate();

      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      messageArea.setLayoutData(fd);

      if (hasFilter)
      {
         fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = new FormAttachment(messageArea);
         fd.right = new FormAttachment(100, 0);
         filterText.setLayoutData(fd);

         fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = new FormAttachment(filterText);
         fd.right = new FormAttachment(100, 0);
         fd.bottom = new FormAttachment(100, 0);
         clientArea.setLayoutData(fd);

         filterEnabled = PreferenceStore.getInstance().getAsBoolean(getBaseId() + ".showFilter", true);
         if (filterEnabled)
            filterText.setFocus();
         else
            enableFilter(false);
      }
      else
      {
         fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = new FormAttachment(messageArea);
         fd.right = new FormAttachment(100, 0);
         fd.bottom = new FormAttachment(100, 0);
         clientArea.setLayoutData(fd);
      }
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
    * Fill view's local toolbar
    *
    * @param manager toolbar manager
    */
   protected void fillLocalToolbar(ToolBarManager manager)
   {
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String)
    */
   @Override
   public int addMessage(int level, String text)
   {
      return messageArea.addMessage(level, text);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String, boolean)
    */
   @Override
   public int addMessage(int level, String text, boolean sticky)
   {
      return messageArea.addMessage(level, text, sticky);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#deleteMessage(int)
    */
   @Override
   public void deleteMessage(int id)
   {
      messageArea.deleteMessage(id);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#clearMessages()
    */
   @Override
   public void clearMessages()
   {
      messageArea.clearMessages();
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
      logger.debug("View disposed - " + getGlobalId());

      // Use copy of listener set to avoid concurrent changes if listener will call View.removeStateListener()
      for(ViewStateListener listener : new ArrayList<ViewStateListener>(stateListeners))
         listener.viewClosed(this);
      stateListeners.clear();

      if ((viewArea != null) && !viewArea.isDisposed())
      {
         viewArea.dispose();
         viewArea = null;
      }
      if (image != null)
      {
         image.dispose();
         image = null;
      }
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
    * Get view's full name that can be used outside context.
    * 
    * @return view's full name
    */
   public String getFullName()
   {
      return name;
   }

   /**
    * @param name
    */
   public void setName(String name)
   {
      this.name = name;
      if (perspective != null)
         perspective.updateViewTrim(this);
      else if (window != null)
         window.getShell().setText(getFullName()); // Standalone view, update window title
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
    * Get view priority in stack. Views with lower priority value placed to the left of views with higher priority values. Default
    * implementation always returns 65535.
    * 
    * @return view priority
    */
   public int getPriority()
   {
      return 65535;
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
    * Get owning perspective (can be null for standalone views).
    *
    * @return owning perspective or null
    */
   public Perspective getPerspective()
   {
      return perspective;
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
    * Handle refresh request initiated via view stack or perspective. Default implementation does nothing.
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
    * Check if this view is currently visible and inside active tab within owning stack.
    * 
    * @return true if active
    */
   public boolean isActive()
   {
      return isVisible(); // FIXME: check owning stack
   }

   /**
    * Get view global ID. Default implementation returns base ID.
    *
    * @return view global ID
    */
   public String getGlobalId()
   {
      return baseId;
   }

   /**
    * Get view base ID.
    *
    * @return view base ID
    */
   public String getBaseId()
   {
      return baseId;
   }

   /**
    * Check if this view can be closed by user.
    *
    * @return true if this view can be closed by user
    */
   public boolean isCloseable()
   {
      return false;
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

   /**
    * Handler for filter modification
    */
   protected void onFilterModify()
   {
      if (filter != null && viewer != null)
         defaultOnFilterModify();
   }
   
   /**
    * Set viewer and filter to use default onFilterModify method
    * 
    * @param viewer viewer to refresh
    * @param filter filter to set filtering text
    */
   protected void setViewerAndFilter(StructuredViewer viewer, ViewerFilterInternal filter)
   {
      this.viewer = viewer;
      this.filter = filter;
   }

   /**
    * Default on modify for view with viewer
    */
   private void defaultOnFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   protected void enableFilter(boolean enable)
   {
      if (!hasFilter)
         return;

      filterEnabled = enable;
      filterText.setVisible(enable);
      FormData fd = (FormData)clientArea.getLayoutData();
      fd.top = new FormAttachment(enable ? filterText : messageArea);
      viewArea.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
      PreferenceStore.getInstance().set(getBaseId() + ".showFilter", enable);
   }

   /**
    * @return the hasFilter
    */
   public boolean hasFilter()
   {
      return hasFilter;
   }

   /**
    * @return the filterEnabled
    */
   public boolean isFilterEnabled()
   {
      return filterEnabled;
   }
}
