/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import java.lang.reflect.Constructor;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.fieldassist.IContentProposalProvider;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ISelectionProvider;
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
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.base.widgets.ProgressArea;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.keyboard.KeyBindingManager;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Base view class
 */
public abstract class View implements MessageAreaHolder
{
   protected static final int DEFAULT_PRIORITY = 65535;

   private static final Logger logger = LoggerFactory.getLogger(View.class);

   private String baseId;
   private String name;
   private ImageDescriptor imageDescriptor;
   private Image image;
   private ViewContainer viewContainer;
   private Composite viewArea;
   private MessageArea messageArea;
   private FilterText filterText;
   private Composite clientArea;
   private ProgressArea progressArea;
   private boolean hasFilter;
   private boolean showFilterTooltip;
   private boolean showFilterLabel;
   private boolean filterEnabled;
   private boolean refreshEnabled;
   private AbstractViewerFilter filter;
   private ISelectionProvider viewer;
   private Set<ViewStateListener> stateListeners = new HashSet<ViewStateListener>();
   private KeyBindingManager keyBindingManager = new KeyBindingManager();
   private View originalView;

   /**
    * Create new view with specific base ID. Actual view ID will be derived from base ID, possibly by classes derived from base view
    * class. This will not create actual widgets that composes view - creation can be delayed by framework until view actually has
    * to be shown for the first time.
    *
    * @param name view name
    * @param image view image
    * @param baseId view base ID
    */
   public View(String name, ImageDescriptor image, String baseId)
   {
      this(name, image, baseId, false, false, false);
   }

   /**
    * Create new view with specific base ID. Actual view ID will be derived from base ID, possibly by classes derived from base view
    * class. This will not create actual widgets that composes view - creation can be delayed by framework until view actually has
    * to be shown for the first time.
    *
    * @param name view name
    * @param image view image
    * @param baseId view base ID
    * @param hasFilter true if view should contain filter
    */
   public View(String name, ImageDescriptor image, String baseId, boolean hasFilter)
   {
      this(name, image, baseId, hasFilter, false, false);
   }

