package org.netxms.ui.eclipse.console;

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
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.actions.ActionFactory;
import org.eclipse.ui.actions.ActionFactory.IWorkbenchAction;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.netxms.ui.eclipse.shared.IActionConstants;

public class NXMCActionBarAdvisor extends ActionBarAdvisor
{
	// Actions - important to allocate these only in makeActions, and then use them
	// in the fill methods.  This ensures that the actions aren't recreated
	// when fillActionBars is called with FILL_PROXY.
	private IWorkbenchAction exitAction;
	private IWorkbenchAction aboutAction;
	
	public NXMCActionBarAdvisor(IActionBarConfigurer configurer)
	{
		super(configurer);
	}
	
	@Override
	protected void makeActions(IWorkbenchWindow window)
	{
		exitAction = ActionFactory.QUIT.create(window);
		register(exitAction);
		
		aboutAction = ActionFactory.ABOUT.create(window);
		register(aboutAction);
	}
	
	@Override
	protected void fillMenuBar(IMenuManager menuBar)
	{
		MenuManager fileMenu = new MenuManager("&File", IWorkbenchActionConstants.M_FILE);
		MenuManager viewMenu = new MenuManager("&View", IActionConstants.M_VIEW);
		MenuManager helpMenu = new MenuManager("&Help", IWorkbenchActionConstants.M_HELP);
		
		menuBar.add(fileMenu);
		menuBar.add(viewMenu);

		// Add a group marker indicating where action set menus will appear.
		menuBar.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		menuBar.add(helpMenu);
		
		// File
		fileMenu.add(new Separator());
		fileMenu.add(exitAction);
		
		// View
		viewMenu.add(new GroupMarker(IActionConstants.M_PRIMARY_VIEW));
		viewMenu.add(new Separator());
		viewMenu.add(new GroupMarker(IActionConstants.M_TOOL_VIEW));
		viewMenu.add(new Separator());
		MenuManager configMenu = new MenuManager("&Configuration", IActionConstants.M_CONFIG);
		configMenu.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		viewMenu.add(configMenu);
		
		// Help
		helpMenu.add(aboutAction);
	}

	@Override
	protected void fillCoolBar(ICoolBarManager coolBar) 
   {
       IToolBarManager toolbar = new ToolBarManager(SWT.FLAT | SWT.TRAIL);
       coolBar.add(new ToolBarContributionItem(toolbar, "view"));
   }

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.ActionBarAdvisor#fillStatusLine(org.eclipse.jface.action.IStatusLineManager)
	 */
	@Override
	protected void fillStatusLine(IStatusLineManager statusLine)
	{
		StatusLineContributionItem statusItem = new StatusLineContributionItem("ConnectionStatus");
		statusItem.setText("");
		statusLine.add(statusItem);	
		Activator.getDefault().setStatusItemConnection(statusItem);
		
		MemoryMonitor mm = new MemoryMonitor("memoryMonitor");
		statusLine.add(mm);
	}
}
