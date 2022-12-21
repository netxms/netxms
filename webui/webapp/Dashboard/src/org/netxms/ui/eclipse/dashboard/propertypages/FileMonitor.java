/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.dashboard.widgets.TitleConfigurator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.FileMonitorConfig;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * File monitor element properties
 */
public class FileMonitor extends PropertyPage
{
   private FileMonitorConfig config;
   private ObjectSelector objectSelector;
   private LabeledText fileName;
   private LabeledText filter;
   private LabeledSpinner historyLimit;
   private Combo syntaxHighlighter;
   private TitleConfigurator title;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      config = getElement().getAdapter(FileMonitorConfig.class);

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      title.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true, true);
      objectSelector.setLabel("Node");
      objectSelector.setObjectClass(Node.class);
      objectSelector.setObjectId(config.getObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      objectSelector.setLayoutData(gd);

      fileName = new LabeledText(dialogArea, SWT.NONE);
      fileName.setLabel("File name");
      fileName.setText(config.getFileName());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      fileName.setLayoutData(gd);

      filter = new LabeledText(dialogArea, SWT.NONE);
      filter.setLabel("Line filter (regular expression)");
      filter.setText(config.getFilter());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      filter.setLayoutData(gd);

      historyLimit = new LabeledSpinner(dialogArea, SWT.NONE);
      historyLimit.setLabel("History limit");
      historyLimit.setRange(0, 99999999);
      historyLimit.setSelection(config.getHistoryLimit());

      syntaxHighlighter = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, "Highlighter", WidgetHelper.DEFAULT_LAYOUT_DATA);
      syntaxHighlighter.add("None");
      int index = syntaxHighlighter.indexOf(config.getSyntaxHighlighter());
      syntaxHighlighter.select((index >= 0) ? index : 0);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      title.updateConfiguration(config);
      config.setObjectId(objectSelector.getObjectId());
      config.setFileName(fileName.getText().trim());
      config.setFilter(filter.getText());
      config.setHistoryLimit(historyLimit.getSelection());
      config.setSyntaxHighlighter(syntaxHighlighter.getText());
      return true;
   }
}
