package org.netxms.webui.core;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IContributionItem;
import org.eclipse.jface.action.ICoolBarManager;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarContributionItem;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.actions.ActionFactory;
import org.eclipse.ui.actions.ActionFactory.IWorkbenchAction;
import org.eclipse.ui.actions.ContributionItemFactory;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.netxms.base.NXCommon;
import org.netxms.ui.eclipse.shared.IActionConstants;

/**
 * Creates, adds and disposes actions for the menus and action bars of each
 * workbench window.
 */
public class ApplicationActionBarAdvisor extends ActionBarAdvisor
{
	private IWorkbenchAction actionExit;
	private Action actionAbout;
	private IWorkbenchAction actionShowPreferences;
	private IWorkbenchAction actionCustomizePerspective;
	private IWorkbenchAction actionSavePerspective;
	private IWorkbenchAction actionResetPerspective;
	private IWorkbenchAction actionClosePerspective;
	private IWorkbenchAction actionCloseAllPerspectives;
	private IWorkbenchAction actionMinimize;
	private IWorkbenchAction actionMaximize;
	private IWorkbenchAction actionPrevView;
	private IWorkbenchAction actionNextView;
	private IWorkbenchAction actionShowViewMenu;
	private Action actionOpenProgressView;
	private IContributionItem contribItemShowView;
	private IContributionItem contribItemOpenPerspective;

