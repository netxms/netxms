/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.propertypages;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.TableValueConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.datacollection.widgets.DciSelector;
import org.netxms.nxmc.modules.datacollection.widgets.TemplateDciSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Table last value element properties
 */
public class TableValue extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(TableValue.class);

	private TableValueConfig config;
	private DciSelector dciSelector;
   private TemplateDciSelector dciName;
   private TemplateDciSelector dciDescription;
   private TemplateDciSelector dciTag;
   private TitleConfigurator title;
	private Spinner refreshRate;
	private Combo sortColumn;
	private Button sortDirectionAscending;
   private Button sortDirectionDescending;
	

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public TableValue(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(TableValue.class).tr("Table Value"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "table-value";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof TableValueConfig;
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 0;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      config = (TableValueConfig)elementConfig;

		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);

      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      title.setLayoutData(gd);

      dciSelector = new DciSelector(dialogArea, SWT.NONE, true);
      dciSelector.setLabel(i18n.tr("Table DCI"));
		dciSelector.setDcObjectType(DataCollectionObject.DCO_TYPE_TABLE);
		dciSelector.setDciId(config.getObjectId(), config.getDciId());
      gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dciSelector.setLayoutData(gd);
      dciSelector.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            boolean isTemplate = (dciSelector.getNodeId() == AbstractObject.CONTEXT);
            dciName.setEnabled(isTemplate);
            dciDescription.setEnabled(isTemplate);
            dciTag.setEnabled(isTemplate);
         }
      });

      dciName = new TemplateDciSelector(dialogArea, SWT.NONE);
      dciName.setLabel(i18n.tr("DCI Name"));
      dciName.setText(config.getDciName());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      dciName.setLayoutData(gd);
      dciName.setEnabled(config.getObjectId() == AbstractObject.CONTEXT);

      dciDescription = new TemplateDciSelector(dialogArea, SWT.NONE);
      dciDescription.setLabel(i18n.tr("DCI Description"));
      dciDescription.setText(config.getDciDescription());
      dciDescription.setField(TemplateDciSelector.Field.DESCRIPTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      dciDescription.setLayoutData(gd);
      dciDescription.setEnabled(config.getObjectId() == AbstractObject.CONTEXT);

      dciTag = new TemplateDciSelector(dialogArea, SWT.NONE);
      dciTag.setLabel(i18n.tr("DCI Tag"));
      dciTag.setText(config.getDciDescription());
      dciTag.setField(TemplateDciSelector.Field.DESCRIPTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      dciTag.setLayoutData(gd);
      dciTag.setEnabled(config.getObjectId() == AbstractObject.CONTEXT);
      
      Group goupSortingOrder = new Group(dialogArea, SWT.NONE);
      goupSortingOrder.setText("Sorting order");
      layout = new GridLayout();
      layout.marginTop = WidgetHelper.OUTER_SPACING;
      layout.marginBottom = WidgetHelper.OUTER_SPACING * 2;
      layout.marginWidth = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      goupSortingOrder.setLayout(layout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      goupSortingOrder.setLayoutData(gd);
      
      sortDirectionAscending = new Button(goupSortingOrder, SWT.RADIO);
      sortDirectionAscending.setText(i18n.tr("Ascending"));
      sortDirectionAscending.setSelection(config.getSortDirection() == SWT.UP);
      sortDirectionAscending.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));
      
      sortDirectionDescending = new Button(goupSortingOrder, SWT.RADIO);
      sortDirectionDescending.setText(i18n.tr("Descending"));
      sortDirectionDescending.setSelection(config.getSortDirection() == SWT.DOWN);
      sortDirectionDescending.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));

      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      sortColumn = WidgetHelper.createLabeledCombo(dialogArea, SWT.NONE, i18n.tr("Sort column"), gd);
      
      if (config.getObjectId() != AbstractObject.CONTEXT && config.getObjectId() != 0 && config.getDciId() != 0)
      {
         NXCSession session = Registry.getSession();
         new Job(i18n.tr("Get table DCI columns"), null) {            
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               DataCollectionObject table = session.getDataCollectionObject(config.getObjectId(), config.getDciId());
               if (table == null || !(table instanceof DataCollectionTable))
                  return;
               
               runInUIThread(() -> {
                  DataCollectionTable dciTable = (DataCollectionTable)table;
                  List<ColumnDefinition> dciColumns = dciTable.getColumns();
                  int selection = -1;
                  int index = 0;
                  ArrayList<String> columns = new ArrayList<String>();
                  for(; index < dciColumns.size(); index++)
                  {
                     columns.add(dciColumns.get(index).getName());
                     if (config.getSortColumn() != null && config.getSortColumn().equals(dciColumns.get(index).getName()))
                     {
                        selection = index;
                     }
                  }
                  if (selection == -1 && config.getSortColumn() != null)
                  {
                     columns.add(config.getSortColumn());
                     selection = index;
                  }    
                  sortColumn.setItems(columns.toArray(new String[columns.size()]));
                  sortColumn.select(selection);
               });
            }
            
            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot get table DCI columns");
            }
         }.start();
      }      

		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      refreshRate = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Refresh interval (seconds)"), 1, 10000, gd);
		refreshRate.setSelection(config.getRefreshRate());

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
      title.updateConfiguration(config);
		config.setObjectId(dciSelector.getNodeId());
		config.setDciId(dciSelector.getDciId());
      config.setDciName(dciName.getText());
      config.setDciDescription(dciDescription.getText());
      config.setDciTag(dciTag.getText());
		config.setRefreshRate(refreshRate.getSelection());
		config.setSortColumn(sortColumn.getText());
		config.setSortDirection(sortDirectionAscending.getSelection() ? SWT.UP : SWT.DOWN);
		return true;
	}
}
