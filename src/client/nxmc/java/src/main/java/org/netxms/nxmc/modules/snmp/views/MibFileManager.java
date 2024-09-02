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
package org.netxms.nxmc.modules.snmp.views;

import java.io.File;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.server.ServerFile;
import org.netxms.client.snmp.MibCompilationLogEntry;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.TextConsole;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.views.helpers.ServerFileComparator;
import org.netxms.nxmc.modules.filemanager.views.helpers.ServerFileLabelProvider;
import org.netxms.nxmc.modules.snmp.views.helpers.MibCompilationErrorLogProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Custom MIB files manager
 */
public class MibFileManager extends ConfigurationView implements SessionListener
{
   private final I18n i18n = LocalizationHelper.getI18n(MibFileManager.class);

   private static final String TABLE_CONFIG_PREFIX = "MibFileManager";

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_SIZE = 2;

   // Columnsfor error tbale
   public static final int ERROR_COLUMN_SEVERITY = 0;
   public static final int ERROR_COLUMN_LOCATION = 1;
   public static final int ERROR_COLUMN_MESSAGE = 2;

   private NXCSession session = Registry.getSession();
   private SortableTableViewer viewer;
   private String filterString = "";
   private Image errorLogIcon;
   private Image outputIcon;
   private TextConsole outputViewer;
   private SortableTableViewer errorLogViewer;
   private Action actionUpload;
   private Action actionDelete;
   private Action actionCompile;
   private List<MibCompilationLogEntry> errors = new ArrayList<MibCompilationLogEntry>();

