package org.netxms.ui.eclipse.networkmaps.views;

import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.zest.core.viewers.GraphViewer;
import org.eclipse.zest.layouts.LayoutStyles;
import org.eclipse.zest.layouts.algorithms.SpringLayoutAlgorithm;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapContentProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapLabelProvider;
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

public abstract class NetworkMap extends ViewPart
{
	protected NXCSession session;
	protected Node node;
	protected NetworkMapPage mapPage;
	protected GraphViewer viewer;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

		session = NXMCSharedData.getInstance().getSession();
		GenericObject obj = session.findObjectById(Long.parseLong(site.getSecondaryId()));
		node = ((obj != null) && (obj instanceof Node)) ? (Node)obj : null;
	
		buildMapPage();
	}
	
	/**
	 * Build map page containing data to display. Should be implemented
	 * in derived classes.
	 */
	protected abstract void buildMapPage();

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		FillLayout layout = new FillLayout();
		parent.setLayout(layout);
		
		viewer = new GraphViewer(parent, SWT.NONE);
		viewer.setContentProvider(new MapContentProvider());
		viewer.setLabelProvider(new MapLabelProvider());
		viewer.setLayoutAlgorithm(new SpringLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING));
		viewer.setInput(mapPage);
	
		createPopupMenu();
	}

	/**
	 * Create popup menu for map
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
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	
		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
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
		mgr.add(new PropertyDialogAction(getSite(), viewer));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}
}
