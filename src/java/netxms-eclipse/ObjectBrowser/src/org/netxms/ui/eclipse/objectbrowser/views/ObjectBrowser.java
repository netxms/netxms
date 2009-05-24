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
import org.netxms.ui.eclipse.objectbrowser.ObjectTree;
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.shared.NXMCSharedData;


/**
 * @author Victor
 *
 */
public class ObjectBrowser extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objectbrowser.view.object_browser";
	
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
		mgr.add(new GroupMarker(IActionConstants.MB_OBJECT_MANAGEMENT));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_DATA_COLLECTION));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_PROPERTIES));
		mgr.add(new PropertyDialogAction(getSite(), objectTree.getTreeViewer()));
		
		/*
		mgr.add(new GroupMarker(IActionConstants.MB_OBJECT_MANAGEMENT));
		mgr.add(new Action("Unbind") { });
		mgr.add(new Action("Rename") { });
		mgr.add(new Action("Delete") { });
		mgr.add(new Action("Move to another container...") { });
		mgr.add(new Action("Change IP address...") { });
		mgr.add(new Action("Manage") { });
		mgr.add(new Action("Unmanage") { });
		mgr.add(new Separator());
		mgr.add(new Action("Edit agent configuration file") { });
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		
		MenuManager tools = new MenuManager("Tools");
		
		MenuManager atm = new MenuManager("Terminal");
		atm.add(new Action("Restart application") { });
		atm.add(new Separator());
		atm.add(new Action("Eject card") { });
		atm.add(new Action("Retain card") { });
		atm.add(new Separator());
		atm.add(new Action("Detailed device stats") { });
		atm.add(new Action("Execute command...") { });
		tools.add(atm);
		
		MenuManager diag = new MenuManager("Diagnostics");
		diag.add(new Action("Shutdown system") { });
		tools.add(diag);
		
		tools.add(new Separator());
		tools.add(new Action("Shutdown system") { });
		tools.add(new Action("Restart system") { });
		tools.add(new Action("Restart agent") { });
		mgr.add(tools);
		
		mgr.add(new Separator());
		mgr.add(new Action("Configure data collection") { });
		mgr.add(new GroupMarker(IActionConstants.MB_DATA_COLLECTION));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_PROPERTIES));
		mgr.add(new Action("Comments") { });
		mgr.add(new PropertyDialogAction(getSite(), objectTree.getTreeViewer()));
		*/

		/*
		MenuManager diag = new MenuManager("Create");
		diag.add(new Action("Shutdown system") { });
		mgr.add(diag);

		mgr.add(new GroupMarker(IActionConstants.MB_OBJECT_MANAGEMENT));
		mgr.add(new Action("Unbind") { });
		mgr.add(new Action("Rename") { });
		mgr.add(new Action("Delete") { });
		mgr.add(new Action("Move to another container...") { });
		mgr.add(new Action("Manage") { });
		mgr.add(new Action("Unmanage") { });
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Action("Execute on all...") { });
		mgr.add(new Action("Upload file to all...") { });
		mgr.add(new Action("Force status poll") { });
		mgr.add(new Action("Force configuration poll") { });
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_PROPERTIES));
		mgr.add(new Action("Comments") { });
		mgr.add(new PropertyDialogAction(getSite(), objectTree.getTreeViewer()));
		*/
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
		
		// Read custom root objects
		long[] rootObjects = null;
		Object value = NXMCSharedData.getInstance().getProperty("ObjectBrowser.rootObjects");
		if ((value != null) && (value instanceof long[]))
		{
			rootObjects = (long[])value;
		}
		
		objectTree = new ObjectTree(parent, SWT.NONE, ObjectTree.NONE, rootObjects);
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
