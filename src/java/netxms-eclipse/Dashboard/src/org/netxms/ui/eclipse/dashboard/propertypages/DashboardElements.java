/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.propertypages;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.dialogs.AddDashboardElementDlg;
import org.netxms.ui.eclipse.dashboard.dialogs.EditDashboardElement;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * @author Victor
 *
 */
public class DashboardElements extends PropertyPage
{
	private Dashboard object;
	private LabeledText columnCount;
	private SortableTableViewer viewer;
	private Button addButton;
	private Button editButton;
	private Button deleteButton;
	private List<DashboardElement> elements;
	
	/* (non-Javadoc)
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
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      columnCount = new LabeledText(dialogArea, SWT.NONE);
      columnCount.setLabel("Number of columns");
      columnCount.setText(Integer.toString(object.getNumColumns()));
      
      final String[] columnNames = { "Type", "Span", "Alignment" };
      final int[] columnWidths = { 200, 150, 200 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                       SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      //viewer.setLabelProvider(new AttrListLabelProvider());
      //viewer.setComparator(new AttrViewerComparator());
      
      elements = copyElements(object.getElements());
      viewer.setInput(elements.toArray());
      
      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      viewer.getControl().setLayoutData(gridData);
      
      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gridData);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText("&Add...");
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addNewElement();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});

      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText("&Modify...");
      editButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editElement();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});

      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText("&Delete");
      deleteButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteElements();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
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
		int numColumns;
		try
		{
			numColumns = Integer.parseInt(columnCount.getText());
			if ((numColumns < 1) || (numColumns > 8))
				throw new NumberFormatException();
		}
		catch(NumberFormatException e)
		{
			MessageDialog.openError(getShell(), "Error", "Please enter valid integer in range 1 .. 8 as number of columns");
			return false;
		}
		
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setDashboardElements(elements);
		md.setColumnCount(numColumns);
		
		if (isApply)
			setValid(false);
		
		new ConsoleJob("Update dashboard configuration", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				((NXCSession)ConsoleSharedData.getSession()).modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update dashboard configuration";
			}

			/* (non-Javadoc)
			 * @see org.netxms.ui.eclipse.jobs.ConsoleJob#jobFinalize()
			 */
			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					PlatformUI.getWorkbench().getDisplay().asyncExec(new Runnable() {
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
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
		AddDashboardElementDlg dlg = new AddDashboardElementDlg(getShell());
		if (dlg.open() == Window.OK)
		{
			DashboardElement element = new DashboardElement(dlg.getElementType(), "<element>\n</element>\n");
			elements.add(element);
			viewer.setInput(elements.toArray());
			viewer.setSelection(new StructuredSelection(element));
		}
	}
	
	private void editElement()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		DashboardElement element = (DashboardElement)selection.getFirstElement();
		EditDashboardElement dlg = new EditDashboardElement(getShell(), element.getData());
		if (dlg.open() == Window.OK)
		{
			element.setData(dlg.getConfig());
			viewer.update(element, null);
		}
	}
	
	private void deleteElements()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		for(Object o : selection.toList())
		{
			elements.remove(o);
		}
		viewer.setInput(elements.toArray());
	}
}
