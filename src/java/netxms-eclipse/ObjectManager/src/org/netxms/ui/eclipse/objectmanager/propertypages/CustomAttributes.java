/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager.propertypages;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
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
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.AttrListLabelProvider;
import org.netxms.ui.eclipse.objectmanager.AttrViewerComparator;
import org.netxms.ui.eclipse.objectmanager.dialogs.AttributeEditDialog;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.SortableTableViewer;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class CustomAttributes extends PropertyPage
{
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VALUE = 1;
	
	private NXCObject object = null;
	private SortableTableViewer viewer;
	private Button addButton;
	private Button editButton;
	private Button deleteButton;
	private Map<String, String> attributes = null;
	private boolean isModified = false;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (NXCObject)getElement().getAdapter(NXCObject.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
      dialogArea.setLayout(layout);
      
      final String[] columnNames = { "Name", "Value" };
      final int[] columnWidths = { 150, 250 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                       SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AttrListLabelProvider());
      viewer.setComparator(new AttrViewerComparator());
      
      attributes = new HashMap<String, String>(object.getCustomAttributes());
      viewer.setInput(attributes.entrySet());
      
      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
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
      addButton.setText("Add...");
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				final AttributeEditDialog dlg = new AttributeEditDialog(CustomAttributes.this.getShell(), null, null);
				if (dlg.open() == Window.OK)
				{
					if (attributes.containsKey(dlg.getAttrName()))
					{
						MessageDialog.openWarning(CustomAttributes.this.getShell(), "Warning", "Attribute named " + dlg.getAttrName() + " already exists");
					}
					else
					{
						attributes.put(dlg.getAttrName(), dlg.getAttrValue());
				      viewer.setInput(attributes.entrySet());
				      CustomAttributes.this.isModified = true;
					}
				}
			}
      });
		
      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText("Modify...");
      editButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@SuppressWarnings("unchecked")
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				if (selection.size() == 1)
				{
					Entry<String, String> element = (Entry<String, String>)selection.getFirstElement();
					final AttributeEditDialog dlg = new AttributeEditDialog(CustomAttributes.this.getShell(), element.getKey(), element.getValue());
					if (dlg.open() == Window.OK)
					{
						attributes.put(dlg.getAttrName(), dlg.getAttrValue());
				      viewer.setInput(attributes.entrySet());
				      CustomAttributes.this.isModified = true;
					}
				}
			}
      });
		
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText("Delete");
      deleteButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@SuppressWarnings("unchecked")
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				Iterator<Entry<String, String>> it = selection.iterator();
				if (it.hasNext())
				{
					while(it.hasNext())
					{
						Entry<String, String> element = it.next();
						attributes.remove(element.getKey());
					}
			      viewer.setInput(attributes.entrySet());
			      CustomAttributes.this.isModified = true;
				}
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
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (!isModified)
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		new Job("Update custom attributes") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					if (object != null)
					{
						NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
						md.setCustomAttributes(attributes);
						NXMCSharedData.getInstance().getSession().modifyObject(md);
					}
					isModified = false;
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
					                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
					                    "Cannot update custom attributes: " + e.getMessage(), e);
				}

				if (isApply)
				{
					new UIJob("Update \"Custom Attributes\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							CustomAttributes.this.setValid(true);
							return Status.OK_STATUS;
						}
					}.schedule();
				}

				return status;
			}
		}.schedule();
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
		applyChanges(false);
		return true;
	}
}
