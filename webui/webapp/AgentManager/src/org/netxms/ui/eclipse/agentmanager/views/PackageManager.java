/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.packages.PackageInfo;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.Messages;
import org.netxms.ui.eclipse.agentmanager.dialogs.EditPackageMetadataDialog;
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
   public static final int COLUMN_TYPE = 2;
   public static final int COLUMN_VERSION = 3;
   public static final int COLUMN_PLATFORM = 4;
   public static final int COLUMN_FILE = 5;
   public static final int COLUMN_COMMAND = 6;
   public static final int COLUMN_DESCRIPTION = 7;

   private List<PackageInfo> packageList = null;
   private SortableTableViewer viewer;
   private Action actionRefresh;
   private Action actionUploadToServer;
   private Action actionRemove;
   private Action actionDeploy;
   private Action actionEditMetadata;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { Messages.get().PackageManager_ColumnID, Messages.get().PackageManager_ColumnName, "Type",
		      Messages.get().PackageManager_ColumnVersion, Messages.get().PackageManager_ColumnPlatform,
		      Messages.get().PackageManager_ColumnFile, "Command", Messages.get().PackageManager_ColumnDescription };
      final int[] widths = { 70, 120, 100, 90, 120, 300, 300, 400 };
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
            IStructuredSelection selection = viewer.getStructuredSelection();
				actionDeploy.setEnabled(selection.size() == 1);
            actionEditMetadata.setEnabled(selection.size() == 1);
				actionRemove.setEnabled(selection.size() > 0);
			}
		});
		
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editPackageMetadata();
         }
      });

		refresh();
	}

   /**
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
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refresh();
			}
		};
		
		actionUploadToServer = new Action(Messages.get().PackageManager_InstallAction, SharedIcons.ADD_OBJECT) {
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

      actionEditMetadata = new Action("&Edit metadata...", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editPackageMetadata();
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
		manager.add(actionUploadToServer);
		manager.add(actionRemove);
		manager.add(actionDeploy);
      manager.add(new Separator());
      manager.add(actionEditMetadata);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local toolbar
	 * @param manager menu manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionUploadToServer);
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
	}

	/**
	 * Fill context menu
	 * @param manager Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionDeploy);
      manager.add(new Separator());
      manager.add(actionUploadToServer);      
      manager.add(actionEditMetadata);
      manager.add(actionRemove);
	}

	/**
	 * Refresh package list
	 */
	private void refresh()
	{
		final NXCSession session = ConsoleSharedData.getSession();
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
      fd.setFilterExtensions(new String[] { "*.apkg", "*.exe", "*.msi", "*.msp", "*.msu", "*.npi", "*.tgz;*.tar.gz", "*.zip", "*.*" });
      String packageFileName = fd.open();
      if (packageFileName == null)
         return;

      try
		{
         // Guess package type, version, etc. from package file name
         boolean showMetadataDialog = true;
         final PackageInfo packageInfo;
         if (packageFileName.endsWith(".npi"))
         {
            packageInfo = new PackageInfo(new File(packageFileName));
            showMetadataDialog = false;
         }
         else
         {
            File packageFile = new File(packageFileName);
            String name = packageFile.getName();
            if (packageFileName.endsWith(".apkg"))
            {
               Pattern pattern = Pattern.compile("^nxagent-([0-9]+\\.[0-9]+\\.[0-9]+)(-[a-zA-Z0-9.]+)?(-[a-zA-Z0-9_]+)?\\.apkg$", Pattern.CASE_INSENSITIVE);
               Matcher matcher = pattern.matcher(name);
               if (matcher.matches())
               {
                  showMetadataDialog = false;
                  if (matcher.group(2) == null)
                  {
                     packageInfo = new PackageInfo("nxagent", "NetXMS Agent Source Package", name, "agent-installer", "src", matcher.group(1), "");
                  }
                  else
                  {
                     String os, platform;
                     if (matcher.group(2).startsWith("-aix"))
                     {
                        os = "AIX";
                        if (matcher.group(2).length() > 4)
                           os = os + " " + matcher.group(2).substring(4);
                        platform = "AIX-powerpc";
                     }
                     else if (matcher.group(2).equals("-hpux"))
                     {
                        os = "HP-UX";
                        platform = "HP-UX" + matcher.group(3);
                     }
                     else if (matcher.group(2).equals("-linux"))
                     {
                        os = "Linux";
                        platform = matcher.group(3).equals("-arm") ? "Linux-arm*" : "Linux" + matcher.group(3);
                     }
                     else if (matcher.group(2).equals("-solaris"))
                     {
                        os = "Solaris";
                        String arch;
                        if (matcher.group(3).equals("-sparc"))
                           arch = "-sun4u";
                        else if (matcher.group(3).equals("-i386"))
                           arch = "-i86pc";
                        else
                           arch = matcher.group(3);
                        platform = "SunOS" + arch;
                     }
                     else
                     {
                        os = matcher.group(2).substring(1);
                        platform = os + matcher.group(3);
                        showMetadataDialog = true;
                     }
                     packageInfo = new PackageInfo("nxagent", "NetXMS Agent for " + os, name, "agent-installer", platform, matcher.group(1), "");
                  }
               }
               else
               {
                  packageInfo = new PackageInfo(name, "", name, "executable", "*", "", "");
               }
            }
            else if (packageFileName.endsWith(".deb"))
            {
               packageInfo = new PackageInfo(name.substring(0, name.length() - 4), "", name, "deb", "Linux-*", "", "");
            }
            else if (packageFileName.endsWith(".exe"))
            {
               Pattern pattern = Pattern.compile("^nxagent-([0-9]+\\.[0-9]+\\.[0-9]+(-rc[0-9.]+|\\.[0-9]+)?)(-x64)?\\.exe$", Pattern.CASE_INSENSITIVE);
               Matcher matcher = pattern.matcher(name);
               if (matcher.matches())
               {
                  packageInfo = new PackageInfo("nxagent", "NetXMS Agent for Windows", name, "agent-installer", "windows-" + ((matcher.group(3) == null) ? "i386" : "x64"), matcher.group(1), "");
                  showMetadataDialog = false;
               }
               else
               {
                  pattern = Pattern.compile("^nxagent-atm-([0-9]+\\.[0-9]+\\.[0-9]+(-rc[0-9.]+|\\.[0-9]+)?)(-x64)?\\.exe$", Pattern.CASE_INSENSITIVE);
                  matcher = pattern.matcher(name);
                  if (matcher.matches())
                  {
                     packageInfo = new PackageInfo("nxagent-atm", "NetXMS Agent for ATM", name, "agent-installer", "windows-" + ((matcher.group(3) == null) ? "i386" : "x64"), matcher.group(1), "");
                     showMetadataDialog = false;
                  }
                  else
                  {
                     packageInfo = new PackageInfo(name, "", name, "executable", "windows-x64", "", "");
                  }
               }
            }
            else if (packageFileName.endsWith(".msi") || packageFileName.endsWith(".msp") || packageFileName.endsWith(".msu"))
            {
               packageInfo = new PackageInfo(name.substring(0, name.lastIndexOf('.')), "", name, name.substring(name.lastIndexOf('.') + 1), "windows-x64", "", "");
            }
            else if (packageFileName.endsWith(".rpm"))
            {
               packageInfo = new PackageInfo(name.substring(0, name.length() - 4), "", name, "rpm", "*", "", "");
            }
            else if (packageFileName.endsWith(".tar.gz") || packageFileName.endsWith(".tgz"))
            {
               int suffixLen = packageFileName.endsWith(".tar.gz") ? 7 : 4;
               packageInfo = new PackageInfo(name.substring(0, name.length() - suffixLen), "", name, "tgz", "*", "", "");
            }
            else if (packageFileName.endsWith(".zip"))
            {
               packageInfo = new PackageInfo(name.substring(0, name.length() - 4), "", name, "zip", "*", "", "");
            }
            else
            {
               packageInfo = new PackageInfo(name, "", name, "executable", "*", "", "");
            }
         }

         if (showMetadataDialog)
         {
            EditPackageMetadataDialog dlg = new EditPackageMetadataDialog(getSite().getShell(), packageInfo);
            if (dlg.open() != Window.OK)
               return;
         }

         final NXCSession session = ConsoleSharedData.getSession();
         new ConsoleJob(Messages.get().PackageManager_InstallPackage, this, Activator.PLUGIN_ID) {
            @Override
            protected void runInternal(final IProgressMonitor monitor) throws Exception
            {
               File packageFile = packageFileName.endsWith(".npi") ? new File(new File(packageFileName).getParent(), packageInfo.getFileName()) : new File(packageFileName);
               final long id = session.installPackage(packageInfo, packageFile, new ProgressListener() {
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
               packageInfo.setId(id);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     packageList.add(packageInfo);
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

	/**
	 * Remove selected package(s)
	 */
	private void removePackage()
	{
		if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.get().PackageManager_ConfirmDeleteTitle, Messages.get().PackageManager_ConfirmDeleteText))
			return;
		
      final NXCSession session = ConsoleSharedData.getSession();
      final Object[] packages = viewer.getStructuredSelection().toArray();
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
      IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() != 1)
			return;
		final PackageInfo pkg = (PackageInfo)selection.getFirstElement();
		
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), ObjectSelectionDialog.createNodeSelectionFilter(false));
		dlg.enableMultiSelection(true);
		if (dlg.open() != Window.OK)
			return;
		
		final Set<Long> objects = new HashSet<Long>();
		for(AbstractObject o : dlg.getSelectedObjects())
		{
			objects.add(o.getObjectId());
		}

      final NXCSession session = ConsoleSharedData.getSession();
      ConsoleJob job = new ConsoleJob(Messages.get().PackageManager_DeployAgentPackage, null, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            session.deployPackage(pkg.getId(), objects);
            runInUIThread(() -> MessageDialogHelper.openInformation(getSite().getShell(), "Package Deployment", "Package deployomet initiated successfully."));
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

   /**
    * Edit metadata for selected package
    */
   private void editPackageMetadata()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      final PackageInfo packageInfo = (PackageInfo)selection.getFirstElement();

      EditPackageMetadataDialog dlg = new EditPackageMetadataDialog(getSite().getShell(), packageInfo);
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Update package metadata", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.updatePackageMetadata(packageInfo);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.update(packageInfo, null);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update metadata for package";
         }
      }.start();
   }
}
