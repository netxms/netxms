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

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
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
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.EmbeddedDashboardConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * embedded dashboard element properties
 */
public class EmbeddedDashboard extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(EmbeddedDashboard.class);

	private EmbeddedDashboardConfig config;
	private List<AbstractObject> dashboardObjects;
   private ObjectSelector contextObjectSelector;
   private TitleConfigurator title;
	private TableViewer viewer;
	private Button addButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;
	private Scale refreshIntervalScale;
	private Spinner refreshIntervalSpinner;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public EmbeddedDashboard(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(EmbeddedDashboard.class).tr("Dashboard"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "dashboard";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof EmbeddedDashboardConfig;
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
      config = (EmbeddedDashboardConfig)elementConfig;
		
		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = layout.numColumns;
      title.setLayoutData(gd);

      contextObjectSelector = new ObjectSelector(dialogArea, SWT.NONE, true, true);
      contextObjectSelector.setLabel(i18n.tr("Context object"));
      contextObjectSelector.setObjectClass(AbstractObject.class);
      contextObjectSelector.setObjectId(config.getContextObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = layout.numColumns;
      contextObjectSelector.setLayoutData(gd);

		Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Dashboards"));
      gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = layout.numColumns;
		label.setLayoutData(gd);

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new BaseObjectLabelProvider());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = layout.numColumns;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 0;
		viewer.getTable().setLayoutData(gd);

      final NXCSession session = Registry.getSession();
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
      upButton.setText(i18n.tr("&Up"));
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
      downButton.setText(i18n.tr("Dow&n"));
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
      addButton.setText(i18n.tr("&Add..."));
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
      deleteButton.setText(i18n.tr("&Delete"));
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
      label.setText(i18n.tr("Display time (seconds)"));
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
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), ObjectSelectionDialog.createDashboardSelectionFilter());
		if (dlg.open() == Window.OK)
		{
			dashboardObjects.addAll(Arrays.asList(dlg.getSelectedObjects(Dashboard.class)));
	      viewer.setInput(dashboardObjects.toArray());
		}
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
      title.updateConfiguration(config);
		long[] idList = new long[dashboardObjects.size()];
		for(int i = 0; i < dashboardObjects.size(); i++)
			idList[i] = dashboardObjects.get(i).getObjectId();
		config.setDashboardObjects(idList);
      config.setContextObjectId(contextObjectSelector.getObjectId());
		config.setDisplayInterval(refreshIntervalSpinner.getSelection());
		return true;
	}
}
