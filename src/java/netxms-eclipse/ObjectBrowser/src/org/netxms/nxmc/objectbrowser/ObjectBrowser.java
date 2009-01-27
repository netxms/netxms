/**
 * 
 */
package org.netxms.nxmc.objectbrowser;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;


/**
 * @author Victor
 *
 */
public class ObjectBrowser extends ViewPart
{
	public static final String ID = "org.netxms.nxmc.objectbrowser.view.object_browser";
	
	private ObjectTree objectTree;

	
	/**
	 * 
	 */
	public ObjectBrowser()
	{
		super();
	}

	
	/**
	 * Create popup menu for object browser
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
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
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}

	
	/**
	 * Create view menu
	 */
   private void createMenu()
   {
      IMenuManager mgr = getViewSite().getActionBars().getMenuManager();
      
      // Show filter
      Action actionShowFilter = new Action("Show filter", SWT.TOGGLE)
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
      Action actionHideUnmanaged = new Action("Hide unmanaged objects", SWT.TOGGLE)
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
		
		objectTree = new ObjectTree(parent, SWT.NONE);
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectTree.setLayoutData(fd);
		
		createMenu();
		createPopupMenu();	
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
