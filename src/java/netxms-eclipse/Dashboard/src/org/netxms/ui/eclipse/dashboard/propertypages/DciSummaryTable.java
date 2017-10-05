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
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
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
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DciSummaryTableConfig;
import org.netxms.ui.eclipse.datacollection.widgets.SummaryTableSelector;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * DCI summary table element properties
 */
public class DciSummaryTable extends PropertyPage
{
	private DciSummaryTableConfig config;
	private List<String> currList;
	private ObjectSelector objectSelector;
	private SummaryTableSelector tableSelector;
	private Spinner refreshInterval;
   private Spinner numRowsShow;
	private TableViewer sortTables;
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
		currList = config.getSortingColumnList();
		
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
      numRowsShow = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, "Number of lines to show", 0, 10000, gd);
      numRowsShow.setSelection(config.getNumRowShown());
      
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Sorting list");
      
      sortTables  = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 2;
      gd.heightHint = 150;
      sortTables.getTable().setLayoutData(gd);
      sortTables.getTable().setSortDirection(SWT.UP);
      sortTables.setContentProvider(new ArrayContentProvider());
      sortTables.setInput(currList);
      sortTables.addSelectionChangedListener(new ISelectionChangedListener() {
         
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            int index = currList.indexOf(selection.getFirstElement());
            buttonUp.setEnabled((selection.size() == 1) && (index > 0));
            buttonDown.setEnabled((selection.size() == 1) && (index >= 0) && (index < currList.size() - 1));
            buttonEdit.setEnabled(selection.size() == 1);
            buttonRemove.setEnabled(selection.size() > 0);            
         }
      });
      
      Composite buttonsRight = new Composite(dialogArea, SWT.NONE);
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      buttonsRight.setLayoutData(gd);
      
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonsRight.setLayout(buttonLayout);
      
      buttonUp = new Button(buttonsRight, SWT.PUSH);
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
      
      buttonDown = new Button(buttonsRight, SWT.PUSH);
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
      buttonsRight.setLayoutData(gd);

      buttonAdd = new Button(buttonsRight, SWT.PUSH);
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
      
      buttonEdit = new Button(buttonsRight, SWT.PUSH);
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

      buttonRemove = new Button(buttonsRight, SWT.PUSH);
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
      int index = currList.indexOf(element);
      if (index <= 0)
         return;
      
      String a = currList.get(index - 1);
      currList.set(index - 1, currList.get(index));
      currList.set(index, a);
      sortTables.setInput(currList.toArray());
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
      int index = currList.indexOf(element);
      if (index >= (currList.size() -1) && index < 0)
         return;
      
      String a = currList.get(index + 1);
      currList.set(index + 1, currList.get(index));
      currList.set(index, a);
      sortTables.setInput(currList.toArray());
   }
   
   /**
    * Add new sorting column 
    */
   private void add()
   {
      InputDialog dlg = new InputDialog(getShell(), "Add sorting table", 
            "Sorting table name", "", null); //$NON-NLS-1$
      if (dlg.open() == Window.OK)
      {
         String s = dlg.getValue();
         if (!currList.contains(s))
         {
            currList.add(s);
            sortTables.setInput(currList.toArray());
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
      InputDialog dlg = new InputDialog(getShell(), "Edit sorting table", 
            "Sorting table name", element, null); //$NON-NLS-1$
      if (dlg.open() == Window.OK)
      {
         String s = dlg.getValue();
         int index = currList.indexOf(element);
         if (index < currList.size() && index >= 0)
         {
            currList.set(index, s);
            sortTables.setInput(currList.toArray());
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
      currList.remove(element);
      sortTables.setInput(currList.toArray());
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
      config.setSortingColumnList(currList);
		return true;
	}
}
