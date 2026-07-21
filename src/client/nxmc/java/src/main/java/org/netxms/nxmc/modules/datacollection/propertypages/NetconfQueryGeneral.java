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
package org.netxms.nxmc.modules.datacollection.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.netxms.client.datacollection.NetconfQueryDefinition;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "General" property page for NETCONF query definition configuration
 */
public class NetconfQueryGeneral extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(NetconfQueryGeneral.class);

   private NetconfQueryDefinition definition;
   private LabeledText name;
   private Combo datastore;
   private Combo filterType;
   private LabeledText filter;
   private LabeledSpinner retentionTime;
   private LabeledSpinner timeout;
   private LabeledText description;

   /**
    * Constructor
    */
   public NetconfQueryGeneral(NetconfQueryDefinition definition)
   {
      super(LocalizationHelper.getI18n(NetconfQueryGeneral.class).tr("General"));
      noDefaultAndApplyButton();
      this.definition = definition;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Name"));
      name.setText(definition.getName());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      name.setLayoutData(gd);

      datastore = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, i18n.tr("Datastore"),
            new GridData(SWT.FILL, SWT.CENTER, true, false));
      datastore.add(i18n.tr("Operational (get)"));
      datastore.add(i18n.tr("Running"));
      datastore.add(i18n.tr("Candidate"));
      datastore.add(i18n.tr("Startup"));
      datastore.select(definition.getDatastore());

      filterType = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, i18n.tr("Filter type"),
            new GridData(SWT.FILL, SWT.CENTER, true, false));
      filterType.add(i18n.tr("None"));
      filterType.add(i18n.tr("Subtree"));
      filterType.add(i18n.tr("XPath"));
      filterType.select(definition.getFilterType());

      filter = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      filter.setLabel(i18n.tr("Filter"));
      filter.setText(definition.getFilter());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      gd.heightHint = 120;
      filter.setLayoutData(gd);

      /* options group */
      Group groupOptions = new Group(dialogArea, SWT.NONE);
      groupOptions.setText(i18n.tr("Options"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      groupOptions.setLayoutData(gd);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      groupOptions.setLayout(layout);

      retentionTime = new LabeledSpinner(groupOptions, SWT.NONE);
      retentionTime.setLabel(i18n.tr("Cache retention time (seconds)"));
      retentionTime.setRange(0, 86400);
      retentionTime.setSelection(definition.getCacheRetentionTime());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      retentionTime.setLayoutData(gd);

      timeout = new LabeledSpinner(groupOptions, SWT.NONE);
      timeout.setLabel(i18n.tr("Request timeout (milliseconds)"));
      timeout.setRange(0, 300000);
      timeout.setSelection(definition.getRequestTimeout());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      timeout.setLayoutData(gd);

      description = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL);
      description.setLabel(i18n.tr("Description"));
      description.setText(definition.getDescription());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      gd.heightHint = 120;
      description.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      String queryName = name.getText().trim();
      if (queryName.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("NETCONF query name cannot be empty!"));
         return false;
      }

      if (queryName.contains(":") || queryName.contains("/") || queryName.contains(",") || queryName.contains("(") || queryName.contains(")") || queryName.contains("{") || queryName.contains("}") ||
            queryName.contains("'") || queryName.contains("\""))
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("NETCONF query name should not contain any of the following characters: / , : ' \" ( ) { }"));
         return false;
      }

      definition.setName(queryName);
      definition.setDatastore(datastore.getSelectionIndex());
      definition.setFilterType(filterType.getSelectionIndex());
      definition.setFilter(filter.getText().trim());
      definition.setCacheRetentionTime(retentionTime.getSelection());
      definition.setRequestTimeout(timeout.getSelection());
      definition.setDescription(description.getText());
      return true;
   }
}