   /**
    * Create new view with specific base ID. Actual view ID will be derived from base ID, possibly by classes derived from base view
    * class. This will not create actual widgets that composes view - creation can be delayed by framework until view actually has
    * to be shown for the first time.
    *
    * @param name view name
    * @param image view image
    * @param baseId view base ID
    * @param hasFilter true if view should contain filter
    * @param showFilterTooltip true if view filter should have tooltip icon
    * @param showFilterLabel true if view filter should contain label on the left of text input field
    */
   public View(String name, ImageDescriptor image, String baseId, boolean hasFilter, boolean showFilterTooltip, boolean showFilterLabel)
   {
      this.baseId = baseId;
      this.name = name;
      this.imageDescriptor = image;
      this.hasFilter = hasFilter;
      this.showFilterTooltip = showFilterTooltip;
      this.showFilterLabel = showFilterLabel;
      this.refreshEnabled = true;
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
         Constructor<? extends View> constructor = getClass().getDeclaredConstructor();
         constructor.setAccessible(true);
         View view = constructor.newInstance();
         view.baseId = baseId;
         view.name = name;
         view.imageDescriptor = imageDescriptor;
         view.hasFilter = hasFilter;
         view.showFilterTooltip = showFilterTooltip;
         view.showFilterLabel = showFilterLabel;
         view.filterEnabled = filterEnabled;
         view.originalView = this;
         return view;
      }
      catch(Exception e)
      {
         logger.error("Cannot clone view", e);
         return null;
      }
   }

   /**
    * Default post clone option
    */
   protected void postClone(View view)
   {      
      final String[] filterText = new String[1];
      view.getDisplay().syncExec(new Runnable() {         
         @Override
         public void run()
         {
            filterText[0] = view.getFilterText();
         }
      });
      setFilterText(filterText[0]);
   }

   /**
    * Create view. Intended to be called only by framework.
    * 
    * @param window owning window
    * @param perspective owning perspective
    * @param parent parent composite
    * @param onFilterCloseCallback 
    */
   protected final void create(ViewContainer viewContainer, Composite parent, Runnable onFilterCloseCallback)
   {
      this.viewContainer = viewContainer;
      if (imageDescriptor != null)
         image = imageDescriptor.createImage();

      viewArea = new Composite(parent, SWT.NONE);
      viewArea.setLayout(new FormLayout());

      messageArea = new MessageArea(viewArea, SWT.NONE);
      if (hasFilter)
      {  
         filterText = new FilterText(viewArea, SWT.NONE, showFilterTooltip ? "" : null, true, showFilterLabel);
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

      progressArea = new ProgressArea(viewArea, SWT.NONE);

      createContent(clientArea);

      if (originalView != null)
      {
         postClone(originalView);
         originalView = null;
      }
      else
      {
         postContentCreate();
      }

      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      messageArea.setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.bottom = new FormAttachment(100, 0);
      fd.right = new FormAttachment(100, 0);
      progressArea.setLayoutData(fd);

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
         fd.bottom = new FormAttachment(progressArea);
         clientArea.setLayoutData(fd);

         filterEnabled = PreferenceStore.getInstance().getAsBoolean(getBaseId() + ".showFilter", true);
         if (!filterEnabled)
            enableFilter(false);
      }
      else
      {
         fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = new FormAttachment(messageArea);
         fd.right = new FormAttachment(100, 0);
         fd.bottom = new FormAttachment(progressArea);
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
   protected void fillLocalToolBar(IToolBarManager manager)
   {
   }

   /**
    * Fill view's local menu
    *
    * @param manager menu manager
    */
   protected void fillLocalMenu(IMenuManager manager)
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
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String, boolean, java.lang.String,
    *      java.lang.Runnable)
    */
   @Override
   public int addMessage(int level, String text, boolean sticky, String buttonText, Runnable action)
   {
      return messageArea.addMessage(level, text, sticky, buttonText, action);
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
    * Activate view. Called by framework when view is activated. Derived classes should always call parent class method when
    * overriding.
    */
   public void activate()
   {
      viewArea.moveAbove(null);
      viewArea.layout(true, true);

      // Use copy of listener set to avoid concurrent changes if listener will call View.removeStateListener()
      for(ViewStateListener listener : new ArrayList<ViewStateListener>(stateListeners))
         listener.viewActivated(this);
   }

   /**
    * Deactivate view. Called by framework when view is deactivated. Derived classes should always call parent class method when
    * overriding.
    */
   public void deactivate()
   {
      // Use copy of listener set to avoid concurrent changes if listener will call View.removeStateListener()
      for(ViewStateListener listener : new ArrayList<ViewStateListener>(stateListeners))
         listener.viewDeactivated(this);
   }

   /**
    * Handle set focus. Default implementation sets focus on filter text if present, otherwise on view area.
    */
   public void setFocus()
   {
      if (hasFilter && filterEnabled)
      {
         filterText.setFocus();
      }
      else if ((viewer != null) && (viewer instanceof StructuredViewer))
      {
         ((StructuredViewer)viewer).getControl().setFocus();
      }
      else
      {
         viewArea.setFocus();
      }
   }

   /**
    * Called by framework when view in folder or standalone window is about to be closed. View can cancel close operation by
    * returning false.
    *
    * @return true to allow close operation to continue
    */
   public boolean beforeClose()
   {
      return true;
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
      return (name != null) ? name : ("[" + baseId + "]");
   }

   /**
    * Get view's full name that can be used outside context.
    * 
    * @return view's full name
    */
   public String getFullName()
   {
      return getName();
   }

   /**
    * @param name
    */
   public void setName(String name)
   {
      this.name = name;
      if (viewContainer != null)
      {
         if (viewContainer.getPerspective() != null)
            viewContainer.getPerspective().updateViewTrim(this);
         else if (viewContainer instanceof ViewFolder)
            ((ViewFolder)viewContainer).updateViewTrim(this);
         else if (viewContainer.getWindow() != null)
            viewContainer.getWindow().getShell().setText(getFullName()); // Standalone view, update window title
      }
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
    * Set image for this view. Current image will be disposed.
    *
    * @param imageDescriptor descriptor for new image
    */
   public void setImage(ImageDescriptor imageDescriptor)
   {
      this.imageDescriptor = imageDescriptor;
      if (image != null)
         image.dispose();
      image = (imageDescriptor != null) ? imageDescriptor.createImage() : null;
      if (viewContainer != null)
      {
         if (viewContainer.getPerspective() != null)
            viewContainer.getPerspective().updateViewTrim(this);
         else if (viewContainer instanceof ViewFolder)
            ((ViewFolder)viewContainer).updateViewTrim(this);
      }
   }

   /**
    * Get view priority in stack. Views with lower priority value placed to the left of views with higher priority values. Default
    * implementation always returns 65535.
    * 
    * @return view priority
    */
   public int getPriority()
   {
      return DEFAULT_PRIORITY;
   }

   /**
    * Get owning window.
    * 
    * @return owning window
    */
   public Window getWindow()
   {
      return (viewContainer != null) ? viewContainer.getWindow() : null;
   }

   /**
    * Get owning perspective (can be null for standalone views).
    *
    * @return owning perspective or null
    */
   public Perspective getPerspective()
   {
      return (viewContainer != null) ? viewContainer.getPerspective() : null;
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
    * Get client area composite.
    *
    * @return client area composite
    */
   public Composite getClientArea()
   {
      return clientArea;
   }

   /**
    * Handle refresh request initiated via view stack or perspective. Default implementation does nothing.
    */
   public void refresh()
   {
   }

   /**
    * Enable/disable view refresh action.
    *
    * @param enabled true to enable
    */
   protected void enableRefresh(boolean enabled)
   {
      if (refreshEnabled != enabled)
      {
         refreshEnabled = enabled;
         if (viewContainer != null)
            viewContainer.updateRefreshActionState();
      }
   }

   /**
    * Check if refresh is enabled for this view.
    *
    * @return true if refresh is enabled for this view
    */
   public boolean isRefreshEnabled()
   {
      return refreshEnabled;
   }

   /**
    * Set visibility flag.
    * 
    * @param visible true if view should be visible
    */
   public void setVisible(boolean visible)
   {
      if ((viewArea != null) && !viewArea.isDisposed())
         viewArea.setVisible(visible);
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
      return viewContainer.isViewActive(this);
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
    * Set tooltip to be shown on filter's "information" icon (if present).
    *
    * @param tooltip new filter information tooltip text
    */
   public void setFilterTooltip(String tooltip)
   {
      filterText.setTooltip(tooltip);
   }

   /**
    * Add filter modify listener
    *
    * @param listener filter modify listener to add
    */
   public void addFilterModifyListener(ModifyListener listener)
   {
      if (filterText != null)
         filterText.addModifyListener(listener);
   }

   /**
    * Remove filter modify listener
    *
    * @param listener filter modify listener to remove
    */
   public void removeFilterModifyListener(ModifyListener listener)
   {
      if (filterText != null)
         filterText.removeModifyListener(listener);
   }

   /**
    * Handler for filter modification
    */
   protected void onFilterModify()
   {
      if ((filter != null) && (viewer != null))
         defaultOnFilterModify();
   }

   /**
    * Set viewer and filter that will be managed by default filter string handler
    * 
    * @param viewer viewer to refresh
    * @param filter filter to set filtering text
    */
   public void setFilterClient(ISelectionProvider viewer, AbstractViewerFilter filter)
   {
      this.viewer = viewer;
      this.filter = filter;
      if ((filterText != null) && !filterText.getText().isEmpty())
         defaultOnFilterModify();
   }

   /**
    * Default on modify for view with viewer
    */
   private void defaultOnFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      if (viewer instanceof StructuredViewer)
         ((StructuredViewer)viewer).refresh(false);
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
         filterText.setText("");
         onFilterModify();
      }
      PreferenceStore.getInstance().set(getBaseId() + ".showFilter", enable);
   }

   /**
    * Enable autocomplete for filter string.
    *
    * @param proposalProvider proposal provider
    */
   protected void enableFilterAutocomplete(IContentProposalProvider proposalProvider)
   {
      if (hasFilter)
         filterText.enableAutoComplete(proposalProvider);
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

   /**
    * Get current filter text.
    *
    * @return current filter text or null
    */
   public String getFilterText()
   {
      return (hasFilter && filterEnabled) ? filterText.getText() : null;
   }

   /**
    * Set filter text.
    *
    * @param text new filter text
    */
   public void setFilterText(String text)
   {
      if (hasFilter && filterEnabled)
      {
         filterText.setText(text);
         onFilterModify();
      }
   }

   /**
    * Get filter text control from this view
    *
    * @return filter text control from this view
    */
   public FilterText getFilterTextControl()
   {
      return filterText;
   }

   /**
    * Process keystroke.
    *
    * @param keyStroke keystroke to process
    */
   public void processKeyStroke(KeyStroke keyStroke)
   {
      keyBindingManager.processKeyStroke(keyStroke);
   }

   /**
    * Get key binding manager.
    *
    * @return key binding manager for this view
    */
   protected KeyBindingManager getKeyBindingManager()
   {
      return keyBindingManager;
   }

   /**
    * Add key binding.
    *
    * @param keyStroke keystroke to handle
    * @param action action to execute
    */
   public void addKeyBinding(KeyStroke keyStroke, IAction action)
   {
      keyBindingManager.addBinding(keyStroke, action, null);
   }

   /**
    * Add key binding.
    *
    * @param keyStroke keystroke to handle
    * @param action action to execute
    */
   public void addKeyBinding(String keyStroke, IAction action)
   {
      keyBindingManager.addBinding(keyStroke, action, null);
   }

   /**
    * Add key binding.
    *
    * @param modifiers modifier bits for keystroke to handle
    * @param key key code for keystroke to handle
    * @param action action to execute
    */
   public void addKeyBinding(int modifiers, int key, IAction action)
   {
      keyBindingManager.addBinding(modifiers, key, action, null);
   }

   /**
    * Add key binding.
    *
    * @param keyStroke keystroke to handle
    * @param action action to execute
    * @param radioGroup group of radio button actions this action belongs to
    */
   public void addKeyBinding(KeyStroke keyStroke, IAction action, String radioGroup)
   {
      keyBindingManager.addBinding(keyStroke, action, radioGroup);
   }

   /**
    * Add key binding.
    *
    * @param keyStroke keystroke to handle
    * @param action action to execute
    * @param radioGroup group of radio button actions this action belongs to
    */
   public void addKeyBinding(String keyStroke, IAction action, String radioGroup)
   {
      keyBindingManager.addBinding(keyStroke, action, radioGroup);
   }

   /**
    * Add key binding.
    *
    * @param modifiers modifier bits for keystroke to handle
    * @param key key code for keystroke to handle
    * @param action action to execute
    * @param radioGroup group of radio button actions this action belongs to
    */
   public void addKeyBinding(int modifiers, int key, IAction action, String radioGroup)
   {
      keyBindingManager.addBinding(modifiers, key, action, radioGroup);
   }

   /**
    * Check if view client area is disposed.
    *
    * @return true if view client area is disposed
    */
   public boolean isClientAreaDisposed()
   {
      return clientArea.isDisposed();
   }

   /**
    * Request framework to update view toolbar.
    */
   protected void updateToolBar()
   {
      if (viewContainer != null)
      {
         viewContainer.updateViewToolBar(this);
         viewContainer.layout(true, true);
      }
   }

   /**
    * Request framework to update view menu.
    */
   protected void updateMenu()
   {
      if (viewContainer != null)
      {
         if (viewContainer.updateViewMenu(this))
            viewContainer.layout(true, true);
      }
   }

   /**
    * Update both toolbar and menu in one operation
    */
   protected void updateControlBars()
   {
      if (viewContainer != null)
      {
         viewContainer.updateViewToolBar(this);
         viewContainer.updateViewMenu(this);
         viewContainer.layout(true, true);
      }
   }

   /**
    * Open new view in same perspective or in pop out window if this view is not in perspective.
    *
    * @param view view to open
    */
   public void openView(View view)
   {
      if (viewContainer == null)
         return; // This view is not attached to any container yet

      if (viewContainer.getPerspective() != null)
      {
         viewContainer.getPerspective().addMainView(view, true);
      }
      else if (viewContainer instanceof ViewFolder)
      {
         ((ViewFolder)viewContainer).addView(view, true, true);
      }
      else
      {
         PopOutViewWindow.open(view);
      }
   }

   /**
    * Save view state
    * 
    * @param memento memento to save state
    */
   public void saveState(Memento memento)
   {      
      String className = getClass().getName();
      memento.set("class", className);
      memento.set("baseId", baseId);
   }
   
   /**
    * Restore view state
    * 
    * @param memento memento to restore the state
    */
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      baseId = memento.getAsString("baseId");
   }

   /**
    * Create progress monitor for job.
    *
    * @param jobId ID if job to create monitor for
    * @return progress monitor
    */
   public IProgressMonitor createProgressMonitor(int jobId)
   {
      return (progressArea != null) ? progressArea.createMonitor(jobId) : null;
   }

   /**
    * Job completion handler. Removes progress monitor from progress area.
    *
    * @param jobId ID of completed job
    */
   public void onJobCompletion(int jobId)
   {
      if (progressArea != null)
         progressArea.removeMonitor(jobId);
   }
}