	/**
	 * @param configurer
	 */
	public ApplicationActionBarAdvisor(IActionBarConfigurer configurer)
	{
		super(configurer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.ActionBarAdvisor#makeActions(org.eclipse.ui.IWorkbenchWindow)
	 */
	@Override
	protected void makeActions(final IWorkbenchWindow window)
	{
		contribItemShowView = ContributionItemFactory.VIEWS_SHORTLIST.create(window);
		contribItemOpenPerspective = ContributionItemFactory.PERSPECTIVES_SHORTLIST.create(window);
		
		actionExit = ActionFactory.QUIT.create(window);
		register(actionExit);

		actionAbout = new Action("&About NetXMS Management Console") {
			private static final long serialVersionUID = 1L;
			
			@Override
			public void run()
			{
				MessageDialog.openInformation(window.getShell(), "About", "NetXMS Management Console (Web Edition)\nVersion " + NXCommon.VERSION + "\nCopyright (c) 2003-2012 Raden Solutions");
			}
		};
		
		actionShowPreferences = ActionFactory.PREFERENCES.create(window);
		register(actionShowPreferences);

		actionCustomizePerspective = ActionFactory.EDIT_ACTION_SETS.create(window);
		register(actionCustomizePerspective);
		
		actionSavePerspective = ActionFactory.SAVE_PERSPECTIVE.create(window);
		register(actionSavePerspective);
		
		actionResetPerspective = ActionFactory.RESET_PERSPECTIVE.create(window);
		register(actionResetPerspective);
		
		actionClosePerspective = ActionFactory.CLOSE_PERSPECTIVE.create(window);
		register(actionClosePerspective);
		
		actionCloseAllPerspectives = ActionFactory.CLOSE_ALL_PERSPECTIVES.create(window);
		register(actionCloseAllPerspectives);
		
		actionMinimize = ActionFactory.MINIMIZE.create(window);
		register(actionMinimize);
		
		actionMaximize = ActionFactory.MAXIMIZE.create(window);
		register(actionMaximize);
		
		actionPrevView = ActionFactory.PREVIOUS_PART.create(window);
		register(actionPrevView);
		
		actionNextView = ActionFactory.NEXT_PART.create(window);
		register(actionNextView);
		
		actionShowViewMenu = ActionFactory.SHOW_VIEW_MENU.create(window);
		register(actionShowViewMenu);

		actionOpenProgressView = new Action() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				IWorkbench wb = PlatformUI.getWorkbench();
				if (wb != null)
				{
					IWorkbenchWindow win = wb.getActiveWorkbenchWindow();
					if (win != null)
					{
						IWorkbenchPage page = win.getActivePage();
						if (page != null)
						{
							try
							{
								page.showView("org.eclipse.ui.views.ProgressView"); //$NON-NLS-1$
							}
							catch(PartInitException e)
							{
								e.printStackTrace();
							}
						}
					}
				}
			}
		};
		actionOpenProgressView.setText("Progress");
		actionOpenProgressView.setImageDescriptor(Activator.getImageDescriptor("icons/pview.gif")); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.ActionBarAdvisor#fillMenuBar(org.eclipse.jface.action.IMenuManager)
	 */
	@Override
	protected void fillMenuBar(IMenuManager menuBar)
	{
		MenuManager fileMenu = new MenuManager("&File", IWorkbenchActionConstants.M_FILE);
		MenuManager viewMenu = new MenuManager("&View", IActionConstants.M_VIEW);
		MenuManager monitorMenu = new MenuManager("&Monitor", IActionConstants.M_MONITOR);
		MenuManager configMenu = new MenuManager("&Configuration", IActionConstants.M_CONFIG);
		MenuManager toolsMenu = new MenuManager("&Tools", IActionConstants.M_TOOLS);
		MenuManager windowMenu = new MenuManager("&Window", IWorkbenchActionConstants.M_WINDOW);
		MenuManager helpMenu = new MenuManager("&Help", IWorkbenchActionConstants.M_HELP);

		menuBar.add(fileMenu);
		menuBar.add(viewMenu);
		menuBar.add(monitorMenu);
		menuBar.add(configMenu);
		menuBar.add(toolsMenu);

		// Add a group marker indicating where action set menus will appear.
		menuBar.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		
		menuBar.add(windowMenu);
		menuBar.add(helpMenu);
		
		// File
		fileMenu.add(actionShowPreferences);
		fileMenu.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		fileMenu.add(new Separator());
		fileMenu.add(actionExit);

		// View
		viewMenu.add(new GroupMarker(IActionConstants.M_PRODUCT_VIEW));
		viewMenu.add(new Separator());
		viewMenu.add(new GroupMarker(IActionConstants.M_PRIMARY_VIEW));
		viewMenu.add(new Separator());
		viewMenu.add(new GroupMarker(IActionConstants.M_LOGS_VIEW));
		viewMenu.add(new Separator());
		viewMenu.add(actionOpenProgressView);
		viewMenu.add(new GroupMarker(IActionConstants.M_TOOL_VIEW));
		
		// Monitor
		monitorMenu.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		
		// Tools
		toolsMenu.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		
		// Window
		MenuManager openPerspectiveMenuMgr = new MenuManager("Open Perspective", "openPerspective");
		openPerspectiveMenuMgr.add(contribItemOpenPerspective);
		windowMenu.add(openPerspectiveMenuMgr);
		
		final MenuManager showViewMenuMgr = new MenuManager("Show View", "showView");
		showViewMenuMgr.add(contribItemShowView);
		windowMenu.add(showViewMenuMgr);
		
		windowMenu.add(new Separator());
		windowMenu.add(actionCustomizePerspective);
		windowMenu.add(actionSavePerspective);
		windowMenu.add(actionResetPerspective);
		windowMenu.add(actionClosePerspective);
		windowMenu.add(actionCloseAllPerspectives);
		windowMenu.add(new Separator());
		
		final MenuManager navMenu = new MenuManager("Navigation", IWorkbenchActionConstants.M_NAVIGATE);
		windowMenu.add(navMenu);
		navMenu.add(actionShowViewMenu);
		navMenu.add(new Separator());
		navMenu.add(actionMaximize);
		navMenu.add(actionMinimize);
		navMenu.add(new Separator());
		navMenu.add(actionNextView);
		navMenu.add(actionPrevView);

		// Help
		helpMenu.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		helpMenu.add(actionAbout);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.ActionBarAdvisor#fillCoolBar(org.eclipse.jface.action.ICoolBarManager)
	 */
	@Override
	protected void fillCoolBar(ICoolBarManager coolBar)
	{
		IToolBarManager toolbar = new ToolBarManager(SWT.FLAT | SWT.TRAIL);
		toolbar.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		coolBar.add(new ToolBarContributionItem(toolbar, "product")); //$NON-NLS-1$

		toolbar = new ToolBarManager(SWT.FLAT | SWT.TRAIL);
		toolbar.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		coolBar.add(new ToolBarContributionItem(toolbar, "view")); //$NON-NLS-1$

		toolbar = new ToolBarManager(SWT.FLAT | SWT.TRAIL);
		toolbar.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		coolBar.add(new ToolBarContributionItem(toolbar, "logs")); //$NON-NLS-1$

		toolbar = new ToolBarManager(SWT.FLAT | SWT.TRAIL);
		toolbar.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		coolBar.add(new ToolBarContributionItem(toolbar, "tools")); //$NON-NLS-1$

		toolbar = new ToolBarManager(SWT.FLAT | SWT.TRAIL);
		toolbar.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		coolBar.add(new ToolBarContributionItem(toolbar, "config")); //$NON-NLS-1$
	}
}
