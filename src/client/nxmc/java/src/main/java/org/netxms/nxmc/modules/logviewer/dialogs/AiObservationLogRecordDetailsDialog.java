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
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonElement;
import com.google.gson.JsonParser;

/**
 * Dialog for displaying AI operator observation details
 */
public class AiObservationLogRecordDetailsDialog extends Dialog
{
   private static final Logger logger = LoggerFactory.getLogger(AiObservationLogRecordDetailsDialog.class);

   private final I18n i18n = LocalizationHelper.getI18n(AiObservationLogRecordDetailsDialog.class);

   private LogRecordDetails data;
   private TableRow record;
   private Log logHandle;
   private CTabFolder tabFolder;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param data record details (contains observation body and references)
    * @param record original log record row
    * @param logHandle log handle
    */
   public AiObservationLogRecordDetailsDialog(Shell parentShell, LogRecordDetails data, TableRow record, Log logHandle)
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
      newShell.setText(i18n.tr("AI Observation Details"));
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

      createGeneralTab();
      createReferencesTab(data.getValue("refs"));
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
    * Get column value from the log record row as long integer.
    *
    * @param column column name
    * @return value or 0 if column is not present or cannot be parsed
    */
   private long getValueAsLong(String column)
   {
      int index = logHandle.getColumnIndex(column);
      return (index >= 0) ? record.get(index).getValueAsLong() : 0;
   }

   /**
    * Create tab with general observation information
    */
   private void createGeneralTab()
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

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Title"), getValue("title"), gd);

      Text bodyControl = new Text(dialogArea, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY | SWT.WRAP);
      bodyControl.setFont(JFaceResources.getTextFont());
      String body = data.getValue("body");
      bodyControl.setText((body != null) ? body : "");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      gd.widthHint = 300;
      gd.heightHint = 300;
      bodyControl.setLayoutData(gd);

      long timestamp = getValueAsLong("observation_timestamp");
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Timestamp"), (timestamp != 0) ? DateFormatFactory.getDateTimeFormat().format(new Date(timestamp * 1000)) : "",
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Severity"), StatusDisplayInfo.getStatusText((int)getValueAsLong("severity")), WidgetHelper.DEFAULT_LAYOUT_DATA);
      long objectId = getValueAsLong("object_id");
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Object"), (objectId != 0) ? Registry.getSession().getObjectName(objectId) : "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Operator instance"), getValue("instance_id"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("State"), getStateText(), WidgetHelper.DEFAULT_LAYOUT_DATA);

      tab.setControl(dialogArea);
   }

   /**
    * Get display text for observation state.
    *
    * @return display text for observation state
    */
   private String getStateText()
   {
      switch((int)getValueAsLong("state"))
      {
         case 0:
            return i18n.tr("New");
         case 1:
            return i18n.tr("Acknowledged");
         case 2:
            return i18n.tr("Dismissed");
         default:
            return getValue("state");
      }
   }

   /**
    * Create tab with supporting references
    *
    * @param refs JSON document with references to alarms, events, and other entities
    */
   private void createReferencesTab(String refs)
   {
      CTabItem tab = createTab(i18n.tr("References"));
      Text textControl = new Text(tabFolder, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY);
      textControl.setFont(JFaceResources.getTextFont());
      textControl.setText(formatJSON(refs));
      tab.setControl(textControl);
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
