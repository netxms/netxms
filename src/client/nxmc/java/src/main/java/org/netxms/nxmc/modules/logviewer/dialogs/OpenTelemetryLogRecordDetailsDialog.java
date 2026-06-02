/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.nxmc.modules.logviewer.dialogs;

import java.util.Date;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.TableRow;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogRecordDetails;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonElement;
import com.google.gson.JsonParser;

/**
 * Dialog for displaying OpenTelemetry log record details
 */
public class OpenTelemetryLogRecordDetailsDialog extends Dialog
{
   private static final Logger logger = LoggerFactory.getLogger(OpenTelemetryLogRecordDetailsDialog.class);

   private final I18n i18n = LocalizationHelper.getI18n(OpenTelemetryLogRecordDetailsDialog.class);

   private LogRecordDetails data;
   private TableRow record;
   private Log logHandle;
   private CTabFolder tabFolder;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param data record details (contains detail columns such as attributes)
    * @param record original log record row
    * @param logHandle log handle
    */
   public OpenTelemetryLogRecordDetailsDialog(Shell parentShell, LogRecordDetails data, TableRow record, Log logHandle)
   {
      super(parentShell);
      this.data = data;
      this.record = record;
      this.logHandle = logHandle;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("OpenTelemetry Log Record Details"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#isResizable()
    */
   @Override
   protected boolean isResizable()
   {
      return true;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.OK_ID, i18n.tr("Close"), true);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      dialogArea.setLayout(layout);

      tabFolder = new CTabFolder(dialogArea, SWT.BORDER);
      tabFolder.setTabHeight(24);
      WidgetHelper.disableTabFolderSelectionBar(tabFolder);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 800;
      gd.heightHint = 500;
      tabFolder.setLayoutData(gd);

      createInformationTab();
      createAttributesTab(data.getValue("attributes"));
      tabFolder.setSelection(0);

      return dialogArea;
   }

   /**
    * Get column value from the log record row.
    *
    * @param column column name
    * @return value or empty string if column is not present
    */
   private String getValue(String column)
   {
      int index = logHandle.getColumnIndex(column);
      return (index >= 0) ? record.get(index).getValue() : "";
   }

   /**
    * Format an epoch-millisecond timestamp column for display.
    *
    * @param column column name
    * @return formatted date/time, or empty string if not set
    */
   private String formatTimestamp(String column)
   {
      int index = logHandle.getColumnIndex(column);
      if (index < 0)
         return "";
      long timestamp = record.get(index).getValueAsLong();
      if (timestamp == 0)
         return "";
      return DateFormatFactory.getDateTimeFormat().format(new Date(timestamp));
   }

   /**
    * Format compact JSON document for display.
    *
    * @param json original JSON document
    * @return pretty-printed JSON document
    */
   private static String formatJSON(String json)
   {
      if ((json == null) || json.isEmpty())
         return "";
      try
      {
         JsonElement element = JsonParser.parseString(json);
         return new GsonBuilder().setPrettyPrinting().create().toJson(element);
      }
      catch(Exception e)
      {
         logger.warn("Cannot format JSON", e);
         return json;
      }
   }

   /**
    * Create tab with main information
    */
   private void createInformationTab()
   {
      CTabItem tab = createTab(i18n.tr("General"));

      Composite dialogArea = new Composite(tabFolder, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      Text bodyControl = new Text(dialogArea, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY | SWT.WRAP);
      bodyControl.setFont(JFaceResources.getTextFont());
      bodyControl.setText(getValue("body"));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      gd.widthHint = 300;
      gd.heightHint = 300;
      bodyControl.setLayoutData(gd);

      String severity = getValue("severity_number");
      String severityText = getValue("severity_text");
      if (!severityText.isEmpty())
         severity = severity + " (" + severityText + ")";

      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Severity"), severity, WidgetHelper.DEFAULT_LAYOUT_DATA);
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Service"), getValue("service_name"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Scope"), getValue("scope_name"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Event time"), formatTimestamp("origin_timestamp"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Observed time"), formatTimestamp("observed_timestamp"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Trace ID"), getValue("trace_id"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Span ID"), getValue("span_id"), WidgetHelper.DEFAULT_LAYOUT_DATA);

      tab.setControl(dialogArea);
   }

   /**
    * Create tab with attributes
    *
    * @param attributes JSON document with flattened resource and record attributes
    */
   private void createAttributesTab(String attributes)
   {
      CTabItem tab = createTab(i18n.tr("Attributes"));
      Text textControl = new Text(tabFolder, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY);
      textControl.setFont(JFaceResources.getTextFont());
      textControl.setText(formatJSON(attributes));
      tab.setControl(textControl);
   }

   /**
    * Create tab
    *
    * @param name tab name
    * @return created tab item
    */
   private CTabItem createTab(String name)
   {
      CTabItem tab = new CTabItem(tabFolder, SWT.NONE);
      tab.setText(name);
      return tab;
   }
}
