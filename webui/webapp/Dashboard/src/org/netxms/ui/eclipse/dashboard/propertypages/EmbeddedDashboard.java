/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
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
import org.eclipse.swt.widgets.Scale;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.EmbeddedDashboardConfig;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * embedded dashboard element properties
 */
public class EmbeddedDashboard extends PropertyPage
{
	private static final Set<Integer> objectSelectionFilter;
	
	static
	{
		objectSelectionFilter = new HashSet<Integer>(2);
		objectSelectionFilter.add(AbstractObject.OBJECT_DASHBOARD);
		objectSelectionFilter.add(AbstractObject.OBJECT_DASHBOARDROOT);
	}
	
	private EmbeddedDashboardConfig config;
	private List<AbstractObject> dashboardObjects;
	private TableViewer viewer;
	private Button addButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;
	private Scale refreshIntervalScale;
	private Spinner refreshIntervalSpinner;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (EmbeddedDashboardConfig)getElement().getAdapter(EmbeddedDashboardConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		Label label = new Label(dialogArea, SWT.NONE);
		label.setText(Messages.EmbeddedDashboard_Dashboards);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		label.setLayoutData(gd);
		
      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new WorkbenchLabelProvider());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 0;
		viewer.getTable().setLayoutData(gd);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		dashboardObjects = session.findMultipleObjects(config.getDashboardObjects(), Dashboard.class, false);
		viewer.setInput(dashboardObjects.toArray());

      /* buttons on left side */
      Composite leftButtons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginLeft = 0;
      leftButtons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      leftButtons.setLayoutData(gd);
      
      upButton = new Button(leftButtons, SWT.PUSH);
      upButton.setText(Messages.EmbeddedDashboard_Up);
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      upButton.setLayoutData(rd);
      upButton.addSelectionListener(new SelectionListener() {
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
      upButton.setEnabled(false);
      
      downButton = new Button(leftButtons, SWT.PUSH);
      downButton.setText(Messages.EmbeddedDashboard_Down);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      downButton.setLayoutData(rd);
      downButton.addSelectionListener(new SelectionListener() {
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
      downButton.setEnabled(false);

      /* buttons on right side */
      Composite rightButtons = new Composite(dialogArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      rightButtons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      rightButtons.setLayoutData(gd);

      addButton = new Button(rightButtons, SWT.PUSH);
      addButton.setText(Messages.EmbeddedDashboard_Add);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addDashboard();
			}
      });
		
      deleteButton = new Button(rightButtons, SWT.PUSH);
      deleteButton.setText(Messages.EmbeddedDashboard_Delete);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteDashboards();
			}
      });
      deleteButton.setEnabled(false);
      
      Composite refreshIntervalGroup = new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.marginTop = WidgetHelper.OUTER_SPACING;
      refreshIntervalGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      refreshIntervalGroup.setLayoutData(gd);
      
      label = new Label(refreshIntervalGroup, SWT.NONE);
      label.setText(Messages.EmbeddedDashboard_DisplayTime);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.horizontalSpan = 2;
      label.setLayoutData(gd);
      
      refreshIntervalScale = new Scale(refreshIntervalGroup, SWT.HORIZONTAL);
      refreshIntervalScale.setMinimum(1);
      refreshIntervalScale.setMaximum(600);
      refreshIntervalScale.setSelection(config.getDisplayInterval());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      refreshIntervalScale.setLayoutData(gd);
      refreshIntervalScale.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				refreshIntervalSpinner.setSelection(refreshIntervalScale.getSelection());
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
      });
      
      refreshIntervalSpinner = new Spinner(refreshIntervalGroup, SWT.BORDER);
      refreshIntervalSpinner.setMinimum(1);
      refreshIntervalSpinner.setMaximum(600);
      refreshIntervalSpinner.setSelection(config.getDisplayInterval());
      refreshIntervalSpinner.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				refreshIntervalScale.setSelection(refreshIntervalSpinner.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				deleteButton.setEnabled(selection.size() > 0);
				upButton.setEnabled(selection.size() == 1);
				downButton.setEnabled(selection.size() == 1);
			}
		});
		
		return dialogArea;
	}

	/**
	 * Move selected element down
	 */
	protected void moveDown()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 1)
		{
			Object element = selection.getFirstElement();
			int index = dashboardObjects.indexOf(element);
			if ((index < dashboardObjects.size() - 1) && (index >= 0))
			{
				Collections.swap(dashboardObjects, index + 1, index);
		      viewer.setInput(dashboardObjects.toArray());
		      viewer.setSelection(new StructuredSelection(element));
			}
		}
	}

	/**
	 * Move selected element up
	 */
	protected void moveUp()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 1)
		{
			Object element = selection.getFirstElement();
			int index = dashboardObjects.indexOf(element);
			if (index > 0)
			{
				Collections.swap(dashboardObjects, index - 1, index);
		      viewer.setInput(dashboardObjects.toArray());
		      viewer.setSelection(new StructuredSelection(element));
			}
		}
	}

	/**
	 * Delete selected dashboards from list
	 */
	protected void deleteDashboards()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		for(Object o : selection.toList())
			dashboardObjects.remove(o);
      viewer.setInput(dashboardObjects.toArray());
	}

	/**
	 * Add dashboard to list
	 */
	protected void addDashboard()
	{
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), null, objectSelectionFilter);
		if (dlg.open() == Window.OK)
		{
			dashboardObjects.addAll(Arrays.asList(dlg.getSelectedObjects(Dashboard.class)));
	      viewer.setInput(dashboardObjects.toArray());
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		long[] idList = new long[dashboardObjects.size()];
		for(int i = 0; i < dashboardObjects.size(); i++)
			idList[i] = dashboardObjects.get(i).getObjectId();
		config.setDashboardObjects(idList);
		config.setDisplayInterval(refreshIntervalSpinner.getSelection());
		return true;
	}
}
