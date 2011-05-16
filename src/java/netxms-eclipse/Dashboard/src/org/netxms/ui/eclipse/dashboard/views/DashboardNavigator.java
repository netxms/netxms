/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.views;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.dashboard.views.helpers.DashboardEditorInput;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectTree;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.IActionConstants;

/**
 * @author Victor
 *
 */
public class DashboardNavigator extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.dashboard.views.DashboardNavigator";
	
	private NXCSession session;
	private ObjectTree objectTree;
	
	private Action actionOpenDashboard;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
		final Set<Integer> classFilter = new HashSet<Integer>(2);
		classFilter.add(GenericObject.OBJECT_DASHBOARD);
		objectTree = new ObjectTree(parent, SWT.NONE, ObjectTree.NONE, getRootObjects(classFilter), classFilter);
		objectTree.enableFilter(false);
		
		createActions();
		createPopupMenu();
	}
	
	/**
	 * @param classFilter
	 * @return
	 */
	private long[] getRootObjects(Set<Integer> classFilter)
	{
		GenericObject[] objects = session.getTopLevelObjects(classFilter);
		long[] ids = new long[objects.length];
		for(int i = 0; i < objects.length; i++)
			ids[i] = objects[i].getObjectId();
		return ids;
	}
	
	private void createActions()
	{
		actionOpenDashboard = new Action("&Open dashboard") {
			@Override
			public void run()
			{
				openDashboard();
			}
		};
	}

	/**
	 * Create popup menu for object browser
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager manager = new MenuManager();
		manager.setRemoveAllWhenShown(true);
		manager.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = manager.createContextMenu(objectTree.getTreeControl());
		objectTree.getTreeControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(manager, objectTree.getTreeViewer());
	}

	/**
	 * Fill context menu
	 * @param manager Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionOpenDashboard);
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_PROPERTIES));
		manager.add(new PropertyDialogAction(getSite(), objectTree.getTreeViewer()));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		objectTree.setFocus();
	}

	/**
	 * Open selected dashboard
	 */
	private void openDashboard()
	{
		GenericObject object = objectTree.getFirstSelectedObject2();
		if (object instanceof Dashboard)
		{
			try
			{
				getSite().getPage().openEditor(new DashboardEditorInput((Dashboard)object), DashboardEditorPart.ID, true);
			}
			catch(PartInitException e)
			{
				MessageDialog.openError(getSite().getShell(), "Error", "Cannot open dashboard: " + e.getMessage());
			}
		}
	}
}
