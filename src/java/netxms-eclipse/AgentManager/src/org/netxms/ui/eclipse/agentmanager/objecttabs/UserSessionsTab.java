/**
 * NetXMS - open source network management system
 * Copyright (C) 2019 Raden Solutions
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
package org.netxms.ui.eclipse.agentmanager.objecttabs;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.Messages;
import org.netxms.ui.eclipse.agentmanager.objecttabs.helpers.AgentSessionFilter;
import org.netxms.ui.eclipse.agentmanager.objecttabs.helpers.UserSessionComparator;
import org.netxms.ui.eclipse.agentmanager.objecttabs.helpers.UserSessionLabelProvider;
import org.netxms.ui.eclipse.agentmanager.views.ScreenshotView;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.ViewRefreshController;
import org.netxms.ui.eclipse.tools.VisibilityValidator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * User session tab tab
 */
public class UserSessionsTab extends ObjectTab
{
   public static final String[] COLUMNS = { "SESSION_ID", "SESSION_NAME", "USER_NAME", "CLIENT_NAME", "STATE", "AGENT_TYPE" };
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_SESSION = 1;
   public static final int COLUMN_USER = 2;
   public static final int COLUMN_CLIENT = 3;
   public static final int COLUMN_STATE = 4;
   public static final int COLUMN_AGENT_TYPE = 5;

   private NXCSession session;
   private AbstractObject object;
   private Table sessionsTable = null;
   private boolean initShowFilter = true;
   private SortableTableViewer viewer;
   private FilterText filterText;
   private AgentSessionFilter filter;
   private ViewRefreshController refreshController;
   private Action actionExportToCsv;
   private Action actionTakeScreenshot;

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createTabContent(Composite parent)
   {
      parent.setLayout(new FormLayout());
      session = ConsoleSharedData.getSession();
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      initShowFilter = safeCast(settings.get("UserSessionsTab.showFilter"), settings.getBoolean("UserSessionsTab.showFilter"), initShowFilter);

      // Create filter area
      filterText = new FilterText(parent, SWT.NONE, null, true);
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
         }
      });

      final String[] names = { "Session id", "Session", "User", "Client", "State", "Agent type" };
      final int[] widths = { 150, 150, 100, 150, 150, 100 };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setLabelProvider(new UserSessionLabelProvider(this));
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new UserSessionComparator(this));
      viewer.getTable().setHeaderVisible(true);
      viewer.getTable().setLinesVisible(true);
      filter = new AgentSessionFilter();
      viewer.addFilter(filter);
      WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "NodeTable.V2"); //$NON-NLS-1$
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "NodeTable.V2"); //$NON-NLS-1$
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

      VisibilityValidator validator = new VisibilityValidator() { // $NON-NLS-1$
         @Override
         public boolean isVisible()
         {
            return isActive();
         }
      };

      refreshController = new ViewRefreshController(getViewPart(), -1, new Runnable() {
         @Override
         public void run()
         {
            if (viewer.getTable().isDisposed())
               return;

            refresh();
         }
      }, validator);
      refreshController.setInterval(30);

      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            settings.put("UserSessionsTab.showFilter", initShowFilter);
            refreshController.dispose();
         }
      });

      // Set initial focus to filter input line
      if (initShowFilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      createActions();
      createPopupMenu();
   }

   /**
    * Create popup menu
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager(getViewPart().getSite().getId() + ".UserSessionsTab");
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {

         @Override
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getTable());
      viewer.getTable().setMenu(menu);

      // Register menu for extension.
      getViewPart().getSite().registerContextMenu(menuMgr, viewer);
   }

   /**
    * Fill context menu 
    * 
    * @param manager menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection.size() >= 1))
      {
         manager.add(actionExportToCsv);
         manager.add(new Separator());
         manager.add(actionTakeScreenshot);
      }
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionExportToCsv = new ExportToCsvAction(getViewPart(), viewer, true);
      actionTakeScreenshot = new Action("Take screenshot") {
         @Override
         public void run()
         {
            takeScreenshot();
         }
      };
   }

   /**
    * Take screenshot action implementation
    */
   private void takeScreenshot()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection.size() >= 1))
      {
         int colIndexName = sessionsTable.getColumnIndex("SESSION_NAME"); //$NON-NLS-1$
         int colIndexUser = sessionsTable.getColumnIndex("USER_NAME"); //$NON-NLS-1$
         for(Object o : selection.toList())
         {
            try
            {
               PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage()
                     .showView(ScreenshotView.ID, Long.toString(object.getObjectId()) + "&"
                           + ((TableRow)o).get(colIndexName).getValue() + "&" + ((TableRow)o).get(colIndexUser).getValue(),
                           IWorkbenchPage.VIEW_ACTIVATE);
            }
            catch(PartInitException e)
            {
               MessageDialogHelper.openError(getViewPart().getViewSite().getShell(), Messages.get().TakeScreenshot_Error,
                     String.format(Messages.get().TakeScreenshot_ErrorOpeningView, e.getLocalizedMessage()));
            }
         }
      }
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
      viewer.getTable().getParent().layout();
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
   private void setFilter(final String text)
   {
      filterText.setText(text);
      onFilterModify();
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

   /**
    * @param b
    * @param defval
    * @return
    */
   private static boolean safeCast(String s, boolean b, boolean defval)
   {
      return (s != null) ? b : defval;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public void objectChanged(AbstractObject object)
   {
      this.object = object;
      if (getViewPart().getSite().getPage().isPartVisible(getViewPart()) && isActive())
         refresh();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean showForObject(AbstractObject object)
   {
      return object instanceof Node && ((Node)object).hasAgent();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
   @Override
   public void refresh()
   {
      if (!isActive())
         return;
      
      new ConsoleJob("Get list of agent sessions", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            sessionsTable = session.queryAgentTable(object.getObjectId(), "Agent.SessionAgents");
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(sessionsTable.getAllRows());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of user agent sessions";
         }
      }.start();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      super.selected();
      refresh();
   }
   
   /**
    * @return displayed table
    */
   public Table getTable()
   {
      return sessionsTable;
   }
}
