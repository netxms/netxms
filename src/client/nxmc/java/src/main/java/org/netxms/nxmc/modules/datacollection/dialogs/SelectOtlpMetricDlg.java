/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.OTLPMetric;
import org.netxms.client.constants.DataType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting an OTLP metric observed by the server for a node.
 */
public class SelectOtlpMetricDlg extends Dialog implements IParameterSelectionDialog
{
   private static final String CONFIG_PREFIX = "SelectOtlpMetricDlg";

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_ATTRIBUTES = 2;

   private final I18n i18n = LocalizationHelper.getI18n(SelectOtlpMetricDlg.class);

   private long nodeId;
   private AbstractObject object;
   private Text filterText;
   private SortableTableViewer viewer;
   private MetricFilter filter;
   private OTLPMetric selectedMetric;

   /**
    * @param parentShell parent shell
    * @param nodeId ID of the node whose observed metrics will be listed
    */
   public SelectOtlpMetricDlg(Shell parentShell, long nodeId)
   {
      super(parentShell);
      this.nodeId = nodeId;
      this.object = Registry.getSession().findObjectById(nodeId);
      setShellStyle(getShellStyle() | SWT.RESIZE);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Select OTLP Metric"));
      PreferenceStore settings = PreferenceStore.getInstance();
      try
      {
         newShell.setSize(settings.getAsPoint(CONFIG_PREFIX + ".size", 600, 400));
      }
      catch(NumberFormatException e)
      {
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Observed metrics"));

      filterText = new Text(dialogArea, SWT.BORDER);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      filterText.setLayoutData(gd);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filter.setFilter(filterText.getText());
            viewer.refresh(false);
         }
      });

      final String[] names = { i18n.tr("Name"), i18n.tr("Type"), i18n.tr("Attribute keys") };
      final int[] widths = { 300, 100, 200 };
      viewer = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.BORDER);
      WidgetHelper.restoreTableViewerSettings(viewer, CONFIG_PREFIX + ".viewer");
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new MetricLabelProvider());

      filter = new MetricFilter();
      viewer.addFilter(filter);

      viewer.getTable().addMouseListener(new MouseAdapter() {
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            okPressed();
         }
      });

      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, CONFIG_PREFIX + ".viewer");
         }
      });

      gd = new GridData();
      gd.heightHint = 250;
      gd.grabExcessVerticalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      viewer.getControl().setLayoutData(gd);

      fillList();

      return dialogArea;
   }

   /**
    * Fill list with metrics observed for the node
    */
   private void fillList()
   {
      final NXCSession session = Registry.getSession();
      new Job(String.format(i18n.tr("Reading OTLP metrics for %s"), (object != null) ? object.getObjectName() : Long.toString(nodeId)), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<OTLPMetric> metrics = session.getOTLPMetrics(nodeId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!viewer.getControl().isDisposed())
                     viewer.setInput(metrics.toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read list of OTLP metrics");
         }
      }.start();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select a metric from the list"));
         return;
      }
      selectedMetric = (OTLPMetric)selection.getFirstElement();
      saveSettings();
      super.okPressed();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
   @Override
   protected void cancelPressed()
   {
      saveSettings();
      super.cancelPressed();
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      PreferenceStore.getInstance().set(CONFIG_PREFIX + ".size", size);
   }

   /**
    * @return selected metric, or null if nothing was selected
    */
   public OTLPMetric getSelectedMetric()
   {
      return selectedMetric;
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.dialogs.IParameterSelectionDialog#getParameterName()
    */
   @Override
   public String getParameterName()
   {
      return (selectedMetric != null) ? selectedMetric.getName() : "";
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.dialogs.IParameterSelectionDialog#getParameterDescription()
    */
   @Override
   public String getParameterDescription()
   {
      return (selectedMetric != null) ? selectedMetric.getName() : "";
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.dialogs.IParameterSelectionDialog#getParameterDataType()
    */
   @Override
   public DataType getParameterDataType()
   {
      return DataType.FLOAT;
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.dialogs.IParameterSelectionDialog#getInstanceColumn()
    */
   @Override
   public String getInstanceColumn()
   {
      return "";
   }

   /**
    * Label provider for metric list
    */
   private static class MetricLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         OTLPMetric metric = (OTLPMetric)element;
         switch(columnIndex)
         {
            case COLUMN_NAME:
               return metric.getName();
            case COLUMN_TYPE:
               return metric.getTypeName();
            case COLUMN_ATTRIBUTES:
               return String.join(", ", metric.getAttributeKeys());
            default:
               return "";
         }
      }
   }

   /**
    * Filter matching metric name, type, or attribute keys against the filter text
    */
   private static class MetricFilter extends ViewerFilter
   {
      private String filterString = null;

      @Override
      public boolean select(Viewer viewer, Object parentElement, Object element)
      {
         if ((filterString == null) || filterString.isEmpty())
            return true;
         OTLPMetric metric = (OTLPMetric)element;
         if (metric.getName().toLowerCase().contains(filterString))
            return true;
         if (metric.getTypeName().toLowerCase().contains(filterString))
            return true;
         for(String key : metric.getAttributeKeys())
         {
            if (key.toLowerCase().contains(filterString))
               return true;
         }
         return false;
      }

      public void setFilter(String filterString)
      {
         this.filterString = (filterString != null) ? filterString.toLowerCase() : null;
      }
   }
}
