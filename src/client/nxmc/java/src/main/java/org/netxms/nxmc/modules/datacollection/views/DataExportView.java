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
package org.netxms.nxmc.modules.datacollection.views;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.BackgroundTaskInfo;
import org.netxms.client.NXCSession;
import org.netxms.client.TimePeriod;
import org.netxms.client.constants.DciTier;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.datacollection.DataExportMetric;
import org.netxms.client.datacollection.DataExportRequest;
import org.netxms.client.datacollection.DataExportResult;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.TimePeriodSelector;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.propertypages.OtherOptions;
import org.netxms.nxmc.modules.filemanager.FileDownloadHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Export of tagged DCI history (CSV in ZIP archive) for external analysis
 */
public class DataExportView extends View
{
   private final I18n i18n = LocalizationHelper.getI18n(DataExportView.class);

   private static final int COLUMN_OWNER = 0;
   private static final int COLUMN_TAG = 1;
   private static final int COLUMN_NAME = 2;
   private static final int COLUMN_DESCRIPTION = 3;
   private static final int COLUMN_UNIT = 4;
   private static final int COLUMN_ORIGIN = 5;
   private static final int COLUMN_TIER = 6;

   private static final int DOWNLOAD_TIMEOUT = 3600000;

   private final String[] columnNames = { i18n.tr("Owner"), i18n.tr("Tag"), i18n.tr("Name"), i18n.tr("Description"), i18n.tr("Unit"), i18n.tr("Origin"), i18n.tr("Tier") };
   private static final int[] COLUMN_WIDTHS = { 200, 150, 200, 300, 80, 100, 80 };

   private long initialRootObjectId;
   private ObjectSelector objectSelector;
   private TimePeriodSelector timePeriodSelector;
   private Map<String, Button> tagGroupButtons = new LinkedHashMap<>();
   private LabeledText additionalPatterns;
   private LabeledCombo tierSelector;
   private Button anonymizeButton;
   private Button keepMappingButton;
   private Button previewButton;
   private Button exportButton;
   private Label statusLabel;
   private SortableTableViewer viewer;
   private List<DataExportMetric> previewedMetrics = new ArrayList<>();
   private volatile boolean exportInProgress = false;
   private Action actionPreview;
   private Action actionExport;
   private Action actionClearPreview;

   /**
    * Create view without preselected root object
    */
   public DataExportView()
   {
      this(0);
   }

