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
import org.eclipse.core.internal.runtime.AdapterManager;
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
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.dialogs.EditElementXmlDlg;
import org.netxms.ui.eclipse.dashboard.propertypages.helpers.DashboardElementsLabelProvider;
import org.netxms.ui.eclipse.dashboard.widgets.DashboardControl;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Dashboard elements" property page for dashboard objects
 */
@SuppressWarnings("restriction")
public class DashboardElements extends PropertyPage
{
	public static final int COLUMN_TYPE = 0;
	public static final int COLUMN_SPAN = 1;
	public static final int COLUMN_HEIGHT = 2;

	private Dashboard object;
	private LabeledSpinner columnCount;
	private SortableTableViewer viewer;
	private Button addButton;
	private Button editButton;
	private Button editXmlButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;
	private List<DashboardElement> elements;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (Dashboard)getElement().getAdapter(Dashboard.class);
		if (object == null)	// Paranoid check
			return dialogArea;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);

      columnCount = new LabeledSpinner(dialogArea, SWT.NONE);
      columnCount.setLabel(Messages.get().DashboardElements_NumColumns);
      columnCount.setRange(1, 128);
      columnCount.setSelection(object.getNumColumns());
      GridData gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      gridData.horizontalSpan = 2;
      columnCount.setLayoutData(gridData);
      
      final String[] columnNames = { Messages.get().DashboardElements_Type, Messages.get().DashboardElements_Span, "Height" };
      final int[] columnWidths = { 150, 60, 90 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                       SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new DashboardElementsLabelProvider());
      
      elements = copyElements(object.getElements());
      viewer.setInput(elements.toArray());
      
      gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      gridData.horizontalSpan = 2;
      viewer.getControl().setLayoutData(gridData);

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
      upButton.setText(Messages.get().DashboardElements_Up);
      upButton.setEnabled(false);
      upButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				moveUp();
			}
      });

      downButton = new Button(leftButtons, SWT.PUSH);
      downButton.setText(Messages.get().DashboardElements_Down);
      downButton.setEnabled(false);
      downButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveDown();
         }
      });

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
      addButton.setText(Messages.get().DashboardElements_Add);
      addButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addNewElement();
			}
		});

      editButton = new Button(rightButtons, SWT.PUSH);
      editButton.setText(Messages.get().DashboardElements_Edit);
      editButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editElement();
			}
		});

      editXmlButton = new Button(rightButtons, SWT.PUSH);
      editXmlButton.setText(Messages.get().DashboardElements_EditXML);
      editXmlButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editElementXml();
			}
		});

      deleteButton = new Button(rightButtons, SWT.PUSH);
      deleteButton.setText(Messages.get().DashboardElements_Delete);
      deleteButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteElements();
			}
		});

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				int index = elements.indexOf(selection.getFirstElement());
				upButton.setEnabled((selection.size() == 1) && (index > 0));
				downButton.setEnabled((selection.size() == 1) && (index >= 0) && (index < elements.size() - 1));
				editButton.setEnabled(selection.size() == 1);
				deleteButton.setEnabled(selection.size() > 0);
			}
      });
      
      viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editButton.notifyListeners(SWT.Selection, new Event());
			}
      });
      
		return dialogArea;
	}

	/**
	 * @param src
	 * @return
	 */
	private static List<DashboardElement> copyElements(List<DashboardElement> src)
	{
		List<DashboardElement> dst = new ArrayList<DashboardElement>(src.size());
		for(DashboardElement e : src)
			dst.add(new DashboardElement(e));
		return dst;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	private boolean applyChanges(final boolean isApply)
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setDashboardElements(elements);
		md.setColumnCount(columnCount.getSelection());
		
		if (isApply)
			setValid(false);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().DashboardElements_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().DashboardElements_JobError;
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
							DashboardElements.this.setValid(true);
						}
					});
				}
			}	
		}.start();
		
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

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
	}

	/**
	 * Add new dashboard element
	 */
	private void addNewElement()
	{
      Rectangle rect = addButton.getBounds();
      Point pt = new Point(rect.x, rect.y + rect.height);
      pt = addButton.getParent().toDisplay(pt);
      Menu m = buildElementTypeSelectionMenu(addButton);
      m.setLocation(pt.x, pt.y);
      m.setVisible(true);
	}

   /**
    * Build menu for selecting dashboard element type.
    *
    * @param parent
    * @return
    */
   private Menu buildElementTypeSelectionMenu(Control parent)
   {
      Menu menu = new Menu(parent);

      /* charts */
      Menu chartsMenu = new Menu(menu);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_LineChart, DashboardElement.LINE_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_BarChart, DashboardElement.BAR_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_PieChart, DashboardElement.PIE_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_TubeChart, DashboardElement.TUBE_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_GaugeChart, DashboardElement.DIAL_CHART);
      new MenuItem(chartsMenu, SWT.SEPARATOR);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_BarChartForTable, DashboardElement.TABLE_BAR_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_PieChartForTable, DashboardElement.TABLE_PIE_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_TubeChartForTable, DashboardElement.TABLE_TUBE_CHART);
      new MenuItem(chartsMenu, SWT.SEPARATOR);
      addTypeToSelectionMenu(chartsMenu, "Scripted bar chart", DashboardElement.SCRIPTED_BAR_CHART);
      addTypeToSelectionMenu(chartsMenu, "Scripted pie chart", DashboardElement.SCRIPTED_PIE_CHART);
      new MenuItem(chartsMenu, SWT.SEPARATOR);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_StatusChart, DashboardElement.STATUS_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_AvailabilityChart, DashboardElement.AVAILABLITY_CHART);

      MenuItem chartsMenuItem = new MenuItem(menu, SWT.CASCADE);
      chartsMenuItem.setText("&Charts");
      chartsMenuItem.setMenu(chartsMenu);

      /* tables */
      Menu tablesMenu = new Menu(menu);
      addTypeToSelectionMenu(tablesMenu, Messages.get().AddDashboardElementDlg_AlarmViewer, DashboardElement.ALARM_VIEWER);
      addTypeToSelectionMenu(tablesMenu, Messages.get().AddDashboardElementDlg_TableValue, DashboardElement.TABLE_VALUE);
      addTypeToSelectionMenu(tablesMenu, Messages.get().AddDashboardElementDlg_DciSummaryTable, DashboardElement.DCI_SUMMARY_TABLE);
      addTypeToSelectionMenu(tablesMenu, "Object query", DashboardElement.OBJECT_QUERY);

      MenuItem tablesMenuItem = new MenuItem(menu, SWT.CASCADE);
      tablesMenuItem.setText("&Tables");
      tablesMenuItem.setMenu(tablesMenu);

      /* maps */
      Menu mapsMenu = new Menu(menu);
      addTypeToSelectionMenu(mapsMenu, Messages.get().AddDashboardElementDlg_NetworkMap, DashboardElement.NETWORK_MAP);
      addTypeToSelectionMenu(mapsMenu, "Service components map", DashboardElement.SERVICE_COMPONENTS);
      addTypeToSelectionMenu(mapsMenu, Messages.get().AddDashboardElementDlg_StatusMap, DashboardElement.STATUS_MAP);
      addTypeToSelectionMenu(mapsMenu, Messages.get().AddDashboardElementDlg_GeoMap, DashboardElement.GEO_MAP);

      MenuItem mapsMenuItem = new MenuItem(menu, SWT.CASCADE);
      mapsMenuItem.setText("&Maps");
      mapsMenuItem.setMenu(mapsMenu);

      /* monitors */
      Menu monitorsMenu = new Menu(menu);
      addTypeToSelectionMenu(monitorsMenu, "Event monitor", DashboardElement.EVENT_MONITOR);
      addTypeToSelectionMenu(monitorsMenu, "SNMP trap monitor", DashboardElement.SNMP_TRAP_MONITOR);
      addTypeToSelectionMenu(monitorsMenu, "Syslog monitor", DashboardElement.SYSLOG_MONITOR);

      MenuItem monitorsMenuItem = new MenuItem(menu, SWT.CASCADE);
      monitorsMenuItem.setText("M&onitors");
      monitorsMenuItem.setMenu(monitorsMenu);

      /* others */
      Menu othersMenu = new Menu(menu);
      addTypeToSelectionMenu(othersMenu, Messages.get().AddDashboardElementDlg_StatusIndicator, DashboardElement.STATUS_INDICATOR);
      addTypeToSelectionMenu(othersMenu, "Port view", DashboardElement.PORT_VIEW);
      addTypeToSelectionMenu(othersMenu, "Rack diagram", DashboardElement.RACK_DIAGRAM);
      addTypeToSelectionMenu(othersMenu, "Object Tools", DashboardElement.OBJECT_TOOLS);
      addTypeToSelectionMenu(othersMenu, Messages.get().AddDashboardElementDlg_CustomWidget, DashboardElement.CUSTOM);

      MenuItem othersMenuItem = new MenuItem(menu, SWT.CASCADE);
      othersMenuItem.setText("&Others");
      othersMenuItem.setMenu(othersMenu);

      /* other top level items */
      new MenuItem(menu, SWT.SEPARATOR);
      addTypeToSelectionMenu(menu, Messages.get().AddDashboardElementDlg_Dashboard, DashboardElement.DASHBOARD);
      addTypeToSelectionMenu(menu, Messages.get().AddDashboardElementDlg_WebPage, DashboardElement.WEB_PAGE);
      new MenuItem(menu, SWT.SEPARATOR);
      addTypeToSelectionMenu(menu, Messages.get().AddDashboardElementDlg_Label, DashboardElement.LABEL);
      addTypeToSelectionMenu(menu, Messages.get().AddDashboardElementDlg_Separator, DashboardElement.SEPARATOR);
      return menu;
   }

   /**
    * Add element type to selection menu.
    *
    * @param menu menu to add
    * @param name type name
    * @param type type code
    */
   private void addTypeToSelectionMenu(Menu menu, String name, int type)
   {
      MenuItem item = new MenuItem(menu, SWT.PUSH);
      item.setText(name);
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            DashboardElement element = new DashboardElement(type, getDefaultElementConfig(type));
            elements.add(element);
            viewer.setInput(elements.toArray());
            viewer.setSelection(new StructuredSelection(element));
         }
      });
   }

   /**
    * Get default configuration for given element type.
    *
    * @param type element type
    * @return default configuration
    */
   private static String getDefaultElementConfig(int type)
   {
      switch(type)
      {
         case DashboardElement.BAR_CHART:
         case DashboardElement.PIE_CHART:
         case DashboardElement.TUBE_CHART:
         case DashboardElement.SCRIPTED_BAR_CHART:
         case DashboardElement.SCRIPTED_PIE_CHART:
            return DashboardControl.DEFAULT_CHART_CONFIG;
         case DashboardElement.LINE_CHART:
            return DashboardControl.DEFAULT_LINE_CHART_CONFIG;
         case DashboardElement.DIAL_CHART:
            return DashboardControl.DEFAULT_DIAL_CHART_CONFIG;
         case DashboardElement.AVAILABLITY_CHART:
            return DashboardControl.DEFAULT_AVAILABILITY_CHART_CONFIG;
         case DashboardElement.TABLE_BAR_CHART:
         case DashboardElement.TABLE_PIE_CHART:
         case DashboardElement.TABLE_TUBE_CHART:
            return DashboardControl.DEFAULT_TABLE_CHART_CONFIG;
         case DashboardElement.LABEL:
            return DashboardControl.DEFAULT_LABEL_CONFIG;
         case DashboardElement.ALARM_VIEWER:
         case DashboardElement.EVENT_MONITOR:
         case DashboardElement.SYSLOG_MONITOR:
         case DashboardElement.SNMP_TRAP_MONITOR:
         case DashboardElement.STATUS_INDICATOR:
         case DashboardElement.STATUS_MAP:
         case DashboardElement.DASHBOARD:
         case DashboardElement.RACK_DIAGRAM:
            return DashboardControl.DEFAULT_OBJECT_REFERENCE_CONFIG;
         case DashboardElement.NETWORK_MAP:
         case DashboardElement.SERVICE_COMPONENTS:
            return DashboardControl.DEFAULT_NETWORK_MAP_CONFIG;
         case DashboardElement.GEO_MAP:
            return DashboardControl.DEFAULT_GEO_MAP_CONFIG;
         case DashboardElement.WEB_PAGE:
            return DashboardControl.DEFAULT_WEB_PAGE_CONFIG;
         case DashboardElement.TABLE_VALUE:
            return DashboardControl.DEFAULT_TABLE_VALUE_CONFIG;
         case DashboardElement.DCI_SUMMARY_TABLE:
            return DashboardControl.DEFAULT_SUMMARY_TABLE_CONFIG;
         default:
            return "<element>\n</element>"; //$NON-NLS-1$
      }
   }

	/**
	 * Edit selected element
	 */
	private void editElement()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() != 1)
			return;
		
		DashboardElement element = (DashboardElement)selection.getFirstElement();
		DashboardElementConfig config = (DashboardElementConfig)AdapterManager.getDefault().getAdapter(element, DashboardElementConfig.class);
		if (config != null)
		{
			try
			{
				config.setLayout(DashboardElementLayout.createFromXml(element.getLayout()));
				
				PropertyDialog dlg = PropertyDialog.createDialogOn(getShell(), null, config);
				if (dlg.open() == Window.CANCEL)
					return;	// element creation cancelled
				
				element.setData(config.createXml());
				element.setLayout(config.getLayout().createXml());
				viewer.update(element, null);
			}
			catch(Exception e)
			{
				MessageDialogHelper.openError(getShell(), Messages.get().DashboardElements_InternalErrorTitle, Messages.get().DashboardElements_InternalErrorText + e.getMessage());
			}
		}
		else
		{
			MessageDialogHelper.openError(getShell(), Messages.get().DashboardElements_InternalErrorTitle, Messages.get().DashboardElements_InternalErrorText2);
		}
	}
	
	/**
	 * Edit selected element's configuration directly as XML
	 */
	private void editElementXml()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() != 1)
			return;
		
		DashboardElement element = (DashboardElement)selection.getFirstElement();
		EditElementXmlDlg dlg = new EditElementXmlDlg(getShell(), element.getData());
		if (dlg.open() == Window.OK)
		{
			element.setData(dlg.getValue());
			viewer.update(element, null);
		}
	}
	
	/**
	 * Delete selected elements
	 */
	private void deleteElements()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		for(Object o : selection.toList())
		{
			elements.remove(o);
		}
		viewer.setInput(elements.toArray());
	}

	/**
	 * Move currently selected element up
	 */
	private void moveUp()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() == 1)
		{
			DashboardElement element = (DashboardElement)selection.getFirstElement();
			
			int index = elements.indexOf(element);
			if (index > 0)
			{
				Collections.swap(elements, index - 1, index);
		      viewer.setInput(elements.toArray());
		      viewer.setSelection(new StructuredSelection(element));
			}
		}
	}
	
	/**
	 * Move currently selected element down
	 */
	private void moveDown()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() == 1)
		{
			DashboardElement element = (DashboardElement)selection.getFirstElement();
			
			int index = elements.indexOf(element);
			if ((index < elements.size() - 1) && (index >= 0))
			{
				Collections.swap(elements, index + 1, index);
		      viewer.setInput(elements.toArray());
		      viewer.setSelection(new StructuredSelection(element));
			}
		}
	}
}
