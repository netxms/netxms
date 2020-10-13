/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Solutions
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
package org.netxms.ui.eclipse.logviewer.dialogs;

import java.io.ByteArrayInputStream;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import java.util.Date;
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
import org.netxms.client.TableRow;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogRecordDetails;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.StyledText;

/**
 * Dialog for displaying windows event log record details
 */
public class WindowsEventLogRecordDetailsDialog extends Dialog
{
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
      newShell.setText("Windows Event Log Record Details");
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
      createButton(parent, IDialogConstants.OK_ID, "Close", true);
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
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 800;
      gd.heightHint = 500;
      tabFolder.setLayoutData(gd);
      
      createInformationTab("General");      
      String rawData = data.getValue("raw_data");
      createTextValueTab("Details", formatXML(rawData));      
      tabFolder.setSelection(0);

      return dialogArea;
   }
   
   private String formatXML(String text)
   {
      String result = "";
      try {
         StringWriter stringWriter = new StringWriter();
         StreamResult xmlOutput = new StreamResult(stringWriter);
         TransformerFactory transformerFactory = TransformerFactory.newInstance();
         transformerFactory.setAttribute("indent-number", 3);
         Transformer transformer = transformerFactory.newTransformer(); 
         transformer.setOutputProperty(OutputKeys.INDENT, "yes");
         transformer.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "yes"); //$NON-NLS-1$
         transformer.transform(new StreamSource(new ByteArrayInputStream(text.getBytes(StandardCharsets.UTF_8))), xmlOutput);
         result = xmlOutput.getWriter().toString();
     } catch (Exception e) {
        throw new RuntimeException(e);
     }
     return result;
   }

   /**
    * Create tab with main informaiton
    * 
    * @param string
    * @param data2
    */
   private void createInformationTab(String name)
   {
      CTabItem tab = createTab(name);
      
      Composite dialogArea = new Composite(tabFolder, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);
      
      StyledText textControl = new StyledText(dialogArea, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY | SWT.WRAP);
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
            "Logged", RegionalSettings.getDateTimeFormat().format(date), WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            "Source", record.get(logHandle.getColumnIndex("event_source")).getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            "Event ID", record.get(logHandle.getColumnIndex("event_code")).getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);

      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            "Level", record.get(logHandle.getColumnIndex("event_severity")).getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      tab.setControl(dialogArea);
   }

   /**
    * Create text value tab
    * 
    * @param name
    * @param imageName
    * @param text
    */
   private void createTextValueTab(String name, String text)
   {
      CTabItem tab = createTab(name);
      // Use StyledText instead of Text for uniform look with "Diff" tab
      StyledText textControl = new StyledText(tabFolder, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY | SWT.WRAP);
      textControl.setFont(JFaceResources.getTextFont());
      textControl.setText(text);
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
