package org.netxms.ui.eclipse.quickmaps.views;

import org.eclipse.draw2d.IFigure;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.zest.core.viewers.GraphViewer;
import org.eclipse.zest.core.viewers.IFigureProvider;
import org.eclipse.zest.core.viewers.IGraphEntityContentProvider;
import org.eclipse.zest.layouts.LayoutStyles;
import org.eclipse.zest.layouts.algorithms.SpringLayoutAlgorithm;
import org.netxms.client.NXCMapPage;
import org.netxms.client.NXCNode;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.quickmaps.Activator;
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

public abstract class NetworkMap extends ViewPart
{
	/**
	 * Content provider for map
	 * 
	 * @author Victor
	 */
	class MapContentProvider implements IGraphEntityContentProvider
	{
		private NXCMapPage page;
		
		@Override
		public Object[] getElements(Object inputElement)
		{
			if (!(inputElement instanceof NXCMapPage))
				return null;
			
			return ((NXCMapPage)inputElement).getResolvedObjects(session);
		}

		@Override
		public Object[] getConnectedTo(Object entity)
		{
			if (!(entity instanceof NXCObject) || (page == null))
				return null;
			
			return page.getConnectedObjects(((NXCObject)entity).getObjectId(), session);
		}

		@Override
		public void dispose()
		{
		}

		@Override
		public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
		{
			if (newInput instanceof NXCMapPage)
				page = (NXCMapPage)newInput;
			else
				page = null;
		}
	}
	
	/**
	 * Label provider for map
	 */
	class MapLabelProvider extends LabelProvider implements IFigureProvider
	{
		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
		 */
		@Override
		public String getText(Object element)
		{
			if (element instanceof NXCObject)
				return ((NXCObject)element).getObjectName();
			return null;
		}

		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
		 */
		@Override
		public Image getImage(Object element)
		{
			if (element instanceof NXCObject)
				return ((NXCObject)element).getObjectClass() == NXCObject.OBJECT_NODE ? imgNode : imgSubnet;
			return null;
		}

		/* (non-Javadoc)
		 * @see org.eclipse.zest.core.viewers.IFigureProvider#getFigure(java.lang.Object)
		 */
		@Override
		public IFigure getFigure(Object element)
		{
			return null;
			//return new ObjectFigure((NXCObject)element);
		}
	}
	
	protected NXCSession session;
	protected NXCNode node;
	protected NXCMapPage mapPage;
	protected GraphViewer viewer;
	protected Image imgNode;
	protected Image imgSubnet;

	public NetworkMap()
	{
		super();
		imgNode = Activator.getImageDescriptor("icons/node.png").createImage();
		imgSubnet = Activator.getImageDescriptor("icons/subnet.png").createImage();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

		session = NXMCSharedData.getInstance().getSession();
		NXCObject obj = session.findObjectById(Long.parseLong(site.getSecondaryId()));
		node = ((obj != null) && (obj instanceof NXCNode)) ? (NXCNode)obj : null;
	
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

	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	@Override
	public void dispose()
	{
		super.dispose();
		
		imgNode.dispose();
		imgSubnet.dispose();
	}
}