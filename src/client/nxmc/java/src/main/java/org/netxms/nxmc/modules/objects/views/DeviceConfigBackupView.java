/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.base.DiffMatchPatch;
import org.netxms.base.DiffMatchPatch.Diff;
import org.netxms.client.DeviceConfigBackup;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.DiffViewer;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.StyledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.DeviceConfigBackupComparator;
import org.netxms.nxmc.modules.objects.views.helpers.DeviceConfigBackupFilter;
import org.netxms.nxmc.modules.objects.views.helpers.DeviceConfigBackupLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Device configuration backup view
 */
public class DeviceConfigBackupView extends ObjectView
{
   public static final int COLUMN_TIMESTAMP = 0;
   public static final int COLUMN_RUNNING_CONFIG = 1;
   public static final int COLUMN_STARTUP_CONFIG = 2;
   public static final int COLUMN_STATUS = 3;

   private static final int TAB_RUNNING = 0;
   private static final int TAB_STARTUP = 1;
   private static final int TAB_DIFF = 2;

   private final I18n i18n = LocalizationHelper.getI18n(DeviceConfigBackupView.class);

   private boolean componentRegistered;
   private List<DeviceConfigBackup> backupList = new ArrayList<>();
   private Map<Long, DeviceConfigBackup> contentCache = new HashMap<>();
   private SortableTableViewer viewer;
   private DeviceConfigBackupFilter filter = new DeviceConfigBackupFilter();
   private CTabFolder tabFolder;
   private StyledText runningConfigText;
   private StyledText startupConfigText;
   private DiffViewer diffViewer;
   private Combo diffTypeCombo;
   private LinkedList<Diff> currentDiffs;
   private Action actionBackupNow;
   private Action actionExportRunning;
   private Action actionExportStartup;
   private Action actionCopyToClipboard;