   /**
    * Create view with preselected root object (for "Export data..." object context menu).
    *
    * @param rootObjectId initial root object ID, 0 for none
    */
   public DataExportView(long rootObjectId)
   {
      super(LocalizationHelper.getI18n(DataExportView.class).tr("Data Export"), ResourceManager.getImageDescriptor("icons/export-data.png"), "tools.data-export", false);
      initialRootObjectId = rootObjectId;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      initialRootObjectId = ((DataExportView)view).objectSelector.getObjectId();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      parent.setLayout(layout);

      Composite form = new Composite(parent, SWT.NONE);
      GridLayout formLayout = new GridLayout(2, false);
      form.setLayout(formLayout);
      form.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Composite selectionArea = new Composite(form, SWT.NONE);
      GridLayout selectionLayout = new GridLayout();
      selectionLayout.marginHeight = 0;
      selectionLayout.marginWidth = 0;
      selectionArea.setLayout(selectionLayout);
      selectionArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      objectSelector = new ObjectSelector(selectionArea, SWT.NONE, true);
      objectSelector.setLabel(i18n.tr("Root object"));
      objectSelector.removeSelectionClassFilter();   // Any object can be export root: containers, subnets, zones, facilities, nodes
      objectSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      if (initialRootObjectId != 0)
         objectSelector.setObjectId(initialRootObjectId);

      Group tagGroup = new Group(selectionArea, SWT.NONE);
      tagGroup.setText(i18n.tr("Tag groups"));
      tagGroup.setLayout(new GridLayout(4, false));
      tagGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      for(String prefix : getTagGroupPrefixes())
      {
         Button button = new Button(tagGroup, SWT.CHECK);
         button.setText(prefix + ".*");
         button.setSelection(!prefix.equals("iface"));
         tagGroupButtons.put(prefix, button);
      }

      additionalPatterns = new LabeledText(selectionArea, SWT.NONE);
      additionalPatterns.setLabel(i18n.tr("Additional tag patterns (comma-separated, * and ? wildcards)"));
      additionalPatterns.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Composite optionsArea = new Composite(form, SWT.NONE);
      GridLayout optionsLayout = new GridLayout();
      optionsLayout.marginHeight = 0;
      optionsLayout.marginWidth = 0;
      optionsArea.setLayout(optionsLayout);
      optionsArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false));

      Group periodGroup = new Group(optionsArea, SWT.NONE);
      periodGroup.setText(i18n.tr("Time period"));
      periodGroup.setLayout(new GridLayout());
      periodGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      timePeriodSelector = new TimePeriodSelector(periodGroup, SWT.NONE, new TimePeriod(TimeFrameType.BACK_FROM_NOW, 1, TimeUnit.DAY, null, null));
      timePeriodSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      tierSelector = new LabeledCombo(optionsArea, SWT.NONE);
      tierSelector.setLabel(i18n.tr("History tier"));
      tierSelector.add(i18n.tr("Raw samples"));
      tierSelector.add(i18n.tr("Hourly aggregates"));
      tierSelector.add(i18n.tr("Daily aggregates"));
      tierSelector.select(0);
      tierSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      anonymizeButton = new Button(optionsArea, SWT.CHECK);
      anonymizeButton.setText(i18n.tr("Anonymize (replace names with pseudonyms)"));
      anonymizeButton.addListener(SWT.Selection, e -> keepMappingButton.setEnabled(anonymizeButton.getSelection()));

      keepMappingButton = new Button(optionsArea, SWT.CHECK);
      keepMappingButton.setText(i18n.tr("Save local pseudonym mapping file"));
      keepMappingButton.setEnabled(false);

      Composite buttonArea = new Composite(optionsArea, SWT.NONE);
      GridLayout buttonLayout = new GridLayout(2, true);
      buttonLayout.marginHeight = 0;
      buttonLayout.marginWidth = 0;
      buttonArea.setLayout(buttonLayout);
      buttonArea.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, true));

      previewButton = new Button(buttonArea, SWT.PUSH);
      previewButton.setText(i18n.tr("Preview"));
      previewButton.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      previewButton.addListener(SWT.Selection, e -> previewMetrics());

      exportButton = new Button(buttonArea, SWT.PUSH);
      exportButton.setImage(SharedIcons.IMG_EXECUTE);
      exportButton.setText(i18n.tr("Export"));
      exportButton.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      exportButton.addListener(SWT.Selection, e -> exportData());

      new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      statusLabel = new Label(parent, SWT.NONE);
      statusLabel.setFont(JFaceResources.getBannerFont());
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.verticalIndent = 8;
      gd.horizontalIndent = 4;
      statusLabel.setLayoutData(gd);
      statusLabel.setText(i18n.tr("Press \"Preview\" to list metrics the export would contain; unchecked metrics are excluded from export"));

      Label separator = new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.verticalIndent = 8;
      separator.setLayoutData(gd);

      viewer = new SortableTableViewer(parent, columnNames, COLUMN_WIDTHS, COLUMN_OWNER, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI | SWT.CHECK);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new MetricLabelProvider());
      viewer.setInput(previewedMetrics);
      viewer.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      createActions();
   }

   /**
    * Build view actions
    */
   private void createActions()
   {
      actionPreview = new Action(i18n.tr("&Preview"), SharedIcons.FIND) {
         @Override
         public void run()
         {
            previewMetrics();
         }
      };

      actionExport = new Action(i18n.tr("&Export"), SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            exportData();
         }
      };
      addKeyBinding("F9", actionExport);

      actionClearPreview = new Action(i18n.tr("&Clear preview"), SharedIcons.CLEAR) {
         @Override
         public void run()
         {
            previewedMetrics.clear();
            viewer.refresh();
            statusLabel.setText(i18n.tr("Preview cleared; export will use tag patterns"));
         }
      };
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionPreview);
      manager.add(actionExport);
      manager.add(new Separator());
      manager.add(actionClearPreview);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionPreview);
      manager.add(actionExport);
      manager.add(new Separator());
      manager.add(actionClearPreview);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      objectSelector.setFocus();
   }

   /**
    * Get tag group prefixes (first segment of tags in the vocabulary, in vocabulary order)
    */
   private static Set<String> getTagGroupPrefixes()
   {
      Set<String> prefixes = new LinkedHashSet<>();
      for(String[] entry : OtherOptions.TAGS)
      {
         String tag = entry[0];
         int index = tag.indexOf('.');
         prefixes.add((index > 0) ? tag.substring(0, index) : tag);
      }
      return prefixes;
   }

   /**
    * Collect tag patterns from form
    */
   private List<String> getTagPatterns()
   {
      List<String> patterns = new ArrayList<>();
      for(Map.Entry<String, Button> e : tagGroupButtons.entrySet())
      {
         if (e.getValue().getSelection())
            patterns.add(e.getKey() + ".*");
      }
      for(String p : additionalPatterns.getText().split(","))
      {
         p = p.trim();
         if (!p.isEmpty())
            patterns.add(p);
      }
      return patterns;
   }

   /**
    * Validate selection part of the form. Returns tag patterns or null if validation failed.
    */
   private List<String> validateSelection()
   {
      clearMessages();
      if (objectSelector.getObjectId() == 0)
      {
         addMessage(MessageArea.WARNING, i18n.tr("Root object is not selected"));
         return null;
      }
      List<String> patterns = getTagPatterns();
      if (patterns.isEmpty())
      {
         addMessage(MessageArea.WARNING, i18n.tr("No tag patterns selected"));
         return null;
      }
      return patterns;
   }

   /**
    * Find metrics matching current selection and show them in table
    */
   private void previewMetrics()
   {
      final List<String> patterns = validateSelection();
      if (patterns == null)
         return;

      final DataExportRequest request = buildRequest(patterns);
      request.setDciList(new ArrayList<>());   // Preview always resolves by tag patterns
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Searching for tagged metrics"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<DataExportMetric> result = session.getDataExportMetrics(request);
            runInUIThread(() -> {
               previewedMetrics.clear();
               previewedMetrics.addAll(result);
               viewer.refresh();
               for(TableItem item : viewer.getTable().getItems())
                  item.setChecked(true);
               statusLabel.setText(i18n.tr("{0} metric(s) found; unchecked metrics are excluded from export", result.size()));
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot search for tagged metrics");
         }
      }.start();
   }

   /**
    * Build export request from form state
    */
   private DataExportRequest buildRequest(List<String> patterns)
   {
      DataExportRequest request = new DataExportRequest();
      request.setRootObjectId(objectSelector.getObjectId());
      request.setTagPatterns(patterns);

      if (!previewedMetrics.isEmpty())
      {
         List<DataExportRequest.DciReference> dciList = new ArrayList<>();
         for(TableItem item : viewer.getTable().getItems())
         {
            if (item.getChecked() && (item.getData() instanceof DataExportMetric))
            {
               DataExportMetric m = (DataExportMetric)item.getData();
               dciList.add(new DataExportRequest.DciReference(m.getOwnerId(), m.getDciId()));
            }
         }
         request.setDciList(dciList);
      }

      TimePeriod period = timePeriodSelector.getTimePeriod();
      request.setTimeFrom(period.getPeriodStart());
      request.setTimeTo(period.getPeriodEnd());

      switch(tierSelector.getSelectionIndex())
      {
         case 1:
            request.setTier(DciTier.HOURLY);
            break;
         case 2:
            request.setTier(DciTier.DAILY);
            break;
         default:
            request.setTier(DciTier.RAW);
            break;
      }

      request.setAnonymize(anonymizeButton.getSelection());
      request.setKeepMapping(anonymizeButton.getSelection() && keepMappingButton.getSelection());
      return request;
   }

   /**
    * Run export and offer produced files for saving
    */
   private void exportData()
   {
      if (exportInProgress)
         return;

      final List<String> patterns = validateSelection();
      if (patterns == null)
         return;

      final DataExportRequest request = buildRequest(patterns);
      if ((request.getTimeFrom() == null) || (request.getTimeTo() == null) || !request.getTimeTo().after(request.getTimeFrom()))
      {
         addMessage(MessageArea.WARNING, i18n.tr("Invalid time period"));
         return;
      }
      if (!previewedMetrics.isEmpty() && request.getDciList().isEmpty())
      {
         addMessage(MessageArea.WARNING, i18n.tr("All previewed metrics are unchecked"));
         return;
      }

      final String baseName = "netxms-export-" + new SimpleDateFormat("yyyyMMdd-HHmmss").format(new Date());
      final NXCSession session = Registry.getSession();
      exportInProgress = true;
      exportButton.setEnabled(false);
      statusLabel.setText(i18n.tr("Export in progress..."));

      new Job(i18n.tr("Exporting DCI data"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            long taskId = session.startDataExport(request);

            monitor.beginTask(i18n.tr("Exporting DCI data"), 100);
            int reportedProgress = 0;
            while(true)
            {
               BackgroundTaskInfo taskInfo = session.getBackgroundTaskInfo(taskId);
               if (taskInfo.getProgress() > reportedProgress)
               {
                  monitor.worked(taskInfo.getProgress() - reportedProgress);
                  reportedProgress = taskInfo.getProgress();
               }
               if (taskInfo.isFinished())
               {
                  if (taskInfo.isFailed())
                     throw new Exception(taskInfo.getFailureReason());
                  break;
               }
               Thread.sleep(1000);
            }
            monitor.done();

            final DataExportResult archive = session.downloadDataExportFile(taskId, false, DOWNLOAD_TIMEOUT);
            final DataExportResult mapping = request.isKeepMapping() ? session.downloadDataExportFile(taskId, true, DOWNLOAD_TIMEOUT) : null;

            getDisplay().syncExec(() -> {
               try
               {
                  boolean saved = FileDownloadHelper.saveLocalFile(getWindow().getShell(), archive.getFile(), baseName + ".zip");
                  if (saved && (mapping != null))
                     FileDownloadHelper.saveLocalFile(getWindow().getShell(), mapping.getFile(), baseName + "-mapping.csv");
                  statusLabel.setText(i18n.tr("Export completed: {0} metric(s), {1} sample(s), {2} bytes", archive.getMetricCount(), archive.getSampleCount(), archive.getFileSize()));
                  if (saved)
                     addMessage(MessageArea.INFORMATION, i18n.tr("Export saved as {0}", baseName + ".zip"));
               }
               catch(Exception e)
               {
                  addMessage(MessageArea.ERROR, i18n.tr("Cannot save export file: {0}", e.getLocalizedMessage()));
               }
               archive.getFile().delete();
               if (mapping != null)
                  mapping.getFile().delete();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot export DCI data");
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> {
               exportInProgress = false;
               if (!exportButton.isDisposed())
                  exportButton.setEnabled(true);
               if (!statusLabel.isDisposed() && statusLabel.getText().equals(i18n.tr("Export in progress...")))
                  statusLabel.setText(i18n.tr("Export failed"));
            });
         }
      }.start();
   }

   /**
    * Label provider for previewed metrics
    */
   private class MetricLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      private final NXCSession session = Registry.getSession();

      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         DataExportMetric m = (DataExportMetric)element;
         switch(columnIndex)
         {
            case COLUMN_OWNER:
               AbstractObject owner = session.findObjectById(m.getOwnerId());
               return (owner != null) ? owner.getObjectName() : ("[" + m.getOwnerId() + "]");
            case COLUMN_TAG:
               return m.getTag();
            case COLUMN_NAME:
               return m.getName();
            case COLUMN_DESCRIPTION:
               return m.getDescription();
            case COLUMN_UNIT:
               return m.getUnit();
            case COLUMN_ORIGIN:
               return m.getOrigin().toString();
            case COLUMN_TIER:
               return m.getTier().toString();
         }
         return null;
      }
   }
}