   /**
    * Create server configuration variable view
    */
   public MibFileManager()
   {
      super(LocalizationHelper.getI18n(MibFileManager.class).tr("SNMP MIB Files"), ResourceManager.getImageDescriptor("icons/config-views/mib-files.png"), "config.mib-files", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      SashForm content = new SashForm(parent, SWT.VERTICAL);

      Composite listArea = new Composite(content, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      listArea.setLayout(layout);

      final String[] columnNames = { i18n.tr("Name"), i18n.tr("Type"), i18n.tr("Size"), i18n.tr("Modified") };
      final int[] columnWidths = { 300, 150, 300, 300 };
      viewer = new SortableTableViewer(listArea, columnNames, columnWidths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      WidgetHelper.restoreTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ServerFileLabelProvider());
      viewer.setComparator(new ServerFileComparator());
      viewer.addFilter(new ViewerFilter() {
         @Override
         public boolean select(Viewer viewer, Object parentElement, Object element)
         {
            return filterString.isEmpty() || ((ServerFile)element).getName().toLowerCase().contains(filterString);
         }
      });
      setFilterClient(viewer, new AbstractViewerFilter() {
         @Override
         public void setFilterString(String string)
         {
            filterString = string.toLowerCase();
         }
      });
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionDelete.setEnabled(selection.size() > 0);
            }
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
         }
      });
      viewer.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      new Label(listArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      errorLogIcon = ResourceManager.getImage("icons/error_log.png");
      outputIcon = ResourceManager.getImage("icons/console.png");

      Composite outputArea = new Composite(content, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      outputArea.setLayout(layout);

      new Label(outputArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      CTabFolder outputTabFolder = new CTabFolder(outputArea, SWT.TOP);
      outputTabFolder.setTabHeight(26);
      WidgetHelper.disableTabFolderSelectionBar(outputTabFolder);
      outputTabFolder.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      CTabItem tabItem = new CTabItem(outputTabFolder, SWT.NONE);
      tabItem.setText("Output");
      tabItem.setImage(outputIcon);

      outputViewer = new TextConsole(outputTabFolder, SWT.NONE);
      tabItem.setControl(outputViewer);

      tabItem = new CTabItem(outputTabFolder, SWT.NONE);
      tabItem.setText("Error Log");
      tabItem.setImage(errorLogIcon);

      final String[] names = { i18n.tr("Severity"), i18n.tr("Location"), i18n.tr("Message") };
      final int[] widths = { 100, 300, 600 };
      errorLogViewer = new SortableTableViewer(outputTabFolder, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      errorLogViewer.setContentProvider(new ArrayContentProvider());
      errorLogViewer.setLabelProvider(new MibCompilationErrorLogProvider());
      errorLogViewer.setComparator(new ElementLabelComparator((ILabelProvider)errorLogViewer.getLabelProvider()));
      tabItem.setControl(errorLogViewer.getControl());
      errorLogViewer.setInput(errors);

      outputTabFolder.setSelection(0);

      content.setSashWidth(3);
      content.setWeights(new int[] { 70, 30 });

      createActions();
      createContextMenu();
      session.addListener(this);

      refresh();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionUpload = new Action(i18n.tr("&Upload..."), SharedIcons.UPLOAD) {
         @Override
         public void run()
         {
            uploadFile();
         }
      };
      addKeyBinding("M1+U", actionUpload);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteFile();
         }
      };
      addKeyBinding("M1+D", actionDelete);

      actionCompile = new Action(i18n.tr("&Compile"), ResourceManager.getImageDescriptor("icons/compile.png")) {
         @Override
         public void run()
         {
            compileMibFiles();
         }
      };
      addKeyBinding("F7", actionCompile);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionUpload);
      manager.add(actionCompile);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionUpload);
      manager.add(actionCompile);
   }

   /**
    * Create context menu for file list
    */
   private void createContextMenu()
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
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      if (!viewer.getStructuredSelection().isEmpty())
      {
         mgr.add(actionDelete);
         mgr.add(new Separator());
      }
      mgr.add(actionUpload);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Reading MIB file list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final ServerFile[] files = session.listMibFiles();
            runInUIThread(() -> viewer.setInput(files));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of MIB files");
         }
      }.start();
   }

   /**
    * Upload file(s)
    */
   private void uploadFile()
   {
      FileDialog fd = new FileDialog(getWindow().getShell(), SWT.OPEN | SWT.MULTI);
      fd.setText("Select Files");
      String selection = fd.open();
      if (selection == null)
         return;

      final List<File> files = new ArrayList<File>();
      for(String fname : fd.getFileNames())
      {
         files.add(Paths.get(fname).isAbsolute() ? new File(fname) : new File(new File(selection).getParentFile(), fname));
      }
      new Job(i18n.tr("Uploading file to server"), this) {
         @Override
         protected void run(final IProgressMonitor monitor) throws Exception
         {
            for(final File localFile : files)
            {
               final String remoteFile = localFile.getName();
               session.uploadMibFile(localFile, remoteFile, new ProgressListener() {
                  private long prevWorkDone = 0;

                  @Override
                  public void setTotalWorkAmount(long workTotal)
                  {
                     monitor.beginTask(localFile.getAbsolutePath(), (int)workTotal);
                  }

                  @Override
                  public void markProgress(long workDone)
                  {
                     monitor.worked((int)(workDone - prevWorkDone));
                     prevWorkDone = workDone;
                  }
               });
               monitor.done();
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot upload file to server");
         }
      }.start();
   }

   /**
    * Delete selected file
    */
   private void deleteFile()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Delete File"), i18n.tr("Are you sure you want to delete selected files?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Deleting server files"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               session.deleteMibFile(((ServerFile)objects[i]).getName());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete server file");
         }
      }.start();
   }

   /**
    * Compile MIB files
    */
   private void compileMibFiles()
   {
      actionCompile.setEnabled(false);

      outputViewer.setText("");
      errors.clear();
      errorLogViewer.refresh();
      new Job(i18n.tr("Compiling MIB  files"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.compileMibs((c) -> processCompilationLogEntry(c));
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> {
               actionCompile.setEnabled(true);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot compile MIB files");
         }
      }.start();
   }

   /**
    * Process new entry
    * 
    * @param entry new entry
    */
   private void processCompilationLogEntry(MibCompilationLogEntry entry)
   {
      getDisplay().asyncExec(() -> {
         switch(entry.getType())
         {
            case STAGE:
            case STATE:
               outputViewer.append("\u001b[34;1m" + entry.getText() + "\u001b[0m");
               break;
            case FILE:
               outputViewer.append("   \u001b[37m" + entry.getFileName() + "\u001b[0m");
               break;
            case WARNING:
               outputViewer.append("   \u001b[33;1mWarning:\u001b[36m " + entry.getFileName() + "\u001b[0m: " + entry.getText());
               break;
            case ERROR:
               outputViewer.append("   \u001b[31;1mError:\u001b[36m " + entry.getFileName() + "\u001b[0m: " + entry.getText());
               break;
         }
         outputViewer.append("\n");

         if (entry.isErrorInformation())
         {
            errors.add(entry);
            errorLogViewer.refresh();
         }
      });
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      if ((n.getCode() == SessionNotification.FILE_LIST_CHANGED) && (n.getSubCode() == 1))
      {
         getDisplay().asyncExec(() -> refresh());
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(this);
      errorLogIcon.dispose();
      outputIcon.dispose();
      super.dispose();
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