   /**
    * Default constructor
    */
   public DeviceConfigBackupView()
   {
      super(LocalizationHelper.getI18n(DeviceConfigBackupView.class).tr("Configuration Backup"), ResourceManager.getImageDescriptor("icons/object-views/backup.png"),
            "objects.device-config-backup", true);
      componentRegistered = Registry.getSession().isServerComponentRegistered("DEVBACKUP");
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return componentRegistered && (context instanceof AbstractNode) && ((AbstractNode)context).isRegisteredForConfigBackup();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 61;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      SashForm sashForm = new SashForm(parent, SWT.VERTICAL);

      String[] columnNames = { i18n.tr("Timestamp"), i18n.tr("Running Config"), i18n.tr("Startup Config"), i18n.tr("Status") };
      int[] columnWidths = { 200, 120, 120, 150 };
      viewer = new SortableTableViewer(sashForm, columnNames, columnWidths, COLUMN_TIMESTAMP, SWT.DOWN, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new DeviceConfigBackupLabelProvider(backupList));
      viewer.setComparator(new DeviceConfigBackupComparator());
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
      viewer.enableColumnReordering();
      WidgetHelper.restoreColumnOrder(viewer, getBaseId());
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnOrder(viewer, getBaseId());
         }
      });
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            onBackupSelectionChanged();
         }
      });

      tabFolder = new CTabFolder(sashForm, SWT.TOP | SWT.BORDER);
      tabFolder.setTabHeight(24);
      WidgetHelper.disableTabFolderSelectionBar(tabFolder);

      CTabItem runningTab = new CTabItem(tabFolder, SWT.NONE);
      runningTab.setText(i18n.tr("Running Config"));
      runningConfigText = new StyledText(tabFolder, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY);
      runningConfigText.setFont(JFaceResources.getTextFont());
      runningTab.setControl(runningConfigText);

      CTabItem startupTab = new CTabItem(tabFolder, SWT.NONE);
      startupTab.setText(i18n.tr("Startup Config"));
      startupConfigText = new StyledText(tabFolder, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY);
      startupConfigText.setFont(JFaceResources.getTextFont());
      startupTab.setControl(startupConfigText);

      CTabItem diffTab = new CTabItem(tabFolder, SWT.NONE);
      diffTab.setText(i18n.tr("Diff"));
      Composite diffComposite = new Composite(tabFolder, SWT.NONE);
      GridLayout diffLayout = new GridLayout();
      diffLayout.numColumns = 2;
      diffLayout.marginHeight = 0;
      diffLayout.marginWidth = 0;
      diffComposite.setLayout(diffLayout);

      Label diffLabel = new Label(diffComposite, SWT.NONE);
      diffLabel.setText(i18n.tr("Compare:"));

      diffTypeCombo = new Combo(diffComposite, SWT.DROP_DOWN | SWT.READ_ONLY);
      diffTypeCombo.add(i18n.tr("Running Config"));
      diffTypeCombo.add(i18n.tr("Startup Config"));
      diffTypeCombo.select(0);
      diffTypeCombo.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onBackupSelectionChanged();
         }
      });

      diffViewer = new DiffViewer(diffComposite, SWT.NONE);
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 2;
      diffViewer.setLayoutData(gd);
      diffTab.setControl(diffComposite);

      tabFolder.setSelection(0);

      sashForm.setWeights(new int[] { 40, 60 });

      createActions();
      createContextMenu();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionBackupNow = new Action(i18n.tr("&Backup Now"), SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            backupNow();
         }
      };

      actionExportRunning = new Action(i18n.tr("Export &Running Config")) {
         @Override
         public void run()
         {
            exportConfig(true);
         }
      };

      actionExportStartup = new Action(i18n.tr("Export &Startup Config")) {
         @Override
         public void run()
         {
            exportConfig(false);
         }
      };

      actionCopyToClipboard = new Action(i18n.tr("&Copy to Clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyToClipboard();
         }
      };
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager menuManager = new MenuManager();
      menuManager.setRemoveAllWhenShown(true);
      menuManager.addMenuListener((m) -> fillContextMenu(m));
      Menu menu = menuManager.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    *
    * @param manager menu manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (!selection.isEmpty())
      {
         manager.add(actionExportRunning);
         manager.add(actionExportStartup);
         manager.add(new Separator());
         manager.add(actionCopyToClipboard);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionBackupNow);
      manager.add(new Separator());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      Action resetAction = viewer.getResetColumnOrderAction();
      if (resetAction != null)
         manager.add(resetAction);
      super.fillLocalMenu(manager);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      backupList.clear();
      contentCache.clear();
      viewer.setInput(new Object[0]);
      clearContentTabs();
      if (object != null)
         refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      final long nodeId = getObjectId();
      new Job(i18n.tr("Loading device configuration backups"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<DeviceConfigBackup> backups = session.getDeviceConfigBackups(nodeId);
            Collections.sort(backups, (a, b) -> b.getTimestamp().compareTo(a.getTimestamp()));
            runInUIThread(() -> {
               if (viewer.getControl().isDisposed() || (nodeId != getObjectId()))
                  return;
               backupList.clear();
               backupList.addAll(backups);
               contentCache.clear();
               viewer.setInput(backupList.toArray());
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load device configuration backups");
         }
      }.start();
   }

   /**
    * Handle backup selection change in table viewer
    */
   private void onBackupSelectionChanged()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
      {
         clearContentTabs();
         return;
      }

      List<?> selectedList = selection.toList();
      if (selectedList.size() == 1)
      {
         DeviceConfigBackup backup = (DeviceConfigBackup)selectedList.get(0);
         loadAndDisplayBackup(backup, null);
      }
      else if (selectedList.size() == 2)
      {
         DeviceConfigBackup b1 = (DeviceConfigBackup)selectedList.get(0);
         DeviceConfigBackup b2 = (DeviceConfigBackup)selectedList.get(1);
         DeviceConfigBackup newer = b1.getTimestamp().after(b2.getTimestamp()) ? b1 : b2;
         DeviceConfigBackup older = (newer == b1) ? b2 : b1;
         loadAndDisplayBackup(newer, older);
      }
   }

   /**
    * Load backup content and display in tabs.
    *
    * @param primary primary (selected or newer) backup
    * @param secondary secondary (older) backup for diff comparison, or null to use predecessor
    */
   private void loadAndDisplayBackup(final DeviceConfigBackup primary, final DeviceConfigBackup secondary)
   {
      final long nodeId = getObjectId();
      final int activeTab = tabFolder.getSelectionIndex();


      DeviceConfigBackup cachedPrimary = contentCache.get(primary.getId());
      DeviceConfigBackup cachedSecondary = (secondary != null) ? contentCache.get(secondary.getId()) : null;

      if ((cachedPrimary != null) && (secondary == null || cachedSecondary != null))
      {
         displayBackupContent(cachedPrimary, cachedSecondary, activeTab);
         return;
      }

      new Job(i18n.tr("Loading backup content"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final DeviceConfigBackup fullPrimary = contentCache.containsKey(primary.getId()) ? contentCache.get(primary.getId()) : session.getDeviceConfigBackup(nodeId, primary.getId());
            DeviceConfigBackup fullSecondary = null;
            if (secondary != null)
            {
               fullSecondary = contentCache.containsKey(secondary.getId()) ? contentCache.get(secondary.getId()) : session.getDeviceConfigBackup(nodeId, secondary.getId());
            }
            else
            {
               int index = backupList.indexOf(primary);
               if ((index >= 0) && (index < backupList.size() - 1))
               {
                  DeviceConfigBackup predecessor = backupList.get(index + 1);
                  fullSecondary = contentCache.containsKey(predecessor.getId()) ? contentCache.get(predecessor.getId()) : session.getDeviceConfigBackup(nodeId, predecessor.getId());
               }
            }

            contentCache.put(fullPrimary.getId(), fullPrimary);
            if (fullSecondary != null)
               contentCache.put(fullSecondary.getId(), fullSecondary);

            final DeviceConfigBackup displayPrimary = fullPrimary;
            final DeviceConfigBackup displaySecondary = fullSecondary;
            runInUIThread(() -> {
               if (viewer.getControl().isDisposed() || (nodeId != getObjectId()))
                  return;
               displayBackupContent(displayPrimary, displaySecondary, activeTab);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load backup content");
         }
      }.start();
   }

   /**
    * Display backup content in tabs.
    *
    * @param primary primary backup with full content
    * @param secondary secondary backup for diff (may be null)
    * @param activeTab tab index to restore
    */
   private void displayBackupContent(DeviceConfigBackup primary, DeviceConfigBackup secondary, int activeTab)
   {
      String runningText = getConfigText(primary.getRunningConfig());
      String startupText = getConfigText(primary.getStartupConfig());

      runningConfigText.setText(runningText);
      startupConfigText.setText(startupText);

      if (secondary != null)
      {
         boolean compareRunning = (diffTypeCombo.getSelectionIndex() == 0);
         String oldText = compareRunning ? getConfigText(secondary.getRunningConfig()) : getConfigText(secondary.getStartupConfig());
         String newText = compareRunning ? runningText : startupText;

         DiffMatchPatch dmp = new DiffMatchPatch();
         currentDiffs = dmp.diff_main(oldText, newText);
         dmp.diff_cleanupSemantic(currentDiffs);
         diffViewer.setContent(currentDiffs);
      }
      else
      {
         currentDiffs = null;
         diffViewer.setContent(new LinkedList<Diff>());
      }

      if (activeTab >= 0 && activeTab < tabFolder.getItemCount())
         tabFolder.setSelection(activeTab);
   }

   /**
    * Get config content as string.
    *
    * @param data raw config data
    * @return config text or empty string if data is null
    */
   private static String getConfigText(byte[] data)
   {
      return (data != null) ? new String(data) : "";
   }

   /**
    * Clear all content tabs
    */
   private void clearContentTabs()
   {
      runningConfigText.setText("");
      startupConfigText.setText("");
      currentDiffs = null;
      diffViewer.setContent(new LinkedList<Diff>());
   }

   /**
    * Start backup now
    */
   private void backupNow()
   {
      final long nodeId = getObjectId();
      new Job(i18n.tr("Starting device configuration backup"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.startDeviceConfigBackup(nodeId);
            runInUIThread(() -> addMessage(MessageArea.INFORMATION, i18n.tr("Backup job started")));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot start device configuration backup");
         }
      }.start();
   }

   /**
    * Export configuration to file.
    *
    * @param running true to export running config, false for startup config
    */
   private void exportConfig(boolean running)
   {
      int activeTab = tabFolder.getSelectionIndex();
      String text;
      String type;
      if (activeTab == TAB_DIFF && currentDiffs != null)
      {
         text = reconstructDiffText();
         type = "diff";
      }
      else if (running)
      {
         text = runningConfigText.getText();
         type = "running";
      }
      else
      {
         text = startupConfigText.getText();
         type = "startup";
      }

      if (text.isEmpty())
         return;

      SimpleDateFormat df = new SimpleDateFormat("yyyyMMdd_HHmmss");
      String fileName = getObjectName() + "_" + type + "_" + df.format(new Date()) + ".txt";
      WidgetHelper.saveTextToFile(this, fileName, text);
   }

   /**
    * Copy currently visible content to clipboard
    */
   private void copyToClipboard()
   {
      int activeTab = tabFolder.getSelectionIndex();
      String text;
      switch(activeTab)
      {
         case TAB_RUNNING:
            text = runningConfigText.getText();
            break;
         case TAB_STARTUP:
            text = startupConfigText.getText();
            break;
         case TAB_DIFF:
            text = (currentDiffs != null) ? reconstructDiffText() : "";
            break;
         default:
            text = "";
            break;
      }
      if (!text.isEmpty())
         WidgetHelper.copyToClipboard(text);
   }

   /**
    * Reconstruct plain text from current diff
    *
    * @return concatenated text from all diff entries
    */
   private String reconstructDiffText()
   {
      if (currentDiffs == null)
         return "";
      StringBuilder sb = new StringBuilder();
      for(Diff diff : currentDiffs)
         sb.append(diff.text);
      return sb.toString();
   }
}
