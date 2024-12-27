/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.propertypages.helpers.DciListLabelProvider;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AbstractChartConfig;
import org.netxms.ui.eclipse.datacollection.dialogs.DataSourceEditDlg;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectDciDialog;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * DCI list editor for dashboard element
 */
public class DataSources extends PropertyPage
{
	public static final int COLUMN_POSITION = 0;
	public static final int COLUMN_NODE = 1;
   public static final int COLUMN_METRIC_DISPLAYNAME = 2;
	public static final int COLUMN_METRIC = 3;
	public static final int COLUMN_LABEL = 4;
	public static final int COLUMN_COLOR = 5;

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
	private ColorCache colorCache;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      config = getElement().getAdapter(AbstractChartConfig.class);

		Composite dialogArea = new Composite(parent, SWT.NONE);
		colorCache = new ColorCache(dialogArea);

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

      final String[] columnNames = { Messages.get().DataSources_Pos, Messages.get().DataSources_Node, "DCI display name", "Metric", Messages.get().DataSources_Label, Messages.get().DataSources_Color };
      final int[] columnWidths = { 40, 130, 200, 200, 150, 50 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(labelProvider);
      viewer.disableSorting();
      viewer.getTable().addListener(SWT.PaintItem, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				if (event.index == COLUMN_COLOR)
					drawColorCell(event);
			}
		});
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
      upButton.setText(Messages.get().DataSources_Up);
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
      downButton.setText(Messages.get().DataSources_Down);
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
      addButton.setText(Messages.get().DataSources_Add);
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
      addTemplateButton.setText("Add &template...");
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
      editButton.setText(Messages.get().DataSources_Modify);
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
      deleteButton.setText(Messages.get().DataSources_Delete);
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
	 * @param event
	 */
	private void drawColorCell(Event event)
	{
		TableItem item = (TableItem)event.item;
		ChartDciConfig dci = (ChartDciConfig)item.getData();
		if (dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR))
			return;

		int width = viewer.getTable().getColumn(COLUMN_COLOR).getWidth();
		Color color = ColorConverter.colorFromInt(dci.getColorAsInt(), colorCache);
		event.gc.setForeground(colorCache.create(0, 0, 0));
		event.gc.setBackground(color);
		event.gc.setLineWidth(1);
		event.gc.fillRectangle(event.x + 3, event.y + 2, width - 7, event.height - 5);
		event.gc.drawRectangle(event.x + 3, event.y + 2, width - 7, event.height - 5);
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
			for(DciValue item : selection)
			{
			   ChartDciConfig dci = new ChartDciConfig(item);
			   newSelection.add(dci);
            labelProvider.addCacheEntry(dci.nodeId, dci.dciId, item.getName(), item.getDescription(), item.getUserTag());
            dciList.add(dci);
			}			
         viewer.refresh();
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
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		config.setDciList(dciList.toArray(new ChartDciConfig[dciList.size()]));
		return true;
	}
}
