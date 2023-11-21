/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2023 Raden Solutions
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

import java.io.ByteArrayInputStream;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import java.util.Date;
import javax.xml.XMLConstants;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.stream.StreamSource;
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

/**
 * Dialog for displaying windows event log record details
 */
public class WindowsEventLogRecordDetailsDialog extends Dialog
{
   private static final Logger logger = LoggerFactory.getLogger(WindowsEventLogRecordDetailsDialog.class);

   private final I18n i18n = LocalizationHelper.getI18n(WindowsEventLogRecordDetailsDialog.class);

   private LogRecordDetails data;
   private CTabFolder tabFolder;
   private TableRow record;
   private Log logHandle;

   /**
    * Create dialog
    * 
    * @param parentShell parent shell
    * @param data audit log record details
    * @param logHandle 
    * @param record 
    */
   public WindowsEventLogRecordDetailsDialog(Shell parentShell, LogRecordDetails data, TableRow record, Log logHandle)
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
      newShell.setText(i18n.tr("Windows Event Log Record Details"));
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
      createDetailsTab(data.getValue("raw_data"));
      tabFolder.setSelection(0);

      return dialogArea;
   }
   
   /**
    * Format XML document for display.
    *
    * @param xmlDocument original XML document
    * @return formatted XML document
    */
   private static String formatXML(String xmlDocument)
   {
      String result;
      try
      {
         StringWriter stringWriter = new StringWriter();
         StreamResult xmlOutput = new StreamResult(stringWriter);
         TransformerFactory transformerFactory = TransformerFactory.newInstance();
         transformerFactory.setAttribute("indent-number", 3);
         transformerFactory.setAttribute(XMLConstants.ACCESS_EXTERNAL_DTD, "");
         transformerFactory.setAttribute(XMLConstants.ACCESS_EXTERNAL_STYLESHEET, "");
         Transformer transformer = transformerFactory.newTransformer();
         transformer.setOutputProperty(OutputKeys.INDENT, "yes");
         transformer.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "yes");
         transformer.transform(new StreamSource(new ByteArrayInputStream(xmlDocument.getBytes(StandardCharsets.UTF_8))), xmlOutput);
         result = xmlOutput.getWriter().toString();
      }
      catch(Exception e)
      {
         logger.warn("Cannot format XML", e);
         logger.debug("Source XML: " + xmlDocument);
         result = xmlDocument;
      }
      return result;
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
      
      Text textControl = new Text(dialogArea, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY | SWT.WRAP);
      textControl.setFont(JFaceResources.getTextFont());
      textControl.setText(record.get(logHandle.getColumnIndex("message")).getValue());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      gd.widthHint = 300;
      gd.heightHint = 500;
      textControl.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            "Log Name", record.get(logHandle.getColumnIndex("log_name")).getValue(), gd);  

      long timestamp = record.get(logHandle.getColumnIndex("origin_timestamp")).getValueAsLong();
      Date date = new Date(timestamp * 1000);
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            "Logged", DateFormatFactory.getDateTimeFormat().format(date), WidgetHelper.DEFAULT_LAYOUT_DATA);

      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            "Source", record.get(logHandle.getColumnIndex("event_source")).getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);

      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            "Event ID", record.get(logHandle.getColumnIndex("event_code")).getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);

      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            "Level", record.get(logHandle.getColumnIndex("event_severity")).getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);

      tab.setControl(dialogArea);
   }

   /**
    * Create details tab
    * 
    * @param xmlDocument event's XML
    */
   private void createDetailsTab(String xmlDocument)
   {
      CTabItem tab = createTab(i18n.tr("Details"));
      Text textControl = new Text(tabFolder, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY | SWT.WRAP);
      textControl.setFont(JFaceResources.getTextFont());
      textControl.setText(formatXML(xmlDocument));
      tab.setControl(textControl);
   }

   /**
    * Create tab
    * 
    * @param name
    * @param imageName
    * @return
    */
   private CTabItem createTab(String name)
   {
      CTabItem tab = new CTabItem(tabFolder, SWT.NONE);
      tab.setText(name);
      return tab;
   }
}
