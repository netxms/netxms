/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.ui.eclipse.objectview.widgets;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.PhysicalLink;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.dialogs.PhysicalLinkEditDialog;
import org.netxms.ui.eclipse.objectview.widgets.helpers.PhysicalLinkComparator;
import org.netxms.ui.eclipse.objectview.widgets.helpers.PhysicalLinkFilter;
import org.netxms.ui.eclipse.objectview.widgets.helpers.PhysicalLinkLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.VisibilityValidator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.MessageBar;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Widget to display physical links
 */
public class PhysicalLinkWidget extends  Composite implements SessionListener
{
   public static final String ID = "org.netxms.ui.eclipse.objectmanager.views.PhysicalLinkView"; //$NON-NLS-1$

   private static final String TABLE_CONFIG_PREFIX = "PhysicalLinkView"; //$NON-NLS-1$

   public static final int PHYSICAL_LINK_ID = 0;
   public static final int DESCRIPTOIN = 1;
   public static final int LEFT_OBJECT = 2;
   public static final int LEFT_PORT = 3;
   public static final int RIGHT_OBJECT = 4;
   public static final int RIGHT_PORT = 5;

   private ViewPart viewPart;
   private NXCSession session;
   private SortableTableViewer viewer;
   private Action actionRefresh;
   private Action actionAdd;
   private Action actionDelete;
   private Action actionEdit;
   private Action actionShowFilter;
   private List<PhysicalLink> linkList;
   private long objectId;
   private long patchPanelId;
   private VisibilityValidator visibilityValidator;
   private CompositeWithMessageBar messageBarComposite;
   private Composite content;

   private FilterText filterText;
   private PhysicalLinkFilter filter;
   private boolean initShowFilter = true;

