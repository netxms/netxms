/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.agentmanagement.views;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.packages.PackageInfo;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.dialogs.EditPackageMetadataDialog;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.PackageComparator;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.PackageLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Agent package manager
 */
public class PackageManager extends ConfigurationView
{
   private I18n i18n = LocalizationHelper.getI18n(PackageManager.class);

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
   private Action actionUploadToServer;
   private Action actionRemove;
   private Action actionEditMetadata;
   
   /**
    * Constructor
    */
   public PackageManager()
   {
      super(LocalizationHelper.getI18n(PackageManager.class).tr("Packages"), ResourceManager.getImageDescriptor("icons/config-views/package-manager.png"), "configuration.package-manager", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   protected void createContent(Composite parent)
	{
		final String[] names = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Type"),
		      i18n.tr("Version"), i18n.tr("Platform"),
            i18n.tr("File"), i18n.tr("Command"), i18n.tr("Description") };
      final int[] widths = { 70, 120, 100, 90, 120, 300, 300, 400 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new PackageLabelProvider());
		viewer.setComparator(new PackageComparator());

		createActions();
		createPopupMenu();

      viewer.addSelectionChangedListener((event) -> {
         IStructuredSelection selection = viewer.getStructuredSelection();
         actionEditMetadata.setEnabled(selection.size() == 1);
         actionRemove.setEnabled(selection.size() > 0);
		});
		
      viewer.addDoubleClickListener((event) -> editPackageMetadata());

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
      actionUploadToServer = new Action(i18n.tr("&Upload to server..."), SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				installPackage();
			}
		};

		actionRemove = new Action(i18n.tr("&Remove"), SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				removePackage();
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
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

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
      manager.add(actionUploadToServer);
      manager.add(actionEditMetadata);
      manager.add(actionRemove);
	}

	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionUploadToServer);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionUploadToServer);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
	@Override
	public void refresh()
	{
		final NXCSession session = Registry.getSession();
		new Job(i18n.tr("Load package list"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final List<PackageInfo> list = session.getInstalledPackages();
            runInUIThread(() -> {
               packageList = list;
               viewer.setInput(list.toArray());
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot get package list from server");
			}
		}.start();
	}

	/**
	 * Install new package
	 */
	private void installPackage()
	{
		FileDialog fd = new FileDialog(getWindow().getShell(), SWT.OPEN);
		fd.setText(i18n.tr("Select Package File"));
      WidgetHelper.setFileDialogFilterExtensions(fd, new String[] { "*.apkg", "*.deb", "*.exe", "*.msi", "*.msp", "*.msu", "*.npi", "*.rpm", "*.tgz;*.tar.gz", "*.zip", "*.*" });
      WidgetHelper.setFileDialogFilterNames(fd,
            new String[] {
                  i18n.tr("NetXMS Agent Package"), i18n.tr("Debian Binary Package"), i18n.tr("Executable"), i18n.tr("Windows Installer Package"), i18n.tr("Windows Installer Patch"),
                  i18n.tr("Windows Update Package"), i18n.tr("NetXMS Package Info"), i18n.tr("RPM Package"), i18n.tr("Compressed TAR Archive"), i18n.tr("ZIP Archive"), 
                  i18n.tr("All Files")
            });
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
               Pattern pattern = Pattern.compile("^nxagent-([0-9]+\\.[0-9]+\\.[0-9]+(-rc[0-9.]+|\\.[0-9]+)?)(-x64|-x86|-aarch64)?\\.exe$", Pattern.CASE_INSENSITIVE);
               Matcher matcher = pattern.matcher(name);
               if (matcher.matches())
               {
                  packageInfo = new PackageInfo("nxagent", "NetXMS Agent for Windows", name, "agent-installer", windowsPlatformNameFromSuffix(matcher.group(3)), matcher.group(1), "");
                  showMetadataDialog = false;
               }
               else
               {
                  pattern = Pattern.compile("^nxagent-atm-([0-9]+\\.[0-9]+\\.[0-9]+(-rc[0-9.]+|\\.[0-9]+)?)(-x64|-x86|-aarch64)?\\.exe$", Pattern.CASE_INSENSITIVE);
                  matcher = pattern.matcher(name);
                  if (matcher.matches())
                  {
                     packageInfo = new PackageInfo("nxagent-atm", "NetXMS Agent for ATM", name, "agent-installer", windowsPlatformNameFromSuffix(matcher.group(3)), matcher.group(1), "");
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
            EditPackageMetadataDialog dlg = new EditPackageMetadataDialog(getWindow().getShell(), packageInfo);
            if (dlg.open() != Window.OK)
               return;
         }

         final NXCSession session = Registry.getSession();
         new Job(i18n.tr("Install package"), this) {
            @Override
            protected void run(final IProgressMonitor monitor) throws Exception
            {
               File packageFile = packageFileName.endsWith(".npi") ? new File(new File(packageFileName).getParent(), packageInfo.getFileName()) : new File(packageFileName);
               final long id = session.installPackage(packageInfo, packageFile, new ProgressListener() {
                  long prevAmount = 0;

                  @Override
                  public void setTotalWorkAmount(long amount)
                  {
                     monitor.beginTask(i18n.tr("Upload package file"), (int)amount);
                  }

                  @Override
                  public void markProgress(long amount)
                  {
                     monitor.worked((int)(amount - prevAmount));
                     prevAmount = amount;
                  }
               });
               packageInfo.setId(id);
               runInUIThread(() -> {
                  packageList.add(packageInfo);
                  viewer.setInput(packageList.toArray());
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot install package");
            }
         }.start();
      }
      catch(IOException e)
      {
         MessageDialogHelper.openError(getWindow().getShell(), i18n.tr("Error"), i18n.tr("Cannot open package information file: ") + e.getLocalizedMessage());
		}
	}

   /**
    * Derive platform name for Windows agent from installer's suffix.
    *
    * @param suffix installers suffix
    * @return platform name
    */
   private static String windowsPlatformNameFromSuffix(String suffix)
   {
      if ((suffix == null) || suffix.equalsIgnoreCase("-x86"))
         return "windows-i386";
      if (suffix.equalsIgnoreCase("-aarch64"))
         return "windows-aarch64";
      return "windows-x64";
   }

	/**
	 * Remove selected package(s)
	 */
	private void removePackage()
	{
		if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Package Delete"), i18n.tr("Are you sure you wish to delete selected packages?")))
			return;
		
      final NXCSession session = Registry.getSession();
      final Object[] packages = viewer.getStructuredSelection().toArray();
		final List<Object> removedPackages = new ArrayList<Object>();
		new Job(i18n.tr("Delete agent packages"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
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
            runInUIThread(() -> {
               packageList.removeAll(removedPackages);
               viewer.setInput(packageList.toArray());
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot delete package from server");
			}
		}.start();
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

      EditPackageMetadataDialog dlg = new EditPackageMetadataDialog(getWindow().getShell(), packageInfo);
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job("Update package metadata", this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updatePackageMetadata(packageInfo);
            runInUIThread(() -> viewer.update(packageInfo, null));
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update metadata for package";
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}
