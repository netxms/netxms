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
package org.netxms.nxmc.modules.datacollection.propertypages;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.preference.PreferencePage;
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
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.DataSourceEditDlg;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectDciDialog;
import org.netxms.nxmc.modules.datacollection.propertypages.helpers.DciTemplateListLabelProvider;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Template data sources
 */
public class TemplateDataSources extends PreferencePage
{
   private final I18n i18n = LocalizationHelper.getI18n(TemplateDataSources.class);

	public static final int COLUMN_POSITION = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_DESCRIPTION = 2;
   public static final int COLUMN_TAG = 3;
   public static final int COLUMN_LABEL = 4;
   public static final int COLUMN_COLOR = 5;

   private GraphDefinition config;
	private DciTemplateListLabelProvider labelProvider;
	private SortableTableViewer viewer;
	private Button addButton;
	private Button pickButton;
	private Button editButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;
	private List<ChartDciConfig> dciList = null;
	private ColorCache colorCache;
	private boolean saveToDatabase;

   /**
    * Constructor
    * @param settings
    */
   public TemplateDataSources(GraphDefinition settings, boolean saveToDatabase)
   {
      super("Template Data Source");
      config = settings; 
      this.saveToDatabase = saveToDatabase;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		colorCache = new ColorCache(dialogArea);

      dciList = new ArrayList<ChartDciConfig>();
      for(ChartDciConfig dci : config.getDciList())
      	dciList.add(new ChartDciConfig(dci));

		labelProvider = new DciTemplateListLabelProvider(dciList);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);

      final String[] columnNames = { i18n.tr("Pos."), i18n.tr("DCI name"), i18n.tr("DCI description"), i18n.tr("DCI tag"), i18n.tr("Display name"), i18n.tr("Color") };
      final int[] columnWidths = { 40, 130, 200, 120, 150, 50 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(labelProvider);
      viewer.getTable().addListener(SWT.PaintItem, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				if (event.index == COLUMN_COLOR)
					drawColorCell(event);
			}
		});
      viewer.setInput(dciList.toArray());

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
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginLeft = 0;
      leftButtons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      leftButtons.setLayoutData(gridData);

      upButton = new Button(leftButtons, SWT.PUSH);
      upButton.setText(i18n.tr("&Up"));
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      upButton.setLayoutData(rd);
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
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      downButton.setLayoutData(rd);
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
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      rightButtons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      rightButtons.setLayoutData(gridData);

      pickButton = new Button(rightButtons, SWT.PUSH);
      pickButton.setText(i18n.tr("&Pick..."));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      pickButton.setLayoutData(rd);
      pickButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				pickFromExistingItem();
			}
      });

      addButton = new Button(rightButtons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addItem();
         }
      });

      editButton = new Button(rightButtons, SWT.PUSH);
      editButton.setText(i18n.tr("&Edit..."));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(rd);
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
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
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
				editButton.notifyListeners(SWT.Selection, new Event());
			}
      });
      
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
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
    * Pick name and description from existing item
    */
	private void pickFromExistingItem()
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
   			dciList.add(dci);           
			}
         viewer.setInput(dciList.toArray());
         viewer.setSelection(new StructuredSelection(newSelection));
		}
	}

   /**
    * Add new item
    */
   private void addItem()
   {
      ChartDciConfig dci = new ChartDciConfig();
      DataSourceEditDlg dlg = new DataSourceEditDlg(getShell(), dci, true, true);
      if (dlg.open() == Window.OK)
      {
         dciList.add(dci);
         viewer.setInput(dciList.toArray());
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
		DataSourceEditDlg dlg = new DataSourceEditDlg(getShell(), dci, false, true);
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
      final IStructuredSelection selection = viewer.getStructuredSelection();
		for(Object o : selection.toList())
			dciList.remove(o);
      viewer.setInput(dciList.toArray());
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
		      viewer.setInput(dciList.toArray());
		      viewer.setSelection(new StructuredSelection(element));
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
		      viewer.setInput(dciList.toArray());
		      viewer.setSelection(new StructuredSelection(element));
			}
		}
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
      if (!isControlCreated())
         return;

		config.setDciList(dciList.toArray(new ChartDciConfig[dciList.size()]));
		if (saveToDatabase && isApply)
		{
			setValid(false);
         final NXCSession session = Registry.getSession();
         new Job(i18n.tr("Update data sources for predefined graph"), null) {
				@Override
            protected void run(IProgressMonitor monitor) throws Exception
				{
               session.saveGraph(config, false);
				}
	
				@Override
				protected void jobFinalize()
				{
               runInUIThread(() -> TemplateDataSources.this.setValid(true));
				}

				@Override
				protected String getErrorMessage()
				{
               return i18n.tr("Cannot change data sources");
				}
			}.start();
		}
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
