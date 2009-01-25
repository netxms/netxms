/**
 * 
 */
package org.netxms.nxmc.objectbrowser;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.core.extensionproviders.NXMCSharedData;

/**
 * @author victor
 *
 */
public class ObjectTree extends Composite
{
	private TreeViewer objectTree;

	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectTree(Composite parent, int style)
	{
		super(parent, style);
		
		FormLayout formLayout = new FormLayout();
		setLayout(formLayout);
		
		NXCSession session = NXMCSharedData.getSession();

		objectTree = new TreeViewer(this, SWT.VIRTUAL | SWT.SINGLE);
		objectTree.setContentProvider(new ObjectTreeContentProvider());
		objectTree.setLabelProvider(new ObjectTreeLabelProvider());
		objectTree.setComparator(new ObjectTreeComparator());
		objectTree.setInput(session);
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectTree.getTree().setLayoutData(fd);
		
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
}
