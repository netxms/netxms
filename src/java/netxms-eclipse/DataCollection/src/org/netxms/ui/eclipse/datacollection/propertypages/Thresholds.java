/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.propertypages;

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.datacollection.ThresholdLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * @author Victor
 *
 */
public class Thresholds extends PropertyPage
{
	public static final int COLUMN_OPERATION = 0;
	public static final int COLUMN_EVENT = 1;
	
	private DataCollectionItem dci;
	private LabeledText instance;
	private TableViewer thresholds;
	private Button addButton;
	private Button modifyButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		dci = (DataCollectionItem)getElement().getAdapter(DataCollectionItem.class);
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      instance = new LabeledText(dialogArea, SWT.NONE);
      instance.setLabel("Instance");
      instance.setText(dci.getInstance());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      instance.setLayoutData(gd);
      
      Composite thresholdArea = new Composite(dialogArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      thresholdArea.setLayoutData(gd);
      layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      thresholdArea.setLayout(layout);
	
      new Label(thresholdArea, SWT.NONE).setText("Thresholds");
      
      thresholds = new TableViewer(thresholdArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      thresholds.getControl().setLayoutData(gd);
      setupThresholdList();
      thresholds.setInput(dci.getThresholds().toArray());
      
      Composite leftButtons = new Composite(thresholdArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      leftButtons.setLayoutData(gd);
      RowLayout buttonsLayout = new RowLayout(SWT.HORIZONTAL);
      buttonsLayout.marginBottom = 0;
      buttonsLayout.marginLeft = 0;
      buttonsLayout.marginRight = 0;
      buttonsLayout.marginTop = 0;
      buttonsLayout.spacing = WidgetHelper.OUTER_SPACING;
      buttonsLayout.fill = true;
      buttonsLayout.pack = false;
      leftButtons.setLayout(buttonsLayout);

      upButton = new Button(leftButtons, SWT.PUSH);
      upButton.setText("&Up");
      upButton.setEnabled(false);
      
      downButton = new Button(leftButtons, SWT.PUSH);
      downButton.setText("&Down");
      downButton.setEnabled(false);
      
      Composite buttons = new Composite(thresholdArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);
      buttonsLayout = new RowLayout(SWT.HORIZONTAL);
      buttonsLayout.marginBottom = 0;
      buttonsLayout.marginLeft = 0;
      buttonsLayout.marginRight = 0;
      buttonsLayout.marginTop = 0;
      buttonsLayout.spacing = WidgetHelper.OUTER_SPACING;
      buttonsLayout.fill = true;
      buttonsLayout.pack = false;
      buttons.setLayout(buttonsLayout);
      
      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText("&Add...");
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
      
      modifyButton = new Button(buttons, SWT.PUSH);
      modifyButton.setText("&Edit...");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      modifyButton.setLayoutData(rd);
      modifyButton.setEnabled(false);
      
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText("&Delete");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.setEnabled(false);
      
      /*** Selection change listener for thresholds list ***/
      thresholds.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				upButton.setEnabled(selection.size() == 1);
				downButton.setEnabled(selection.size() == 1);
				modifyButton.setEnabled(selection.size() == 1);
				deleteButton.setEnabled(selection.size() > 0);
			}
      });
      
		return dialogArea;
	}

	/**
	 * Setup threshold list control
	 */
	private void setupThresholdList()
	{
		Table table = thresholds.getTable();
		table.setLinesVisible(true);
		table.setHeaderVisible(true);
		
		TableColumn column = new TableColumn(table, SWT.LEFT);
		column.setText("Expression");
		column.setWidth(200);
		
		column = new TableColumn(table, SWT.LEFT);
		column.setText("Event");
		column.setWidth(150);
		
      thresholds.setContentProvider(new ArrayContentProvider());
      thresholds.setLabelProvider(new ThresholdLabelProvider());
	}
}
