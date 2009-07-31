/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser.widgets;

import java.util.HashSet;
import java.util.Set;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeItem;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeComparator;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeContentProvider;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeFilter;
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
	private Set<Long> checkedObjects = new HashSet<Long>(0);
	private NXCListener sessionListener = null;
	private NXCSession session = null;
	private long[] expandedElements = null;
	private int changeCount = 0;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectTree(Composite parent, int style, int options, long[] rootObjects)
	{
		super(parent, style);
		
		session = NXMCSharedData.getInstance().getSession();

		FormLayout formLayout = new FormLayout();
		setLayout(formLayout);
		
		// Create filter area
		filterArea = new Composite(this, SWT.NONE);
		
		filterLabel = new Label(filterArea, SWT.NONE);
		filterLabel.setText(Messages.getString("ObjectTree.filter")); //$NON-NLS-1$
		
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
					objectTree.setSelection(new StructuredSelection(obj), true);
					NXCObject parent = filter.getParent(obj);
					if (parent != null)
						objectTree.expandToLevel(parent, 1);
					objectTree.reveal(obj);
				}
			}
		});
		
		filterArea.setLayout(new GridLayout(2, false));
		
		// Create object tree control
		objectTree = new TreeViewer(this, SWT.VIRTUAL | SWT.SINGLE | (((options & CHECKBOXES) == CHECKBOXES) ? SWT.CHECK : 0));
		objectTree.setContentProvider(new ObjectTreeContentProvider(rootObjects));
		objectTree.setLabelProvider(WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider());
		objectTree.setComparator(new ObjectTreeComparator());
		filter = new ObjectTreeFilter(rootObjects);
		objectTree.addFilter(filter);
		objectTree.setInput(session);
		
		objectTree.getControl().addListener(SWT.Selection, new Listener() {
			void checkItems(TreeItem item, boolean isChecked)
			{
				item.setGrayed(false);
				item.setChecked(isChecked);
				TreeItem[] items = item.getItems();
				for(int i = 0; i < items.length; i++)
					checkItems(items[i], isChecked);
			}

			void checkPath(TreeItem item, boolean checked, boolean grayed)
			{
				if (item == null)
					return;
				if (grayed)
				{
					checked = true;
				}
				else
				{
					int index = 0;
					TreeItem[] items = item.getItems();
					while(index < items.length)
					{
						TreeItem child = items[index];
						if (child.getGrayed() || checked != child.getChecked())
						{
							checked = grayed = true;
							break;
						}
						index++;
					}
				}
				item.setChecked(checked);
				item.setGrayed(grayed);
				checkPath(item.getParentItem(), checked, grayed);
			}

			@Override
			public void handleEvent(Event event)
			{
				if (event.detail != SWT.CHECK)
					return;
				
				TreeItem item = (TreeItem)event.item;
				boolean isChecked = item.getChecked();
				checkItems(item, isChecked);
				checkPath(item.getParentItem(), isChecked, false);
				Long id = ((NXCObject)item.getData()).getObjectId();
				if (isChecked)
					checkedObjects.add(id);
				else
					checkedObjects.remove(id);
			}
		});
		
		// Setup layout
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
		sessionListener = new NXCListener() {
			@Override
			public void notificationHandler(NXCNotification n)
			{
				if (n.getCode() == NXCNotification.OBJECT_CHANGED)
				{
					changeCount++;
					new UIJob("Update object tree") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							changeCount--;
							if (changeCount <= 0)
							{
								saveExpandedState();
								objectTree.refresh();
								restoreExpandedState();
							}
							return Status.OK_STATUS;
						}
					}.schedule(500);
				}
			}
		};
		session.addListener(sessionListener);
		
		// Set dispose listener
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				if ((session != null) && (sessionListener != null))
					session.removeListener(sessionListener);
			}
		});
		
		// Set initial focus to filter input line
		filterText.setFocus();
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
	 * @return IDs of objects checked in the tree
	 */
	public Long[] getCheckedObjects()
	{
		return checkedObjects.toArray(new Long[checkedObjects.size()]);
	}
	
	/**
	 * Save expanded elements
	 */
	private void saveExpandedState()
	{
		Object[] elements = objectTree.getExpandedElements();
		expandedElements = new long[elements.length];
		for(int i = 0; i < elements.length; i++)
			expandedElements[i] = ((NXCObject)elements[i]).getObjectId();
	}
	
	/**
	 * Expand elements that was expanded when saveExpandedState() was called
	 */
	private void restoreExpandedState()
	{
		if (expandedElements == null)
			return;
		
		NXCObject[] objects = session.findMultipleObjects(expandedElements);
		objectTree.setExpandedElements(objects);
	}
}
