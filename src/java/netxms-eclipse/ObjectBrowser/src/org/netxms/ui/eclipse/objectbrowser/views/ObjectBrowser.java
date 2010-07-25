/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectTree;
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.shared.NXMCSharedData;


/**
 * @author Victor
 *
 */
public class ObjectBrowser extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.view.navigation.objectbrowser"; //$NON-NLS-1$
	
	private ObjectTree objectTree;
	
	/**
	 * Create popup menu for object browser
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(objectTree.getTreeControl());
		objectTree.getTreeControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, objectTree.getTreeViewer());
	}
	

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(new GroupMarker(IActionConstants.MB_OBJECT_CREATION));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_OBJECT_MANAGEMENT));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_OBJECT_BINDING));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_DATA_COLLECTION));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_PROPERTIES));
		mgr.add(new PropertyDialogAction(getSite(), objectTree.getTreeViewer()));
	}
	
	/**
	 * Create view menu
	 */
   private void createMenu()
   {
      IMenuManager mgr = getViewSite().getActionBars().getMenuManager();
      
      // Show filter
      Action actionShowFilter = new Action(Messages.getString("ObjectBrowser.show_filter"), SWT.TOGGLE) //$NON-NLS-1$
      {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				objectTree.enableFilter(isChecked());
			}
      };
      actionShowFilter.setChecked(true);
      mgr.add(actionShowFilter);
      
      // Hide unmanaged objects
      Action actionHideUnmanaged = new Action(Messages.getString("ObjectBrowser.hide_unmanaged"), SWT.TOGGLE) //$NON-NLS-1$
      {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
			}
      };
      actionHideUnmanaged.setChecked(false);
      mgr.add(actionHideUnmanaged);
      
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
   }

	/**
	 * 
	 */
	@Override
	public void createPartControl(Composite parent)
	{
      FormLayout formLayout = new FormLayout();
		parent.setLayout(formLayout);
		
		// Read custom root objects
		long[] rootObjects = null;
		Object value = NXMCSharedData.getInstance().getProperty("ObjectBrowser.rootObjects"); //$NON-NLS-1$
		if ((value != null) && (value instanceof long[]))
		{
			rootObjects = (long[])value;
		}
		
		objectTree = new ObjectTree(parent, SWT.NONE, ObjectTree.NONE, rootObjects, null);
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectTree.setLayoutData(fd);
		
		createMenu();
		createPopupMenu();
		
		getSite().setSelectionProvider(objectTree.getTreeViewer());
	}

	public void setFocus() 
	{
		objectTree.setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		super.dispose();
	}
}
