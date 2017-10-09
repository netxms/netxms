/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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

import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.dialogs.DCISummaryTableSortColumnSelectionDialog;
import org.netxms.ui.eclipse.dashboard.propertypages.helpers.SortColumnTableLabelProvider;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DciSummaryTableConfig;
import org.netxms.ui.eclipse.datacollection.widgets.SummaryTableSelector;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.AbstractSelector;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * DCI summary table element properties
 */
public class DciSummaryTable extends PropertyPage
{
   public static final int NAME = 0;
   public static final int ORDER = 1;
   
	private DciSummaryTableConfig config;
	private List<String> currSortingList;
	private ObjectSelector objectSelector;
	private SummaryTableSelector tableSelector;
	private Spinner refreshInterval;
   private Button checkShowSortAndLimitFields;
   private Spinner numRowsShow;
	private SortableTableViewer sortTables;
	private Button buttonAdd;
	private Button buttonEdit;
	private Button buttonRemove;
	private Button buttonUp;
	private Button buttonDown;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (DciSummaryTableConfig)getElement().getAdapter(DciSummaryTableConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		currSortingList = config.getSortingColumnList();
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
		objectSelector.setLabel(Messages.get().DciSummaryTable_BaseObject);
		objectSelector.setObjectClass(AbstractObject.class);
		objectSelector.setObjectId(config.getBaseObjectId());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		objectSelector.setLayoutData(gd);
		
		tableSelector = new SummaryTableSelector(dialogArea, SWT.NONE, AbstractSelector.SHOW_CLEAR_BUTTON);
		tableSelector.setLabel(Messages.get().DciSummaryTable_SummaryTable);
		tableSelector.setTableId(config.getTableId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      tableSelector.setLayoutData(gd);
      
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      refreshInterval = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.get().DciSummaryTable_RefreshInterval, 0, 10000, gd);
      refreshInterval.setSelection(config.getRefreshInterval());

      
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      checkShowSortAndLimitFields  = new Button(dialogArea, SWT.CHECK);
      checkShowSortAndLimitFields.setText("Show top N nodes");
      checkShowSortAndLimitFields.setSelection(config.isEnableSortingAndLineLimit());
      checkShowSortAndLimitFields.setLayoutData(gd);
      checkShowSortAndLimitFields.addSelectionListener(new SelectionListener() {
         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if(checkShowSortAndLimitFields.getSelection())
            {
               sortTables.setSelection(null);
               buttonAdd.setEnabled(true);
               numRowsShow.setEnabled(true);
            }
            else
            {
               buttonUp.setEnabled(false);
               buttonDown.setEnabled(false);
               buttonEdit.setEnabled(false);
               buttonRemove.setEnabled(false);   
               buttonAdd.setEnabled(false);  
               numRowsShow.setEnabled(false);          
            }            
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
         }
      });
      
      Composite showTopNNodes = new Composite(dialogArea, SWT.BORDER);      
      layout = new GridLayout();
      showTopNNodes.setLayout(layout);
      
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      numRowsShow = WidgetHelper.createLabeledSpinner(showTopNNodes, SWT.BORDER, "Number of lines to show", 0, 10000, gd);
      numRowsShow.setSelection(config.getNumRowShown());
      numRowsShow.setEnabled(config.isEnableSortingAndLineLimit());
      
      Label label = new Label(showTopNNodes, SWT.NONE);
      label.setText("Sort by columns");

      final String[] columnNames = { "Name", "Sort order" };
      final int[] columnWidths = { 320, 120 };
      sortTables  = new SortableTableViewer(showTopNNodes, columnNames, columnWidths, 0, 0, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 2;
      gd.heightHint = 150;
      sortTables.getTable().setLayoutData(gd);
      sortTables.setContentProvider(new ArrayContentProvider());
      sortTables.setLabelProvider(new SortColumnTableLabelProvider());
      sortTables.setInput(currSortingList);
      sortTables.addSelectionChangedListener(new ISelectionChangedListener() {
         
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            if(checkShowSortAndLimitFields.getSelection())
            {
               IStructuredSelection selection = (IStructuredSelection)event.getSelection();
               int index = currSortingList.indexOf(selection.getFirstElement());
               buttonUp.setEnabled((selection.size() == 1) && (index > 0));
               buttonDown.setEnabled((selection.size() == 1) && (index >= 0) && (index < currSortingList.size() - 1));
               buttonEdit.setEnabled(selection.size() == 1);
               buttonRemove.setEnabled(selection.size() > 0);      
            }
            else
            {
               buttonUp.setEnabled(false);
               buttonDown.setEnabled(false);
               buttonEdit.setEnabled(false);
               buttonRemove.setEnabled(false);               
            }
         }
      });
      
