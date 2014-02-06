/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.util.LocalSelectionTransfer;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreePath;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.ViewerDropAdapter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.DND;
import org.eclipse.swt.dnd.DragSourceAdapter;
import org.eclipse.swt.dnd.DragSourceEvent;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.dnd.TransferData;
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenListener;
import org.netxms.ui.eclipse.objectbrowser.api.SubtreeType;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.views.ObjectBrowser;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectFilter;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeComparator;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeContentProvider;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeViewer;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.FilterText;

/**
 * Object tree control
 */
public class ObjectTree extends Composite
{
	// Options
	public static final int NONE = 0;
	public static final int CHECKBOXES = 0x01;
	public static final int MULTI = 0x02;

	private boolean filterEnabled = true;
	private boolean statusIndicatorEnabled = false;
	private ObjectTreeViewer objectTree;
	private FilterText filterText;
	private ObjectFilter filter;
	private Set<Long> checkedObjects = new HashSet<Long>(0);
	private NXCListener sessionListener = null;
	private NXCSession session = null;
	private TreePath[] expandedPaths = null;
	private int changeCount = 0;
	private ObjectStatusIndicator statusIndicator = null;
	private SelectionListener statusIndicatorSelectionListener = null;
	private TreeListener statusIndicatorTreeListener;
	private Set<ObjectOpenListener> openListeners = new HashSet<ObjectOpenListener>(0);
	
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
			@Override
			public void modifyText(ModifyEvent e)
			{
				onFilterModify();
			}
		});
		filterText.setCloseAction(new Action() {
			@Override
			public void run()
			{
				enableFilter(false);
			}
		});
		
		// Create object tree control
		objectTree = new ObjectTreeViewer(this, SWT.VIRTUAL | (((options & MULTI) == MULTI) ? SWT.MULTI : SWT.SINGLE) | (((options & CHECKBOXES) == CHECKBOXES) ? SWT.CHECK : 0));
		objectTree.setUseHashlookup(true);
		objectTree.setContentProvider(new ObjectTreeContentProvider(rootObjects));
		objectTree.setLabelProvider(WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider());
		objectTree.setComparator(new ObjectTreeComparator());
		filter = new ObjectFilter(rootObjects, null, classFilter);
		objectTree.addFilter(filter);
		objectTree.setInput(session);
		
		objectTree.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				TreeItem[] items = objectTree.getTree().getSelection();
				if (items.length == 1)
				{
					// Call open handlers. If open event processed by handler, openObject will return true
					if (!openObject((AbstractObject)items[0].getData()))
					{
						objectTree.toggleItemExpandState(items[0]);
					}
				}
			}
		});
		
		objectTree.getControl().addListener(SWT.Selection, new Listener() {
			void checkItems(TreeItem item, boolean isChecked)
			{
				if (item.getData() == null)
					return;	// filtered out item
				
				item.setGrayed(false);
				item.setChecked(isChecked);
				Long id = ((AbstractObject)item.getData()).getObjectId();
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
				final AbstractObject object = (AbstractObject)item.getData();
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
				if ((n.getCode() == NXCNotification.OBJECT_CHANGED) || (n.getCode() == NXCNotification.OBJECT_DELETED))
				{
					changeCount++;
					new UIJob(getDisplay(), Messages.get().ObjectTree_JobTitle) {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							if (ObjectTree.this.isDisposed() || objectTree.getControl().isDisposed())
								return Status.OK_STATUS;
							
							changeCount--;
							if (changeCount <= 0)
							{
								objectTree.getTree().setRedraw(false);
								saveExpandedState();
								objectTree.refresh();
								if (statusIndicatorEnabled)
									updateStatusIndicator();
								restoreExpandedState();
								objectTree.getTree().setRedraw(true);
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
		objectTree.addDragSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new DragSourceAdapter() {
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
			setFilter(""); //$NON-NLS-1$
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
		expandedPaths = objectTree.getExpandedTreePaths();
	}
	
	/**
	 * Expand elements that was expanded when saveExpandedState() was called
	 */
	private void restoreExpandedState()
	{
		if (expandedPaths == null)
			return;
		
		for(int i = 0; i < expandedPaths.length; i++)
		{
			TreePath p = expandedPaths[i];
			Object[] segments = new Object[p.getSegmentCount()];
			for(int j = 0; j < p.getSegmentCount(); j++)
			{
				AbstractObject object = (AbstractObject)p.getSegment(j);
				segments[j] = session.findObjectById(object.getObjectId());
				if (segments[j] == null)
					segments[j] = object;
			}
			expandedPaths[i] = new TreePath(segments);
		}
		objectTree.setExpandedTreePaths(expandedPaths);
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
		return ((AbstractObject)selection.getFirstElement()).getObjectId();
	}
	
	/**
	 * Get all selected objects
	 * 
	 * @return ID of selected object or 0 if no objects selected
	 */
	public Long[] getSelectedObjects()
	{
		IStructuredSelection selection = (IStructuredSelection)objectTree.getSelection();
		Set<Long> objects = new HashSet<Long>(selection.size());
      Iterator<?> it = selection.iterator();
		while(it.hasNext())
		{
			objects.add(((AbstractObject)it.next()).getObjectId());
		}
		return objects.toArray(new Long[objects.size()]);
	}
	
	/**
	 * Get selected object as object
	 * 
	 * @return
	 */
	public AbstractObject getFirstSelectedObject2()
	{
		IStructuredSelection selection = (IStructuredSelection)objectTree.getSelection();
		return (AbstractObject)selection.getFirstElement();
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
		AbstractObject obj = filter.getLastMatch();
		if (obj != null)
		{
			objectTree.setSelection(new StructuredSelection(obj), true);
			AbstractObject parent = filter.getParent(obj);
			if (parent != null)
				objectTree.expandToLevel(parent, 1);
			objectTree.reveal(obj);
			if (statusIndicatorEnabled)
				updateStatusIndicator();
		}
		objectTree.refresh(false);
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
	
	/**
	 * @param rootObjects
	 */
	public void setRootObjects(long[] rootObjects)
	{
		((ObjectTreeContentProvider)objectTree.getContentProvider()).setRootObjects(rootObjects);
		filter.setRootObjects(rootObjects);
		refresh();
	}
	
	/**
	 * @param listener
	 */
	public void addOpenListener(ObjectOpenListener listener)
	{
		openListeners.add(listener);
	}
	
	/**
	 * @param listener
	 */
	public void removeOpenListener(ObjectOpenListener listener)
	{
		openListeners.remove(listener);
	}
	
	/**
	 * Open selected object
	 * 
	 * @param object
	 * @return
	 */
	private boolean openObject(AbstractObject object)
	{
		for(ObjectOpenListener l : openListeners)
			if (l.openObject(object))
				return true;
		return false;
	}
	
	/**
    * Enable drop support in object tree
    */
   public void enableDropSupport(final ObjectBrowser obj)//SubtreeType infrastructure
   {      
      final Transfer[] transfers = new Transfer[] { LocalSelectionTransfer.getTransfer() };
      objectTree.addDropSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new ViewerDropAdapter(objectTree) {
         
         @Override
         public boolean performDrop(Object data) 
         {
            TreeSelection selection = (TreeSelection)data;
            List<?> movableSelection = selection.toList();
            for (int i = 0; i < movableSelection.size(); i++)
            {
               AbstractObject movableObject = (AbstractObject)movableSelection.get(i);
               TreePath path = selection.getPaths()[0];
               AbstractObject parent = (AbstractObject)path.getSegment(path.getSegmentCount() - 2);
               obj.performObjectMove((AbstractObject)getCurrentTarget(), parent, movableObject);
            }
            return true;
         }

			@Override
         public boolean validateDrop(Object target, int operation, TransferData transferType)
         {
            if (!LocalSelectionTransfer.getTransfer().isSupportedType(transferType))
               return false;

            IStructuredSelection selection = (IStructuredSelection)LocalSelectionTransfer.getTransfer().getSelection();
            TreePath path = ((TreeSelection)selection).getPaths()[0];
            AbstractObject parent = (AbstractObject)path.getSegment(path.getSegmentCount() - 2);
            
            Iterator<?> it = selection.iterator();
            if(!it.hasNext())
               return false;
            
            Object object;
            while (it.hasNext())
            {
               object = it.next();
               SubtreeType subtree = null;
               if ((object instanceof AbstractObject)) {
                  if(obj.isValidSelectionForMove(SubtreeType.INFRASTRUCTURE))
                    subtree = SubtreeType.INFRASTRUCTURE;
                  if(obj.isValidSelectionForMove(SubtreeType.TEMPLATES))
                     subtree = SubtreeType.TEMPLATES;
                  if(obj.isValidSelectionForMove(SubtreeType.BUSINESS_SERVICES))
                     subtree = SubtreeType.BUSINESS_SERVICES;
                  if(obj.isValidSelectionForMove(SubtreeType.DASHBOARDS))
                     subtree = SubtreeType.DASHBOARDS;
                  if(obj.isValidSelectionForMove(SubtreeType.MAPS))
                     subtree = SubtreeType.MAPS;
                  if(obj.isValidSelectionForMove(SubtreeType.POLICIES))
                     subtree = SubtreeType.POLICIES;
                  
               }
               Set<Integer> filter;
               
               if (subtree==null){
               	return false;
               }
               
               switch(subtree)
               {
                  case INFRASTRUCTURE:
                     filter = ObjectSelectionDialog.createContainerSelectionFilter();
                     break;
                  case TEMPLATES:
                     filter = ObjectSelectionDialog.createTemplateGroupSelectionFilter();
                     break;
                  case BUSINESS_SERVICES:
                     filter = ObjectSelectionDialog.createBusinessServiceSelectionFilter();
                     break;
                  case DASHBOARDS:
                     filter = ObjectSelectionDialog.createDashboardSelectionFilter();
                     break;
                  case MAPS:
                     filter = ObjectSelectionDialog.createNetworkMapGroupsSelectionFilter();
                     break;
                  case POLICIES:
                     filter = ObjectSelectionDialog.createPolicySelectionFilter();
                     break;
                  default:
                     filter = null;
                     break;
               }

               if(((AbstractObject)object).getParents().next() != (parent.getObjectId()))
                  return false;
               
               if(!filter.contains(((AbstractObject)target).getObjectClass()) || target.equals(object)){
                  return false;   
               }       
            }
            return true;
         }

      });
   }
}
