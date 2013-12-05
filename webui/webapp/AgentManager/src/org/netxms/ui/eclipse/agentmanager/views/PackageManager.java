/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.agentmanager.views;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.ProgressListener;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.packages.PackageDeploymentListener;
import org.netxms.client.packages.PackageInfo;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.Messages;
import org.netxms.ui.eclipse.agentmanager.views.helpers.PackageComparator;
import org.netxms.ui.eclipse.agentmanager.views.helpers.PackageLabelProvider;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Agent package manager
 */
public class PackageManager extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.PackageManager"; //$NON-NLS-1$
	
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_VERSION = 2;
	public static final int COLUMN_PLATFORM = 3;
	public static final int COLUMN_FILE = 4;
	public static final int COLUMN_DESCRIPTION = 5;

	private List<PackageInfo> packageList = null;
	private SortableTableViewer viewer;
	private Action actionRefresh;
	private Action actionInstall;
	private Action actionRemove;
	private Action actionDeploy;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { Messages.get().PackageManager_ColumnID, Messages.get().PackageManager_ColumnName, Messages.get().PackageManager_ColumnVersion, Messages.get().PackageManager_ColumnPlatform, Messages.get().PackageManager_ColumnFile, Messages.get().PackageManager_ColumnDescription };
		final int[] widths = { 70, 120, 90, 120, 150, 400 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new PackageLabelProvider());
		viewer.setComparator(new PackageComparator());

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				actionDeploy.setEnabled(selection.size() == 1);
				actionRemove.setEnabled(selection.size() > 0);
			}
		});
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().PackageManager_OpenDatabase, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				try
				{
					session.lockPackageDatabase();
				}
				catch(NXCException e)
				{
					// New versions may not require package DB lock
					if (e.getErrorCode() != RCC.NOT_IMPLEMENTED)
						throw e;
				}
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						refresh();
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().PackageManager_OpenError;
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
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				refresh();
			}
		};
		
		actionInstall = new Action(Messages.get().PackageManager_InstallAction, SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				installPackage();
			}
		};
		
		actionRemove = new Action(Messages.get().PackageManager_RemoveAction, SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				removePackage();
			}
		};
		
		actionDeploy = new Action(Messages.get().PackageManager_DeployAction, Activator.getImageDescriptor("icons/package_deploy.gif")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				deployPackage();
			}
		};
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
		manager.add(actionInstall);
		manager.add(actionRemove);
		manager.add(actionDeploy);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local toolbar
	 * @param manager menu manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionInstall);
		manager.add(actionRefresh);
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

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}
	
	/**
	 * Fill context menu
	 * @param manager Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionDeploy);
		manager.add(actionRemove);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}
	
	/**
	 * Refresh package list
	 */
	private void refresh()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().PackageManager_LoadPkgList, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<PackageInfo> list = session.getInstalledPackages();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						packageList = list;
						viewer.setInput(list.toArray());
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().PackageManager_PkgListLoadError;
			}
		}.start();
	}
	
	/**
	 * Install new package
	 */
	private void installPackage()
	{
		FileDialog fd = new FileDialog(getSite().getShell(), SWT.OPEN);
		fd.setText(Messages.get().PackageManager_SelectFile);
		String npiName = fd.open();
		if (npiName != null)
		{
			try
			{
				final File npiFile = new File(npiName);
				final PackageInfo p = new PackageInfo(npiFile);
				final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
				new ConsoleJob(Messages.get().PackageManager_InstallPackage, this, Activator.PLUGIN_ID, null) {
					@Override
					protected void runInternal(final IProgressMonitor monitor) throws Exception
					{
						final long id = session.installPackage(p, new File(npiFile.getParent(), p.getFileName()), new ProgressListener() {
							long prevAmount = 0;
							
							@Override
							public void setTotalWorkAmount(long amount)
							{
								monitor.beginTask(Messages.get(getDisplay()).PackageManager_UploadPackage, (int)amount);
							}
							
							@Override
							public void markProgress(long amount)
							{
								monitor.worked((int)(amount - prevAmount));
								prevAmount = amount;
							}
						});
						p.setId(id);
						runInUIThread(new Runnable() {
							@Override
							public void run()
							{
								packageList.add(p);
								viewer.setInput(packageList.toArray());
							}
						});
					}
					
					@Override
					protected String getErrorMessage()
					{
						return Messages.get().PackageManager_InstallError;
					}
				}.start();
			}
			catch(IOException e)
			{
				MessageDialogHelper.openError(getSite().getShell(), Messages.get().PackageManager_Error, Messages.get().PackageManager_PkgFileOpenError + e.getLocalizedMessage());
			}
		}
	}
	
	/**
	 * Remove selected package(s)
	 */
	private void removePackage()
	{
		if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.get().PackageManager_ConfirmDeleteTitle, Messages.get().PackageManager_ConfirmDeleteText))
			return;
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final Object[] packages = ((IStructuredSelection)viewer.getSelection()).toArray();
		final List<Object> removedPackages = new ArrayList<Object>();
		new ConsoleJob(Messages.get().PackageManager_DeletePackages, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(Object p : packages)
				{
					session.removePackage(((PackageInfo)p).getId());
					removedPackages.add(p);
				}
			}

			@Override
			protected void jobFinalize()
			{
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						packageList.removeAll(removedPackages);
						viewer.setInput(packageList.toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().PackageManager_PkgDeleteError;
			}
		}.start();
	}
	
	/**
	 * Deploy package on managed nodes
	 */
	private void deployPackage()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		final PackageInfo pkg = (PackageInfo)selection.getFirstElement();
		
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), null, ObjectSelectionDialog.createNodeSelectionFilter(false));
		dlg.enableMultiSelection(true);
		if (dlg.open() != Window.OK)
			return;
		
		final Set<Long> objects = new HashSet<Long>();
		for(AbstractObject o : dlg.getSelectedObjects())
		{
			objects.add(o.getObjectId());
		}
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		ConsoleJob job = new ConsoleJob(Messages.get().PackageManager_DeployAgentPackage, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.deployPackage(pkg.getId(), objects.toArray(new Long[objects.size()]), new PackageDeploymentListener() {
					private PackageDeploymentMonitor monitor = null;
					
					@Override
					public void statusUpdate(long nodeId, int status, String message)
					{
						if (monitor != null)
							monitor.statusUpdate(nodeId, status, message);
					}
					
					@Override
					public void deploymentStarted()
					{
						final Object sync = new Object();
						synchronized(sync)
						{
							runInUIThread(new Runnable() {
								@Override
								public void run()
								{
									try
									{
										monitor = (PackageDeploymentMonitor)PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().showView(PackageDeploymentMonitor.ID, toString(), IWorkbenchPage.VIEW_ACTIVATE);
									}
									catch(PartInitException e)
									{
										MessageDialogHelper.openError(getSite().getShell(), Messages.get().PackageManager_Error, Messages.get().PackageManager_ErrorOpenView + e.getLocalizedMessage());
									}
									synchronized(sync)
									{
										sync.notify();
									}
								}
							});

							try
							{
								sync.wait();
							}
							catch(InterruptedException e)
							{
							}
						}
					}
					
					@Override
					public void deploymentComplete()
					{
						runInUIThread(new Runnable() {
							@Override
							public void run()
							{
								MessageDialogHelper.openInformation(getSite().getShell(), Messages.get().PackageManager_Information, Messages.get().PackageManager_PkgDepCompleted);
							}
						});
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().PackageManager_DepStartError;
			}
		};
		job.setUser(false);
		job.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().PackageManager_UnlockDatabase, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				try
				{
					session.unlockPackageDatabase();
				}
				catch(NXCException e)
				{
					// New versions may not require package DB lock
					if (e.getErrorCode() != RCC.NOT_IMPLEMENTED)
						throw e;
				}
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().PackageManager_DBUnlockError;
			}
		}.start();
		super.dispose();
	}
}
