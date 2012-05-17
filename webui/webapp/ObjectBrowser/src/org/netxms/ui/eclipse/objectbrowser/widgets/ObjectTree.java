/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectbrowser.widgets;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.util.LocalSelectionTransfer;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.DND;
import org.eclipse.swt.dnd.DragSourceAdapter;
import org.eclipse.swt.dnd.DragSourceEvent;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.events.TreeEvent;
import org.eclipse.swt.events.TreeListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeItem;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeComparator;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeContentProvider;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeFilter;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeViewer;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.FilterText;

/**
 * Object tree control
 */
public class ObjectTree extends Composite
{
	private static final long serialVersionUID = 1L;

	// Options
	public static final int NONE = 0;
	public static final int CHECKBOXES = 0x01;
	public static final int MULTI = 0x02;

	private boolean filterEnabled = true;
	private boolean statusIndicatorEnabled = false;
	private ObjectTreeViewer objectTree;
	private FilterText filterText;
	private ObjectTreeFilter filter;
	private Set<Long> checkedObjects = new HashSet<Long>(0);
	private NXCListener sessionListener = null;
	private NXCSession session = null;
	private long[] expandedElements = null;
	private int changeCount = 0;
	private ObjectStatusIndicator statusIndicator = null;
	private SelectionListener statusIndicatorSelectionListener = null;
	private TreeListener statusIndicatorTreeListener;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectTree(Composite parent, int style, int options, long[] rootObjects, Set<Integer> classFilter)
	{
		super(parent, style);
		
		session = (NXCSession)ConsoleSharedData.getSession();

		FormLayout formLayout = new FormLayout();
		setLayout(formLayout);
		
		// Create filter area
		filterText = new FilterText(this, SWT.NONE);
		filterText.addModifyListener(new ModifyListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void modifyText(ModifyEvent e)
			{
				onFilterModify();
			}
		});
		filterText.setCloseAction(new Action() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				enableFilter(false);
			}
		});
		
		// Create object tree control
		objectTree = new ObjectTreeViewer(this, SWT.VIRTUAL | (((options & MULTI) == MULTI) ? SWT.MULTI : SWT.SINGLE) | (((options & CHECKBOXES) == CHECKBOXES) ? SWT.CHECK : 0));
		objectTree.setContentProvider(new ObjectTreeContentProvider(rootObjects));
		objectTree.setLabelProvider(WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider());
		objectTree.setComparator(new ObjectTreeComparator());
		filter = new ObjectTreeFilter(rootObjects, classFilter);
		objectTree.addFilter(filter);
		objectTree.setInput(session);
		
		objectTree.getControl().addListener(SWT.Selection, new Listener() {
			private static final long serialVersionUID = 1L;

			void checkItems(TreeItem item, boolean isChecked)
			{
				if (item.getData() == null)
					return;	// filtered out item
				
				item.setGrayed(false);
				item.setChecked(isChecked);
				Long id = ((GenericObject)item.getData()).getObjectId();
				if (isChecked)
					checkedObjects.add(id);
				else
					checkedObjects.remove(id);
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
				final GenericObject object = (GenericObject)item.getData();
				if (object == null)
					return;
				
				boolean isChecked = item.getChecked();
				if (isChecked)
				{
					objectTree.expandToLevel(object, TreeViewer.ALL_LEVELS);
				}
				checkItems(item, isChecked);
				checkPath(item.getParentItem(), isChecked, false);
				final Long id = object.getObjectId();
				if (isChecked)
					checkedObjects.add(id);
				else
					checkedObjects.remove(id);
			}
		});
		
		// Setup layout
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(filterText);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectTree.getTree().setLayoutData(fd);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		filterText.setLayoutData(fd);
		
		// Add client library listener
		sessionListener = new NXCListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == NXCNotification.OBJECT_CHANGED)
				{
					changeCount++;
					new UIJob(getDisplay(), "Update object tree") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							if (ObjectTree.this.isDisposed() || objectTree.getControl().isDisposed())
								return Status.OK_STATUS;
							
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
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				if ((session != null) && (sessionListener != null))
					session.removeListener(sessionListener);
			}
		});
		
		// Set initial focus to filter input line
		if (filterEnabled)
			filterText.setFocus();
		else
			enableFilter(false);	// Will hide filter area correctly
		
		enableStatusIndicator(statusIndicatorEnabled);
	}
	
	/**
	 * Enable drag support in object tree
	 */
	public void enableDragSupport()
	{
		Transfer[] transfers = new Transfer[] { LocalSelectionTransfer.getTransfer() };
		//objectTree.addDragSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new TreeDragSourceEffect(objectTree.getTree()));
		objectTree.addDragSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new DragSourceAdapter() {
			private static final long serialVersionUID = 1L;

			@Override
			public void dragStart(DragSourceEvent event)
			{
				LocalSelectionTransfer.getTransfer().setSelection(objectTree.getSelection());
				event.doit = true;
			}

			@Override
			public void dragSetData(DragSourceEvent event)
			{
				event.data = LocalSelectionTransfer.getTransfer().getSelection();
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
		filterText.setVisible(filterEnabled);
		FormData fd = (FormData)objectTree.getTree().getLayoutData();
		fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
		if (statusIndicatorEnabled)
		{
			fd = (FormData)statusIndicator.getLayoutData();
			fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
		}
		layout();
		if (enable)
			filterText.setFocus();
		else
			setFilter("");
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
		onFilterModify();
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
			expandedElements[i] = ((GenericObject)elements[i]).getObjectId();
	}
	
	/**
	 * Expand elements that was expanded when saveExpandedState() was called
	 */
	private void restoreExpandedState()
	{
		if (expandedElements == null)
			return;
		
		Object[] objects = session.findMultipleObjects(expandedElements, false).toArray();
		objectTree.setExpandedElements(objects);
	}
	
	/**
	 * Get selected object
	 * 
	 * @return ID of selected object or 0 if no objects selected
	 */
	public long getFirstSelectedObject()
	{
		IStructuredSelection selection = (IStructuredSelection)objectTree.getSelection();
		if (selection.isEmpty())
			return 0;
		return ((GenericObject)selection.getFirstElement()).getObjectId();
	}
	
	/**
	 * Get all selected objects
	 * 
	 * @return ID of selected object or 0 if no objects selected
	 */
	@SuppressWarnings("rawtypes")
	public Long[] getSelectedObjects()
	{
		IStructuredSelection selection = (IStructuredSelection)objectTree.getSelection();
		Set<Long> objects = new HashSet<Long>(selection.size());
		Iterator it = selection.iterator();
		while(it.hasNext())
		{
			objects.add(((GenericObject)it.next()).getObjectId());
		}
		return objects.toArray(new Long[objects.size()]);
	}
	
	/**
	 * Get selected object as object
	 * 
	 * @return
	 */
	public GenericObject getFirstSelectedObject2()
	{
		IStructuredSelection selection = (IStructuredSelection)objectTree.getSelection();
		return (GenericObject)selection.getFirstElement();
	}
	
	/**
	 * Refresh object tree
	 */
	public void refresh()
	{
		objectTree.setInput(session);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
	 */
	@Override
	public void setEnabled(boolean enabled)
	{
		super.setEnabled(enabled);
		objectTree.getControl().setEnabled(enabled);
		filterText.setEnabled(enabled);
	}

	/**
	 * Handler for filter modification
	 */
	private void onFilterModify()
	{
		final String text = filterText.getText();
		filter.setFilterString(text);
		objectTree.refresh(false);
		GenericObject obj = filter.getLastMatch();
		if (obj != null)
		{
			objectTree.setSelection(new StructuredSelection(obj), true);
			GenericObject parent = filter.getParent(obj);
			if (parent != null)
				objectTree.expandToLevel(parent, 1);
			objectTree.reveal(obj);
		}
	}

	/**
	 * @return the hideUnmanaged
	 */
	public boolean isHideUnmanaged()
	{
		return filter.isHideUnmanaged();
	}

	/**
	 * @param hideUnmanaged the hideUnmanaged to set
	 */
	public void setHideUnmanaged(boolean hideUnmanaged)
	{
		filter.setHideUnmanaged(hideUnmanaged);
		onFilterModify();
	}

	/**
	 * @return the hideTemplateChecks
	 */
	public boolean isHideTemplateChecks()
	{
		return filter.isHideTemplateChecks();
	}

	/**
	 * @param hideTemplateChecks the hideTemplateChecks to set
	 */
	public void setHideTemplateChecks(boolean hideTemplateChecks)
	{
		filter.setHideTemplateChecks(hideTemplateChecks);
		onFilterModify();
	}
	
	/**
	 * Set action to be executed when user press "Close" button in object filter.
	 * Default implementation will hide filter area without notifying parent.
	 * 
	 * @param action
	 */
	public void setFilterCloseAction(Action action)
	{
		filterText.setCloseAction(action);
	}
	
	/**
	 * @param enabled
	 */
	public void enableStatusIndicator(boolean enabled)
	{
		if (statusIndicatorEnabled == enabled)
			return;
		
		statusIndicatorEnabled = enabled;
		if (enabled)
		{
			statusIndicator = new ObjectStatusIndicator(this, SWT.NONE);
			FormData fd = new FormData();
			fd.left = new FormAttachment(0, 0);
			fd.top = filterEnabled ? new FormAttachment(filterText) : new FormAttachment(0, 0);
			fd.bottom = new FormAttachment(100, 0);
			statusIndicator.setLayoutData(fd);
			
			fd = (FormData)objectTree.getTree().getLayoutData();
			fd.left = new FormAttachment(statusIndicator);
			
			statusIndicatorSelectionListener = new SelectionListener() {
				private static final long serialVersionUID = 1L;

				@Override
				public void widgetSelected(SelectionEvent e)
				{
					updateStatusIndicator();
				}
				
				@Override
				public void widgetDefaultSelected(SelectionEvent e)
				{
					updateStatusIndicator();
				}
			};
			objectTree.getTree().getVerticalBar().addSelectionListener(statusIndicatorSelectionListener);
			
			statusIndicatorTreeListener = new TreeListener() {
				private static final long serialVersionUID = 1L;

				@Override
				public void treeCollapsed(TreeEvent e)
				{
					updateStatusIndicator();
				}

				@Override
				public void treeExpanded(TreeEvent e)
				{
					updateStatusIndicator();
				}
			};
			objectTree.getTree().addTreeListener(statusIndicatorTreeListener);
			
			updateStatusIndicator();
		}
		else
		{
			objectTree.getTree().getVerticalBar().removeSelectionListener(statusIndicatorSelectionListener);
			objectTree.getTree().removeTreeListener(statusIndicatorTreeListener);
			statusIndicator.dispose();
			statusIndicator = null;
			statusIndicatorSelectionListener = null;
			statusIndicatorTreeListener = null;

			FormData fd = (FormData)objectTree.getTree().getLayoutData();
			fd.left = new FormAttachment(0, 0);
		}
		layout(true, true);
	}
	
	/**
	 * @return
	 */
	public boolean isStatusIndicatorEnabled()
	{
		return statusIndicatorEnabled;
	}

	/**
	 * Update status indicator
	 */
	private void updateStatusIndicator()
	{
		statusIndicator.refresh(objectTree);
	}
}
