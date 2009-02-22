/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author victor
 *
 */
public class ObjectTree extends Composite
{
	// Options
	public static final int NONE = 0;
	public static final int CHECKBOXES = 0x01;

	private boolean filterEnabled = true;
	private TreeViewer objectTree;
	private Composite filterArea;
	private Label filterLabel;
	private Text filterText;
	private ObjectTreeFilter filter;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectTree(Composite parent, int style, int options)
	{
		super(parent, style);
		
		FormLayout formLayout = new FormLayout();
		setLayout(formLayout);
		
		NXCSession session = NXMCSharedData.getInstance().getSession();

		objectTree = new TreeViewer(this, SWT.VIRTUAL | SWT.SINGLE | (((options & CHECKBOXES) == CHECKBOXES) ? SWT.CHECK : 0));
		objectTree.setContentProvider(new ObjectTreeContentProvider());
		objectTree.setLabelProvider(new ObjectTreeLabelProvider());
		objectTree.setComparator(new ObjectTreeComparator());
		filter = new ObjectTreeFilter();
		objectTree.addFilter(filter);
		objectTree.setInput(session);
		
		filterArea = new Composite(this, SWT.NONE);
		
		filterLabel = new Label(filterArea, SWT.NONE);
		filterLabel.setText("Filter:");
		
		filterText = new Text(filterArea, SWT.BORDER);
		filterText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		filterText.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				filter.setFilterString(filterText.getText());
				objectTree.refresh(false);
				NXCObject obj = filter.getLastMatch();
				if (obj != null)
				{
					//objectTree.reveal(obj);
					objectTree.expandToLevel(obj, TreeViewer.ALL_LEVELS);
				}
			}
		});
		
		filterArea.setLayout(new GridLayout(2, false));
		
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(filterArea);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectTree.getTree().setLayoutData(fd);
		
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
					new UIJob("Update object tree") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							objectTree.refresh();
							return Status.OK_STATUS;
						}
					}.schedule();
				}
			}
		});
	}
	
	
	/**
	 * Get underlying tree control
	 * 
	 * @return Underlying tree control
	 */
	public Tree getTreeControl()
	{
		return objectTree.getTree();
	}
	
	
	/**
	 * Get underlying tree viewer
	 *
	 * @return Underlying tree viewer
	 */
	public TreeViewer getTreeViewer()
	{
		return objectTree;
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
		FormData fd = (FormData)objectTree.getTree().getLayoutData();
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
}