   /**
    * Widget constructor 
    * 
    * @param viewPart view part 
    * @param parent parent composite
    * @param style style
    * @param objectId object this widget is opened on: node or rack id or 0 if show all
    * @param patchPanelId patch panel id form Rack with id provided in objectId or 0 if show all
    * @param enablefilter if filter should be enabled
    * @param visibilityValidator visibility validator to disable refresh for unactive tab
    */
   public PhysicalLinkWidget(ViewPart viewPart, Composite parent, int style, long objectId, long patchPanelId, boolean enablefilter, VisibilityValidator visibilityValidator)
   {
      super(parent, style);
      session = ConsoleSharedData.getSession();
      this.viewPart = viewPart;  
      this.objectId = objectId;
      this.patchPanelId = patchPanelId;
      initShowFilter = enablefilter;
      this.visibilityValidator = visibilityValidator;
      
      setLayout(new FillLayout());

      messageBarComposite = new CompositeWithMessageBar(this, SWT.NONE);

      content = new Composite(messageBarComposite, SWT.NONE);
      messageBarComposite.setContent(content);
      content.setLayout(new FormLayout());

      // Create filter
      filterText = new FilterText(content, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            actionShowFilter.setChecked(initShowFilter);
         }
      });

      final int[] widths = { 50, 200, 200, 400, 200, 400 };
      final String[] names = { "Id", "Description", "Object 1", "Port 1", "Object 2", "Port 2" };
      viewer = new SortableTableViewer(content, names, widths, PHYSICAL_LINK_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      PhysicalLinkLabelProvider labelPrivater = new PhysicalLinkLabelProvider();
      viewer.setLabelProvider(labelPrivater);
      viewer.setComparator(new PhysicalLinkComparator(labelPrivater));
      filter = new PhysicalLinkFilter(labelPrivater);
      viewer.addFilter(filter);

      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
            }
         }
      });
      
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      createActions();
      createPopupMenu();

      // Set initial focus to filter input line
      if (initShowFilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
      
      linkList = new ArrayList<PhysicalLink>();
      session.addListener(this);
      refresh();
   }

   /**
    * Sync physical links interface objects
    * 
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private void syncAdditionalObjects() throws IOException, NXCException
   {
      List<Long> additionalSyncInterfaces = new ArrayList<Long>();
      for(PhysicalLink link : linkList)
      {
         long id = link.getLeftObjectId();         
         if (id != 0)
            additionalSyncInterfaces.add(id);
         id = link.getRightObjectId();         
         if (id != 0)
            additionalSyncInterfaces.add(id);
      }

      if (!additionalSyncInterfaces.isEmpty())
         session.syncMissingObjects(additionalSyncInterfaces, NXCSession.OBJECT_SYNC_WAIT);
   }

   /**
    * Refresh physical link list
    */
   public void refresh()
   {
      if (objectId == -1)
         return;      

      if (visibilityValidator != null && !visibilityValidator.isVisible())
         return;
      
      new ConsoleJob("Load physical links", viewPart, Activator.PLUGIN_ID, null) {
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {            
            linkList = session.getPhysicalLinks(objectId, patchPanelId);
            syncAdditionalObjects();
            runInUIThread(new Runnable() {
               
               @Override
               public void run()
               {
                  viewer.setInput(linkList);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get physical link list";
         }         

         @Override
         protected IStatus createFailureStatus(final Exception e)
         {            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  messageBarComposite.showMessage(MessageBar.ERROR, String.format("%s (%s)", getErrorMessage(), e.getLocalizedMessage()));
               }
            });
            return Status.OK_STATUS;
         }
      }.start();
   }

   /**
    * Create popup menu
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);

      // Register menu for extension
      if (viewPart != null)
      {
         viewPart.getSite().setSelectionProvider(viewer);
         viewPart.getSite().registerContextMenu(menuMgr, viewer);
      }
   }

   /**
    * Fill context menu
    * @param mgr menu manager
    */
   protected void fillContextMenu(IMenuManager mgr)
   {
      mgr.add(actionEdit);
      mgr.add(actionDelete);
      mgr.add(actionAdd);
      mgr.add(actionRefresh);
   }

   /**
    * Contribute to action bar
    * Should be called manually where needed
    */
   public void contributeToActionBars()
   {
      IActionBars bars = viewPart.getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local tool bar
    * @param toolBarManager tool bar manager
    */
   private void fillLocalToolBar(IToolBarManager toolBarManager)
   {
      toolBarManager.add(actionAdd);
      toolBarManager.add(actionRefresh);
      toolBarManager.add(new Separator());
      toolBarManager.add(actionShowFilter);
   }

   /**
    * Fill pull down menu
    * @param menuManager menu manager
    */
   private void fillLocalPullDown(IMenuManager menuManager)
   {
      menuManager.add(actionAdd);
      menuManager.add(actionRefresh);
      menuManager.add(new Separator());
      menuManager.add(actionShowFilter);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)viewPart.getSite().getService(IHandlerService.class);
      
      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {
            refresh();
         }
      }; 
      
      actionAdd = new Action("&New...") {
         @Override
         public void run()
         {
            addPhysicalLink();
         }         
      };
      actionAdd.setImageDescriptor(SharedIcons.ADD_OBJECT);
      
      actionDelete = new Action("&Delete") {
         @Override
         public void run()
         {
            deletePhysicalLink();
         }         
      };
      actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);
      
      actionEdit = new Action("&Edit...") {
         @Override
         public void run()
         {
            editPhysicalLink();
         }         
      };
      actionEdit.setImageDescriptor(SharedIcons.EDIT);

      actionShowFilter = new Action("Show &filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(initShowFilter);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.objectview.commands.show_physical_link_filter"); //$NON-NLS-1$  
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
   }

   /**
    * Edit selected physical link
    */
   private void editPhysicalLink()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      PhysicalLink link = (PhysicalLink)selection.getFirstElement();
      PhysicalLinkEditDialog dlg = new PhysicalLinkEditDialog(getShell(), link);
      if (dlg.open() != Window.OK)
         return;

      final PhysicalLink editedLink = dlg.getLink();
      new ConsoleJob("Modify physical link", viewPart, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.updatePhysicalLink(editedLink);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot modify physical link";
         }
      }.start();
      
   }

   /**
    * Delete selected physical link
    */
   private void deletePhysicalLink()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final String message = (selection.size() == 1) ? "Do you really want to delete the selected physical link?" : "Do you really want to delete the selected physical links?";
      if (!MessageDialogHelper.openQuestion(getShell(), "Confirm Delete", message))
         return;

      final List<PhysicalLink> links = new ArrayList<PhysicalLink>(selection.size());
      for (Object o : selection.toList())
         links.add((PhysicalLink)o);
      new ConsoleJob("Delete physical link", viewPart, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(PhysicalLink link : links)
               session.deletePhysicalLink(link.getId());
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete physical link";
         }
      }.start();
   }

   /**
    * Add new physical link
    */
   private void addPhysicalLink()
   {  
      PhysicalLinkEditDialog dlg = new PhysicalLinkEditDialog(getShell(), null);
      if(dlg.open() != Window.OK)
         return;
         
      final PhysicalLink link = dlg.getLink();
      new ConsoleJob("Create new physical link", viewPart, Activator.PLUGIN_ID) {
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.updatePhysicalLink(link);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot create physical link";
         }
      }.start();
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      initShowFilter = enable;
      filterText.setVisible(initShowFilter);
      FormData fd = (FormData)viewer.getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      layout();
      if (enable)
         filterText.setFocus();
      else
         setFilter(""); //$NON-NLS-1$

   }

   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   public void setFilter(final String text)
   {
      filterText.setText(text);
      onFilterModify();
   }

   /**
    * Handler for filter modification
    */
   public void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

   /**
    * @see org.eclipse.swt.widgets.Widget#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(this);
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      settings.put("ObjectTools.showFilter", initShowFilter);
      super.dispose();
   }

   /**
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(SessionNotification n)
   {
      if (n.getCode() == SessionNotification.PHYSICAL_LINK_UPDATE)
      {
         viewer.getControl().getDisplay().asyncExec(new Runnable() {
            @Override
            public void run()
            {
               refresh();
            }
         });
      }
   }

   /**
    * Change source object
    * 
    * @param objectId
    * @param patchPanelId
    */
   public void setSourceObject(long objectId, long patchPanelId)
   {
      this.objectId = objectId;
      this.patchPanelId = patchPanelId;
      refresh();      
   }

   /**
    * Returns if filter is enabled
    * @return true if filter is enabled
    */
   public boolean isFilterEnabled()
   {
      return initShowFilter;
   }

   /**
    * Set action on filter close
    * 
    * @param action
    */
   public void setFilterCloseAction(Action action)
   {
      filterText.setCloseAction(action);      
   }
}
