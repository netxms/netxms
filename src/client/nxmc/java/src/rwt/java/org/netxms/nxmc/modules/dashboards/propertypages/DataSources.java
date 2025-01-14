/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import java.util.Collections;
import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciValue;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.AbstractChartConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.ScriptedComparisonChartConfig;
import org.netxms.nxmc.modules.dashboards.propertypages.helpers.DciListLabelProvider;
import org.netxms.nxmc.modules.datacollection.dialogs.DataSourceEditDlg;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectDciDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * DCI list editor for dashboard element
 */
public class DataSources extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(DataSources.class);

	public static final int COLUMN_POSITION = 0;
	public static final int COLUMN_NODE = 1;
   public static final int COLUMN_DCI_DISPLAY_NAME = 2;
	public static final int COLUMN_METRIC = 3;
   public static final int COLUMN_DCI_TAG = 4;
   public static final int COLUMN_LABEL = 5;
   public static final int COLUMN_COLOR = 6;

	private AbstractChartConfig config;
	private DciListLabelProvider labelProvider;
	private SortableTableViewer viewer;
	private Button addButton;
   private Button addTemplateButton;
	private Button editButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;
	private List<ChartDciConfig> dciList = null;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public DataSources(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(DataSources.class).tr("Data Sources"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "data-sources";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (elementConfig instanceof AbstractChartConfig) && !(elementConfig instanceof ScriptedComparisonChartConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 20;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      config = (AbstractChartConfig)elementConfig;

		Composite dialogArea = new Composite(parent, SWT.NONE);
		
      dciList = new ArrayList<ChartDciConfig>();
      for(ChartDciConfig dci : config.getDciList())
      	dciList.add(new ChartDciConfig(dci));

		labelProvider = new DciListLabelProvider(dciList);
		labelProvider.resolveDciNames(dciList);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);

      final String[] columnNames = { i18n.tr("Pos"), i18n.tr("Node"), i18n.tr("DCI display name"), i18n.tr("Metric"), i18n.tr("DCI tag"), i18n.tr("Label"), i18n.tr("Color") };
      final int[] columnWidths = { 40, 130, 200, 200, 100, 150, 50 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(labelProvider);
      viewer.disableSorting();
      viewer.setInput(dciList);

      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      gridData.horizontalSpan = 2;
      viewer.getControl().setLayoutData(gridData);

      /* buttons on left side */
      Composite leftButtons = new Composite(dialogArea, SWT.NONE);
      GridLayout buttonLayout = new GridLayout();
      buttonLayout.numColumns = 2;
      buttonLayout.marginWidth = 0;
      leftButtons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      leftButtons.setLayoutData(gridData);

      upButton = new Button(leftButtons, SWT.PUSH);
      upButton.setText(i18n.tr("&Up"));
      GridData gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      upButton.setLayoutData(gd);
      upButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				moveUp();
			}
      });
      upButton.setEnabled(false);

      downButton = new Button(leftButtons, SWT.PUSH);
      downButton.setText(i18n.tr("Dow&n"));
      gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      downButton.setLayoutData(gd);
      downButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				moveDown();
			}
      });
      downButton.setEnabled(false);

      /* buttons on right side */
      Composite rightButtons = new Composite(dialogArea, SWT.NONE);
      buttonLayout = new GridLayout();
      buttonLayout.numColumns = 4;
      buttonLayout.marginWidth = 0;
      rightButtons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      rightButtons.setLayoutData(gridData);

      addButton = new Button(rightButtons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      addButton.setLayoutData(gd);
      addButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addItem();
			}
      });

      addTemplateButton = new Button(rightButtons, SWT.PUSH);
      addTemplateButton.setText(i18n.tr("Add &template..."));
      gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      addTemplateButton.setLayoutData(gd);
      addTemplateButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addTemplateItem();
         }
      });

      editButton = new Button(rightButtons, SWT.PUSH);
      editButton.setText(i18n.tr("&Edit..."));
      gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      editButton.setLayoutData(gd);
      editButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editItem();
			}
      });
      editButton.setEnabled(false);

      deleteButton = new Button(rightButtons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      deleteButton.setLayoutData(gd);
      deleteButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteItems();
			}
      });
      deleteButton.setEnabled(false);

      viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
            editItem();
			}
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
            IStructuredSelection selection = viewer.getStructuredSelection();
				editButton.setEnabled(selection.size() == 1);
				deleteButton.setEnabled(selection.size() > 0);
				upButton.setEnabled(selection.size() == 1);
				downButton.setEnabled(selection.size() == 1);
			}
		});

		return dialogArea;
	}

	/**
	 * Add new item
	 */
	private void addItem()
	{
		SelectDciDialog dlg = new SelectDciDialog(getShell(), 0);
		if (dlg.open() == Window.OK)
		{
		   List<DciValue> selection = dlg.getSelection();
		   List<ChartDciConfig> newSelection = new ArrayList<ChartDciConfig>();		   
			for(DciValue v : selection)
			{
			   ChartDciConfig dci = new ChartDciConfig(v);
			   newSelection.add(dci);
            labelProvider.addCacheEntry(dci.nodeId, dci.dciId, v.getName(), v.getDescription(), v.getUserTag());
            dciList.add(dci);
			}			
         viewer.refresh();
         viewer.setSelection(new StructuredSelection(newSelection));
		}
	}

   /**
    * Add new template item
    */
   private void addTemplateItem()
   {
      ChartDciConfig dci = new ChartDciConfig();
      DataSourceEditDlg dlg = new DataSourceEditDlg(getShell(), dci, true, true);
      if (dlg.open() == Window.OK)
      {
         dciList.add(dci);
         viewer.refresh();
         viewer.setSelection(new StructuredSelection(dci));
      }
   }

   /**
    * Edit selected item
    */
   private void editItem()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      ChartDciConfig dci = (ChartDciConfig)selection.getFirstElement();
      if (dci == null)
         return;

      DataSourceEditDlg dlg = new DataSourceEditDlg(getShell(), dci, false, dci.getDciId() == 0);
      if (dlg.open() == Window.OK)
      {
         labelProvider.resolveDciNames(dciList);
         viewer.update(dci, null);
      }
   }

	/**
	 * Delete selected item(s)
	 */
	private void deleteItems()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		for(Object o : selection.toList())
			dciList.remove(o);
      viewer.refresh();
	}

	/**
	 * Move selected item up 
	 */
	private void moveUp()
	{
      final IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() == 1)
		{
			Object element = selection.getFirstElement();
			int index = dciList.indexOf(element);
			if (index > 0)
			{
				Collections.swap(dciList, index - 1, index);
            viewer.refresh();
			}
		}
	}

	/**
	 * Move selected item down
	 */
	private void moveDown()
	{
      final IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() == 1)
		{
			Object element = selection.getFirstElement();
			int index = dciList.indexOf(element);
			if ((index < dciList.size() - 1) && (index >= 0))
			{
				Collections.swap(dciList, index + 1, index);
            viewer.refresh();
			}
		}
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
		config.setDciList(dciList.toArray(new ChartDciConfig[dciList.size()]));
		return true;
	}
}
