package org.netxms.ui.eclipse.console;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.ICoolBarManager;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IStatusLineManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.StatusLineContributionItem;
import org.eclipse.jface.action.ToolBarContributionItem;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.actions.ActionFactory;
import org.eclipse.ui.actions.ActionFactory.IWorkbenchAction;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.console.ConsolePlugin;
import org.eclipse.ui.console.IConsoleConstants;
import org.netxms.ui.eclipse.shared.IActionConstants;

public class NXMCActionBarAdvisor extends ActionBarAdvisor
{
	// Actions - important to allocate these only in makeActions, and then use them
	// in the fill methods. This ensures that the actions aren't recreated
	// when fillActionBars is called with FILL_PROXY.
	private IWorkbenchAction actionExit;
	private IWorkbenchAction actionAbout;
	private IWorkbenchAction actionShowPreferences;
	private Action actionOpenConsole;
	private Action actionOpenProgressView;

	/**
	 * @param configurer
	 */
	public NXMCActionBarAdvisor(IActionBarConfigurer configurer)
	{
		super(configurer);
	}

	@Override
	protected void makeActions(IWorkbenchWindow window)
	{
		actionExit = ActionFactory.QUIT.create(window);
		register(actionExit);

		actionAbout = ActionFactory.ABOUT.create(window);
		register(actionAbout);
		
		actionShowPreferences = ActionFactory.PREFERENCES.create(window);
		register(actionShowPreferences);

		actionOpenConsole = new Action()
		{
			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.jface.action.Action#run()
			 */
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
								page.showView(IConsoleConstants.ID_CONSOLE_VIEW);
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
		actionOpenConsole.setText(Messages.getString("NXMCActionBarAdvisor.menu_console")); //$NON-NLS-1$
		actionOpenConsole.setImageDescriptor(ConsolePlugin.getImageDescriptor(IConsoleConstants.IMG_VIEW_CONSOLE));

		actionOpenProgressView = new Action()
		{
			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.jface.action.Action#run()
			 */
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
		actionOpenProgressView.setText(Messages.getString("NXMCActionBarAdvisor.progress")); //$NON-NLS-1$
		actionOpenProgressView.setImageDescriptor(Activator.getImageDescriptor("icons/pview.gif")); //$NON-NLS-1$
	}

	@Override
	protected void fillMenuBar(IMenuManager menuBar)
	{
		MenuManager fileMenu = new MenuManager(Messages.getString("NXMCActionBarAdvisor.menu_file"), //$NON-NLS-1$
				IWorkbenchActionConstants.M_FILE);
		MenuManager viewMenu = new MenuManager(Messages.getString("NXMCActionBarAdvisor.menu_view"), IActionConstants.M_VIEW); //$NON-NLS-1$
		MenuManager configMenu = new MenuManager(Messages.getString("NXMCActionBarAdvisor.menu_configuration"), IActionConstants.M_CONFIG); //$NON-NLS-1$
		MenuManager toolsMenu = new MenuManager(Messages.getString("NXMCActionBarAdvisor.menu_tools"), IActionConstants.M_TOOLS); //$NON-NLS-1$
		MenuManager helpMenu = new MenuManager(Messages.getString("NXMCActionBarAdvisor.menu_help"), //$NON-NLS-1$
				IWorkbenchActionConstants.M_HELP);

		menuBar.add(fileMenu);
		menuBar.add(viewMenu);
		menuBar.add(configMenu);
		menuBar.add(toolsMenu);

		// Add a group marker indicating where action set menus will appear.
		menuBar.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
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
		viewMenu.add(actionOpenConsole);
		viewMenu.add(actionOpenProgressView);
		viewMenu.add(new GroupMarker(IActionConstants.M_TOOL_VIEW));
		
		// Tools
		toolsMenu.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));

		// Help
		helpMenu.add(actionAbout);
	}

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

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.ui.application.ActionBarAdvisor#fillStatusLine(org.eclipse
	 * .jface.action.IStatusLineManager)
	 */
	@Override
	protected void fillStatusLine(IStatusLineManager statusLine)
	{
		StatusLineContributionItem statusItem = new StatusLineContributionItem("ConnectionStatus"); //$NON-NLS-1$
		statusItem.setText(""); //$NON-NLS-1$
		statusLine.add(statusItem);
		Activator.getDefault().setStatusItemConnection(statusItem);
	}
}
