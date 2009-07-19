/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author Victor
 *
 */
public class ObjectList extends Composite
{
	// Options
	public static final int NONE = 0;
	public static final int CHECKBOXES = 0x01;

	private ObjectListFilter filter;
	private boolean filterEnabled = true;
	private TableViewer objectList;
	private Composite filterArea;
	private Label filterLabel;
	private Text filterText;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectList(Composite parent, int style, int options)
	{
		super(parent, style);

		FormLayout formLayout = new FormLayout();
		setLayout(formLayout);
		
		NXCSession session = NXMCSharedData.getInstance().getSession();

		objectList = new TableViewer(this, SWT.SINGLE | SWT.FULL_SELECTION | (((options & CHECKBOXES) == CHECKBOXES) ? SWT.CHECK : 0));
		objectList.setContentProvider(new ArrayContentProvider());
		objectList.setLabelProvider(new WorkbenchLabelProvider());
		objectList.setComparator(new ObjectTreeComparator());
		filter = new ObjectListFilter();
		objectList.addFilter(filter);
		objectList.setInput(session.getAllObjects());
		
		filterArea = new Composite(this, SWT.NONE);
		
		filterLabel = new Label(filterArea, SWT.NONE);
		filterLabel.setText(Messages.getString("ObjectList.filter")); //$NON-NLS-1$
		
		filterText = new Text(filterArea, SWT.BORDER);
		filterText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		filterText.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				filter.setFilterString(filterText.getText());
				objectList.refresh(false);
			}
		});
		
		filterArea.setLayout(new GridLayout(2, false));
		
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(filterArea);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectList.getControl().setLayoutData(fd);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		filterArea.setLayoutData(fd);
		
		// Add client library listener
		session.addListener(new NXCListener() {
			@Override
			public void notificationHandler(NXCNotification n)
			{
				if (n.getCode() == NXCNotification.OBJECT_CHANGED)
				{
					new UIJob("Update object list") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							objectList.setInput(NXMCSharedData.getInstance().getSession().getAllObjects());
							objectList.refresh();
							return Status.OK_STATUS;
						}
					}.schedule();
				}
			}
		});
	}
	
	
	/**
	 * Enable or disable filter
	 * 
	 * @param enable New filter state
	 */
	public void enableFilter(boolean enable)
	{
		filterEnabled = enable;
		filterArea.setVisible(filterEnabled);
		FormData fd = (FormData)objectList.getControl().getLayoutData();
		fd.top = enable ? new FormAttachment(filterArea) : new FormAttachment(0, 0);
		layout();
	}


	/**
	 * @return the filterEnabled
	 */
	public boolean isFilterEnabled()
	{
		return filterEnabled;
	}
	
	
	/**
	 * Set filter text
	 * 
	 * @param text New filter text
	 */
	public void setFilter(final String text)
	{
		filterText.setText(text);
	}

	
	/**
	 * Get filter text
	 * 
	 * @return Current filter text
	 */
	public String getFilter()
	{
		return filterText.getText();
	}

	
	/**
	 * Get underlying table control
	 * 
	 * @return Underlying table control
	 */
	public Control getTableControl()
	{
		return objectList.getControl();
	}
	
	
	/**
	 * Get underlying table viewer
	 *
	 * @return Underlying table viewer
	 */
	public TableViewer getTableViewer()
	{
		return objectList;
	}
}
