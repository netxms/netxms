/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.net.URL;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IContributionItem;
import org.eclipse.jface.action.ICoolBarManager;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IStatusLineManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.StatusLineContributionItem;
import org.eclipse.jface.action.ToolBarContributionItem;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.osgi.service.datalocation.Location;
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
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Action bar advisor for management console
 */
public class NXMCActionBarAdvisor extends ActionBarAdvisor
{
	private IWorkbenchAction actionExit;
	private IWorkbenchAction actionAbout;
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
	private IWorkbenchAction actionQuickAccess;
	private IWorkbenchAction actionShowViewMenu;
	private Action actionOpenProgressView;
	private Action actionFullScreen;
	private Action actionLangChinese;
	private Action actionLangEnglish;
	private Action actionLangRussian;
	private Action actionLangSpanish;
	private IContributionItem contribItemShowView;
	private IContributionItem contribItemOpenPerspective;

	/**
	 * @param configurer
	 */
	public NXMCActionBarAdvisor(IActionBarConfigurer configurer)
	{
		super(configurer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.ActionBarAdvisor#makeActions(org.eclipse.ui.IWorkbenchWindow)
	 */
	@Override
	protected void makeActions(IWorkbenchWindow window)
	{
		contribItemShowView = ContributionItemFactory.VIEWS_SHORTLIST.create(window);
		contribItemOpenPerspective = ContributionItemFactory.PERSPECTIVES_SHORTLIST.create(window);
		
		actionExit = ActionFactory.QUIT.create(window);
		register(actionExit);

		actionAbout = ActionFactory.ABOUT.create(window);
		register(actionAbout);
		
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
		
		actionQuickAccess = ActionFactory.SHOW_QUICK_ACCESS.create(window);
		register(actionQuickAccess);
		
		actionShowViewMenu = ActionFactory.SHOW_VIEW_MENU.create(window);
		register(actionShowViewMenu);
		
		actionOpenProgressView = new Action() {
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
		actionOpenProgressView.setText(Messages.NXMCActionBarAdvisor_progress); //$NON-NLS-1$
		actionOpenProgressView.setImageDescriptor(Activator.getImageDescriptor("icons/pview.gif")); //$NON-NLS-1$
		
		actionFullScreen = new Action(Messages.NXMCActionBarAdvisor_FullScreen, Action.AS_CHECK_BOX) { //$NON-NLS-1$
			@Override
			public void run()
			{
				boolean fullScreen = !PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell().getFullScreen();
				PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell().setFullScreen(fullScreen);
				actionFullScreen.setChecked(fullScreen);
			}
		};
		actionFullScreen.setChecked(false);
		actionFullScreen.setId("org.netxms.ui.eclipse.console.actions.full_screen"); //$NON-NLS-1$
		actionFullScreen.setActionDefinitionId("org.netxms.ui.eclipse.console.commands.full_screen"); //$NON-NLS-1$
		getActionBarConfigurer().registerGlobalAction(actionFullScreen);
		
		actionLangChinese = new Action(Messages.NXMCActionBarAdvisor_LangChinese, Activator.getImageDescriptor("icons/lang/zh.png")) { //$NON-NLS-1$
			public void run()
			{
				setLanguage("zh"); //$NON-NLS-1$
			}
		};
		actionLangEnglish = new Action(Messages.NXMCActionBarAdvisor_LangEnglish, Activator.getImageDescriptor("icons/lang/gb.png")) { //$NON-NLS-1$ //$NON-NLS-2$
			public void run()
			{
				setLanguage("en"); //$NON-NLS-1$
			}
		};
		actionLangRussian = new Action(Messages.NXMCActionBarAdvisor_LangRussian, Activator.getImageDescriptor("icons/lang/ru.png")) { //$NON-NLS-1$ //$NON-NLS-2$
			public void run()
			{
				setLanguage("ru"); //$NON-NLS-1$
			}
		};
		actionLangSpanish = new Action(Messages.NXMCActionBarAdvisor_LangSpanish, Activator.getImageDescriptor("icons/lang/es.png")) { //$NON-NLS-1$ //$NON-NLS-2$
			public void run()
			{
				setLanguage("es"); //$NON-NLS-1$
			}
		};
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.ActionBarAdvisor#fillMenuBar(org.eclipse.jface.action.IMenuManager)
	 */
	@Override
	protected void fillMenuBar(IMenuManager menuBar)
	{
		MenuManager fileMenu = new MenuManager(Messages.NXMCActionBarAdvisor_menu_file, //$NON-NLS-1$
				IWorkbenchActionConstants.M_FILE);
		MenuManager viewMenu = new MenuManager(Messages.NXMCActionBarAdvisor_menu_view, IActionConstants.M_VIEW); //$NON-NLS-1$
		MenuManager monitorMenu = new MenuManager(Messages.NXMCActionBarAdvisor_menu_monitor, IActionConstants.M_MONITOR); //$NON-NLS-1$
		MenuManager configMenu = new MenuManager(Messages.NXMCActionBarAdvisor_menu_configuration, IActionConstants.M_CONFIG); //$NON-NLS-1$
		MenuManager toolsMenu = new MenuManager(Messages.NXMCActionBarAdvisor_menu_tools, IActionConstants.M_TOOLS); //$NON-NLS-1$
		MenuManager windowMenu = new MenuManager(Messages.NXMCActionBarAdvisor_Window, IWorkbenchActionConstants.M_WINDOW); //$NON-NLS-1$
		MenuManager helpMenu = new MenuManager(Messages.NXMCActionBarAdvisor_menu_help, //$NON-NLS-1$
				IWorkbenchActionConstants.M_HELP);

		menuBar.add(fileMenu);
		menuBar.add(viewMenu);
		menuBar.add(monitorMenu);
		menuBar.add(configMenu);
		menuBar.add(toolsMenu);

		// Add a group marker indicating where action set menus will appear.
		menuBar.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		
		menuBar.add(windowMenu);
		menuBar.add(helpMenu);
		
		// Language selection
		final MenuManager langMenu = new MenuManager(Messages.NXMCActionBarAdvisor_Language); //$NON-NLS-1$
		langMenu.add(actionLangChinese);
		langMenu.add(actionLangEnglish);
		langMenu.add(actionLangRussian);
		langMenu.add(actionLangSpanish);

		// File
		fileMenu.add(actionShowPreferences);
		fileMenu.add(langMenu);
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
		windowMenu.add(actionFullScreen);
		windowMenu.add(new Separator());
		
		MenuManager openPerspectiveMenuMgr = new MenuManager(Messages.NXMCActionBarAdvisor_OpenPerspective, "openPerspective"); //$NON-NLS-1$ //$NON-NLS-2$
		openPerspectiveMenuMgr.add(contribItemOpenPerspective);
		windowMenu.add(openPerspectiveMenuMgr);
		
		final MenuManager showViewMenuMgr = new MenuManager(Messages.NXMCActionBarAdvisor_ShowView, "showView"); //$NON-NLS-1$ //$NON-NLS-2$
		showViewMenuMgr.add(contribItemShowView);
		windowMenu.add(showViewMenuMgr);
		
		windowMenu.add(new Separator());
		windowMenu.add(actionCustomizePerspective);
		windowMenu.add(actionSavePerspective);
		windowMenu.add(actionResetPerspective);
		windowMenu.add(actionClosePerspective);
		windowMenu.add(actionCloseAllPerspectives);
		windowMenu.add(new Separator());
		
		final MenuManager navMenu = new MenuManager(Messages.NXMCActionBarAdvisor_Navigation, IWorkbenchActionConstants.M_NAVIGATE); //$NON-NLS-1$
		windowMenu.add(navMenu);
		navMenu.add(actionQuickAccess);
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
	
	/**
	 * Set program language
	 * 
	 * @param locale
	 */
	private void setLanguage(String locale)
	{
		if (!MessageDialogHelper.openConfirm(null, Messages.NXMCActionBarAdvisor_ConfirmRestart, Messages.NXMCActionBarAdvisor_RestartConsoleMessage))
			return;
		
		Activator.getDefault().getPreferenceStore().setValue("NL", locale); //$NON-NLS-1$
		
		// Patch product's .ini file
		final Location configArea = Platform.getInstallLocation();
		if (configArea != null)
		{
			BufferedReader in = null;
			BufferedWriter out = null;
			
			try
			{
				final URL iniFileUrl = new URL(configArea.getURL().toExternalForm() + "nxmc.ini"); //$NON-NLS-1$
				final String iniFileName = iniFileUrl.getFile();
				final File iniFile = new File(iniFileName);
				
				final File iniFileBackup = new File(iniFileName + ".bak"); //$NON-NLS-1$
				iniFileBackup.delete();
				iniFile.renameTo(iniFileBackup); //$NON-NLS-1$
				
				in = new BufferedReader(new FileReader(iniFileName + ".bak")); //$NON-NLS-1$
				out = new BufferedWriter(new FileWriter(iniFileName));
				
				int state = 0;
				while(true)
				{
					String line = in.readLine();
					if (line == null)
						break;
					
					switch(state)
					{
						case 0:
							if (line.equals("-nl")) //$NON-NLS-1$
								state = 1;
							break;
						case 1:	// -nl argument
							line = locale;
							state = 0;
							break;
					}
					out.write(line);
					out.newLine();
				}
			}
			catch(Exception e)
			{
				Activator.getDefault().getLog().log(new Status(Status.ERROR, Activator.PLUGIN_ID, Status.OK, "Exception in setLanguage()", e)); //$NON-NLS-1$
			}
			finally
			{
				try
				{
					if (in != null)
						in.close();
					if (out != null)
						out.close();
				}
				catch(IOException e)
				{
				}
			}
		}
		System.getProperties().setProperty("eclipse.exitdata", "-nl " + locale); //$NON-NLS-1$ //$NON-NLS-2$
		PlatformUI.getWorkbench().restart();
	}
}
