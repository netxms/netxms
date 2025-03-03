/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
import org.eclipse.jface.action.MenuManager;
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
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.xml.XMLTools;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.ElementCreationHandler;
import org.netxms.ui.eclipse.dashboard.ElementCreationMenuManager;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.dialogs.EditElementXmlDlg;
import org.netxms.ui.eclipse.dashboard.propertypages.helpers.DashboardElementsLabelProvider;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import com.google.gson.Gson;

/**
 * "Dashboard elements" property page for dashboard objects
 */
@SuppressWarnings("restriction")
public class DashboardElements extends PropertyPage
{
	public static final int COLUMN_TYPE = 0;
	public static final int COLUMN_SPAN = 1;
   public static final int COLUMN_HEIGHT = 2;
   public static final int COLUMN_NARROW_SCREEN = 3;
   public static final int COLUMN_TITLE = 4;

	private Dashboard object;
	private LabeledSpinner columnCount;
   private Button checkScrollable;
	private SortableTableViewer viewer;
	private Button addButton;
	private Button editButton;
	private Button editXmlButton;
   private Button duplicateButton;
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
      columnCount.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, false));

      checkScrollable = new Button(dialogArea, SWT.CHECK);
      checkScrollable.setText("&Scrollable content");
      checkScrollable.setSelection(object.isScrollable());
      checkScrollable.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, false));

      final String[] columnNames = { Messages.get().DashboardElements_Type, Messages.get().DashboardElements_Span, "Height", "Narrow screen", "Title" };
      final int[] columnWidths = { 150, 60, 90, 100, 300 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.disableSorting();
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new DashboardElementsLabelProvider());

      elements = copyElements(object.getElements());
      viewer.setInput(elements.toArray());

      GridData gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 0;
      gd.horizontalSpan = 2;
      viewer.getControl().setLayoutData(gd);

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
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      rightButtons.setLayoutData(gd);

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

      duplicateButton = new Button(rightButtons, SWT.PUSH);
      duplicateButton.setText("Du&plicate");
      duplicateButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            duplicateElements();
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
      md.setObjectFlags(checkScrollable.getSelection() ? Dashboard.SCROLLABLE : 0, Dashboard.SCROLLABLE);

		if (isApply)
			setValid(false);

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob(Messages.get().DashboardElements_JobTitle, null, Activator.PLUGIN_ID) {
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
               runInUIThread(() -> DashboardElements.this.setValid(true));
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
      MenuManager menuManager = new ElementCreationMenuManager(new ElementCreationHandler() {
         @Override
         public void elementCreated(DashboardElement element)
         {
            elements.add(element);
            viewer.setInput(elements.toArray());
            viewer.setSelection(new StructuredSelection(element));
         }
      });
      Menu menu = menuManager.createContextMenu(addButton);
      menu.setLocation(pt.x, pt.y);
      menu.setVisible(true);
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
			   Gson gson = new Gson();
            config.setLayout(gson.fromJson(element.getLayout(), DashboardElementLayout.class));

				PropertyDialog dlg = PropertyDialog.createDialogOn(getShell(), null, config);
				if (dlg.open() == Window.CANCEL)
					return;	// element creation cancelled

				element.setData(config.createJson());
            element.setLayout(gson.toJson(config.getLayout()));
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
    * Duplicate selected elements
    */
   private void duplicateElements()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      List<DashboardElement> newSelection = new ArrayList<>();
      for(Object o : selection.toList())
      {
         DashboardElement e = new DashboardElement((DashboardElement)o);
         elements.add(e);
         newSelection.add(e);
      }
      viewer.setInput(elements.toArray());
      viewer.setSelection(new StructuredSelection(newSelection));
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
