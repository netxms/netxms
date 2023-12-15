/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.propertypages;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
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
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ConditionDciInfo;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Condition;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectDciDialog;
import org.netxms.nxmc.modules.objects.dialogs.ConditionDciEditDialog;
import org.netxms.nxmc.modules.objects.propertypages.helpers.ConditionDciListLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Data" property page for condition objects
 */
public class ConditionData extends ObjectPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(ConditionData.class);
   
	public static final int COLUMN_POSITION = 0;
	public static final int COLUMN_NODE = 1;
	public static final int COLUMN_METRIC = 2;
	public static final int COLUMN_FUNCTION = 3;
	
	private Condition condition = null;
	private ConditionDciListLabelProvider labelProvider;
	private SortableTableViewer viewer;
	private Button addButton;
	private Button editButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;
	private List<ConditionDciInfo> dciList = null;
	private boolean isModified = false;

   /**
    * Create new page.
    *
    * @param condition object to edit
    */
   public ConditionData(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ConditionData.class).tr("Data"), object);
   }

   @Override
   public String getId()
   {
      return "conditionData";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 10;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof Condition;
   }

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
      this.condition = (Condition)object;
		
      dciList = new ArrayList<ConditionDciInfo>();
      for(ConditionDciInfo dci : condition.getDciList())
      	dciList.add(new ConditionDciInfo(dci));
      
		labelProvider = new ConditionDciListLabelProvider(dciList);
		labelProvider.resolveDciNames(condition.getDciList());
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      final String[] columnNames = { i18n.tr("Pos"), i18n.tr("Node"), i18n.tr("Parameter"), i18n.tr("Function") };
      final int[] columnWidths = { 40, 130, 220, 80 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                       SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(labelProvider);
      viewer.disableSorting();
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
      downButton.setText(i18n.tr("&Down"));
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
      editButton.setText(i18n.tr("&Modify..."));
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
	 * Add new item
	 */
	private void addItem()
	{
		SelectDciDialog dlg = new SelectDciDialog(getShell(), 0);
		if (dlg.open() == Window.OK)
		{
		   List<DciValue> selection = dlg.getSelection();
		   List<ConditionDciInfo> select = new ArrayList<ConditionDciInfo>();
			for (DciValue item : selection)
			{
   			ConditionDciInfo dci = new ConditionDciInfo(item.getNodeId(), item.getId(), item.getDcObjectType(), Threshold.F_LAST, 1);
   			labelProvider.addCacheEntry(dci.getNodeId(), dci.getDciId(), item.getDescription());
   			dciList.add(dci);
   			select.add(dci);
   			viewer.setInput(dciList.toArray());
   			isModified = true;
   			viewer.setSelection(new StructuredSelection(dci));
   			
            editItem();
			}
         viewer.setSelection(new StructuredSelection(select));
		}
	}
	
	/**
	 * Edit selected item
	 */
	private void editItem()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		ConditionDciInfo dci = (ConditionDciInfo)selection.getFirstElement();
		if (dci == null)
			return;
		
		ConditionDciEditDialog dlg = new ConditionDciEditDialog(getShell(), dci, 
				labelProvider.getColumnText(dci, COLUMN_NODE), labelProvider.getColumnText(dci, COLUMN_METRIC));
		if (dlg.open() == Window.OK)
		{
			viewer.update(dci, null);
			isModified = true;
		}
	}
	
	/**
	 * Delete selected item(s)
	 */
	private void deleteItems()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		for(Object o : selection.toList())
			dciList.remove(o);
      viewer.setInput(dciList.toArray());
      isModified = true;
	}

	/**
	 * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
	 */
	@Override
	protected boolean applyChanges(final boolean isApply)
	{
		if (!isModified)
			return true;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final NXCObjectModificationData md = new NXCObjectModificationData(condition.getObjectId());
		md.setDciList(dciList);
		final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating condition's DCI list"), null, messageArea) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
				isModified = false;
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot update condition's DCI list");
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							ConditionData.this.setValid(true);
						}
					});
				}
			}
		}.start();
		
		return true;
	}
	
	/**
	 * Move selected item up 
	 */
	private void moveUp()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 1)
		{
			ConditionDciInfo element = (ConditionDciInfo)selection.getFirstElement();
			
			int index = dciList.indexOf(element);
			if (index > 0)
			{
				Collections.swap(dciList, index - 1, index);
		      viewer.setInput(dciList.toArray());
		      viewer.setSelection(new StructuredSelection(element));
		      isModified = true;
			}
		}
	}
	
	/**
	 * Move selected item down
	 */
	private void moveDown()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 1)
		{
			ConditionDciInfo element = (ConditionDciInfo)selection.getFirstElement();
			
			int index = dciList.indexOf(element);
			if ((index < dciList.size() - 1) && (index >= 0))
			{
				Collections.swap(dciList, index + 1, index);
		      viewer.setInput(dciList.toArray());
		      viewer.setSelection(new StructuredSelection(element));
			}
		}
	}
}
