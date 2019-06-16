package org.netxms.ui.eclipse.agentmanager.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.commands.ActionHandler;
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
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.UserAgentNotification;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.views.helpers.UserAgentNotificationComparator;
import org.netxms.ui.eclipse.agentmanager.views.helpers.UserAgentNotificationFilter;
import org.netxms.ui.eclipse.agentmanager.views.helpers.UserAgentNotificationLabelProvider;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

public class UserAgentNotificationView extends ViewPart implements SessionListener
{
   public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.UserAgentNotificationView";
   
   public static final int COL_ID = 0;
   public static final int COL_OBJECTS = 1;
   public static final int COL_MESSAGE = 2;
   public static final int COL_IS_RECALLED = 3;
   public static final int COL_START_TIME = 4;
   public static final int COL_END_TIME = 5;
   
   private SortableTableViewer viewer; 
   private UserAgentNotificationFilter filter;
   private boolean initShowfilter = true;
   private FilterText filterText;
   private Action actionShowFilter;
   private Action actionRefresh;
   private Action actionRecall;
   private NXCSession session;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      parent.setLayout(new FormLayout());
      session = ConsoleSharedData.getSession();
      
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
            actionShowFilter.setChecked(false);
         }
      });
      
      final String[] names = { "ID", "Objects", "Message", "Is recalled", "Start time", "End time" };
      final int[] widths = { 80, 300, 300, 80, 100, 100 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new UserAgentNotificationLabelProvider());
      viewer.setComparator(new UserAgentNotificationComparator());
      filter = new UserAgentNotificationFilter();
      viewer.addFilter(filter);
      
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      initShowfilter = settings.getBoolean(ID + "initShowFilter");
      
      WidgetHelper.restoreTableViewerSettings(viewer, settings, "UserAgentNotification");
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, settings, "UserAgentNotification");
            settings.put(ID + "initShowFilter", initShowfilter);
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
      contributeToActionBars();
      createPopupMenu();
      activateContext();

      // Set initial focus to filter input line
      if (initShowfilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      session.addListener(this);
      
      refresh();
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(this);
      super.dispose();
   }
   
   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.agentmanager.context.UserAgentNotificationView"); 
      }
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };
      
      actionRecall = new Action("Recall notification") {
         @Override
         public void run()
         {
            recallMessage();
         }
      };
      
      actionShowFilter = new Action("&Show filter") {
         /*
          * (non-Javadoc)
          * 
          * @see org.eclipse.jface.action.Action#run()
          */
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(initShowfilter);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.agentmanager.commands.show_filter"); //$NON-NLS-1$
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
   }
   

   /**
    * Recall message 
    */
   private void recallMessage()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection.size() >= 1))
      {
         for(Object o : selection.toList())
         {            
            final UserAgentNotification msg = (UserAgentNotification)o;
            new ConsoleJob("Recall user agent notification", this, Activator.PLUGIN_ID, null) {
               @Override
               protected void runInternal(IProgressMonitor monitor) throws Exception
               {
                  session.userAgentNotificationRecall(msg.getId());
               }
               
               @Override
               protected String getErrorMessage()
               {
                  return "Cannot recall user agent notification";
               }
            }.start();
         }
      }
   }

   /**
    * Contribute actions to action bars
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pulldown menu
    * @param manager menu manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionRefresh);
      manager.add(actionShowFilter);
   }

   /**
    * Fill local toolbar
    * @param manager menu manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionRefresh);
      manager.add(actionShowFilter);
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);

      // Register menu for extension
      getSite().setSelectionProvider(viewer);
      getSite().registerContextMenu(menuMgr, viewer);
   }
   
   /**
    * Fill context menu
    * @param manager Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection.size() >= 1))
      {
         boolean recallIsActive = true;
         for(Object o : selection.toList())
         {
            if (((UserAgentNotification)o).isRecalled() || (((UserAgentNotification)o).getStartTime().getTime() == 0))
            {
               recallIsActive = false;
               break;
            }
         }
         if(recallIsActive)
            manager.add(actionRecall);
      }
   }
   
   /**
    * Refresh view
    */
   private void refresh()
   {
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Get list of user agent notifications", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<UserAgentNotification> messages = session.getUserAgentNotifications();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(messages);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of user agent notifications";
         }
      }.start();
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getTable().setFocus();
   }
   
   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      initShowfilter = enable;
      filterText.setVisible(initShowfilter);
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

	@Override
	public void notificationHandler(SessionNotification n) {
		switch(n.getCode())
		{
			case SessionNotification.USER_AGENT_MESSAGE_CHANGED:
				refresh();
				break;
		}		
	}
}