      sortTables.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            buttonEdit.notifyListeners(SWT.Selection, new Event());
         }
      });
      
      Composite buttons = new Composite(showTopNNodes, SWT.NONE);
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      buttons.setLayoutData(gd);
      
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttons.setLayout(buttonLayout);
      
      buttonUp = new Button(buttons, SWT.PUSH);
      buttonUp.setText("UP");
      buttonUp.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveUp();
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonUp.setLayoutData(rd);
      buttonUp.setEnabled(false);
      
      buttonDown = new Button(buttons, SWT.PUSH);
      buttonDown.setText("Down");
      buttonDown.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveDown();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonDown.setLayoutData(rd);
      buttonDown.setEnabled(false);
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      buttons.setLayoutData(gd);

      buttonAdd = new Button(buttons, SWT.PUSH);
      buttonAdd.setText("Add");
      buttonAdd.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            add();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonAdd.setLayoutData(rd);
      buttonAdd.setEnabled(config.isEnableSortingAndLineLimit());
      
      buttonEdit = new Button(buttons, SWT.PUSH);
      buttonEdit.setText("Edit");
      buttonEdit.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            edit();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonEdit.setLayoutData(rd);
      buttonEdit.setEnabled(false);

      buttonRemove = new Button(buttons, SWT.PUSH);
      buttonRemove.setText("Delete");
      buttonRemove.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            remove();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonRemove.setLayoutData(rd);
      buttonRemove.setEnabled(false);
		
		return dialogArea;
	}
   
   /**
    * Move selected field up 
    */
   private void moveUp()
   {
      IStructuredSelection selection = (IStructuredSelection)sortTables.getSelection();
      if (selection.size() != 1)
         return;
      
      String element = (String)selection.getFirstElement();
      int index = currSortingList.indexOf(element);
      if (index <= 0)
         return;
      
      String a = currSortingList.get(index - 1);
      currSortingList.set(index - 1, currSortingList.get(index));
      currSortingList.set(index, a);
      sortTables.setInput(currSortingList.toArray());
      sortTables.setSelection(selection);
   }
   
   /**
    * Move selected field down 
    */
   private void moveDown()
   {
      IStructuredSelection selection = (IStructuredSelection)sortTables.getSelection();
      if (selection.size() != 1)
         return;
      
      String element = (String)selection.getFirstElement();
      int index = currSortingList.indexOf(element);
      if (index >= (currSortingList.size() -1) && index < 0)
         return;
      
      String a = currSortingList.get(index + 1);
      currSortingList.set(index + 1, currSortingList.get(index));
      currSortingList.set(index, a);
      sortTables.setInput(currSortingList.toArray());
      sortTables.setSelection(selection);
   }
   
   /**
    * Add new sorting column 
    */
   private void add()
   {
      DCISummaryTableSortColumnSelectionDialog dlg = new DCISummaryTableSortColumnSelectionDialog(getShell(), null, false, tableSelector.getTableId());
      if (dlg.open() == Window.OK)
      {
         String s = dlg.getSortingColumn();
         if (!currSortingList.contains(s))
         {
            currSortingList.add(s);
            sortTables.setInput(currSortingList.toArray());
         }
      }
   }
   
   /**
    * Edit new sorting column 
    */
   private void edit()
   {
      IStructuredSelection selection = (IStructuredSelection)sortTables.getSelection();
      if (selection.size() != 1)
         return;

      String element = (String)selection.getFirstElement();
      DCISummaryTableSortColumnSelectionDialog dlg = new DCISummaryTableSortColumnSelectionDialog(getShell(), element.substring(1), element.charAt(0) == '>' ? true : false, tableSelector.getTableId());
      if (dlg.open() == Window.OK)
      {
         String s = dlg.getSortingColumn();
         int index = currSortingList.indexOf(element);
         if (index < currSortingList.size() && index >= 0)
         {
            currSortingList.set(index, s);
            sortTables.setInput(currSortingList.toArray());
         }
      }
   }
   
   /**
    * Remove column for sorting
    */
   private void remove()
   {
      IStructuredSelection selection = (IStructuredSelection)sortTables.getSelection();
      if (selection.size() != 1)
         return;
      
      String element = (String)selection.getFirstElement();
      currSortingList.remove(element);
      sortTables.setInput(currSortingList.toArray());
   }
   
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		config.setBaseObjectId(objectSelector.getObjectId());
		config.setTableId(tableSelector.getTableId());
		config.setRefreshInterval(refreshInterval.getSelection());
      config.setNumRowShown(numRowsShow.getSelection());
      config.setSortingColumnList(currSortingList);
      config.setEnableSortingAndLineLimit(checkShowSortAndLimitFields.getSelection());
		return true;
	}
}
