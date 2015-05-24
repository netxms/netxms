/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.widgets;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectview.api.ObjectDetailsProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Widget showing "heat" map of nodes under given root object
 */
public class ObjectStatusMap extends ScrolledComposite implements ISelectionProvider
{
	private IViewPart viewPart;
	private long rootObjectId;
	private NXCSession session;
	private Composite dataArea;
	private List<Composite> sections = new ArrayList<Composite>();
	private Map<Long, ObjectStatusWidget> statusWidgets = new HashMap<Long, ObjectStatusWidget>();
	private ISelection selection = null;
	private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
	private MenuManager menuManager;
	private Font titleFont;
	private boolean groupObjects = true;
	private int severityFilter = 0xFF;
	private SortedMap<Integer, ObjectDetailsProvider> detailsProviders = new TreeMap<Integer, ObjectDetailsProvider>();
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectStatusMap(IViewPart viewPart, Composite parent, int style)
	{
		super(parent, style | SWT.V_SCROLL);
		
		initDetailsProviders();
		
		this.viewPart = viewPart;
		session = (NXCSession)ConsoleSharedData.getSession();
		final SessionListener sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == SessionNotification.OBJECT_CHANGED)
					onObjectChange((AbstractObject)n.getObject());
				else if (n.getCode() == SessionNotification.OBJECT_DELETED)
					onObjectDelete(n.getSubCode());
			}
		};
		session.addListener(sessionListener);
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				session.removeListener(sessionListener);
			}
		});
		
		setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, getDisplay()));
		setExpandHorizontal(true);
		setExpandVertical(true);
		addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				Rectangle r = getClientArea();
				setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
			}
		});
		
		dataArea = new Composite(this, SWT.NONE);
		setContent(dataArea);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = 10;
		dataArea.setLayout(layout);
		dataArea.setBackground(getBackground());
		
		titleFont = JFaceResources.getFontRegistry().getBold(JFaceResources.BANNER_FONT);
		
		menuManager = new MenuManager();
		menuManager.setRemoveAllWhenShown(true);
		menuManager.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager manager)
			{
				fillContextMenu(manager);
			}
		});
	}
	
	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_CREATION));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_ATM));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_NXVS));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_MANAGEMENT));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_BINDING));
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_TOPOLOGY));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_DATA_COLLECTION));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_PROPERTIES));
		if (viewPart != null)
			manager.add(new PropertyDialogAction(viewPart.getSite(), this));
	}

	/**
	 * @param objectId
	 */
	public void setRootObject(long objectId)
	{
		rootObjectId = objectId;
		refresh();
	}
	
	/**
	 * Refresh form
	 */
	public void refresh()
	{
		for(Composite s : sections)
			s.dispose();
		sections.clear();
		
		synchronized(statusWidgets)
		{
			statusWidgets.clear();
		}
		
		if (groupObjects)
			buildSection(rootObjectId, ""); //$NON-NLS-1$
		else
			buildFlatView();
		dataArea.layout(true, true);
		
		Rectangle r = getClientArea();
		setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
	}
	
	/**
	 * Build flat view - all nodes in one group
	 */
	private void buildFlatView()
	{
		AbstractObject root = session.findObjectById(rootObjectId);
		if ((root == null) || !((root instanceof Container) || (root instanceof ServiceRoot) || (root instanceof Cluster)))
			return;
		
		List<AbstractObject> objects = 
		      new ArrayList<AbstractObject>(root.getAllChilds(new int[] { AbstractObject.OBJECT_NODE, AbstractObject.OBJECT_CLUSTER }));
		
		// apply severity filter
		if ((severityFilter & 0x1F) != 0x1F)
		{
			Iterator<AbstractObject> it = objects.iterator();
			while(it.hasNext())
			{
				AbstractObject o = it.next();
				if (((1 << o.getStatus().getValue()) & severityFilter) == 0)
				{
					it.remove();
				}
			}
		}
		
		Collections.sort(objects, new Comparator<AbstractObject>() {
			@Override
			public int compare(AbstractObject o1, AbstractObject o2)
			{
				return o1.getObjectName().compareToIgnoreCase(o2.getObjectName());
			}
		});

		final Composite clientArea = new Composite(dataArea, SWT.NONE);
		clientArea.setBackground(getBackground());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		clientArea.setLayoutData(gd);
		RowLayout clayout = new RowLayout();
		clayout.marginBottom = 0;
		clayout.marginTop = 0;
		clayout.marginLeft = 0;
		clayout.marginRight = 0;
		clayout.type = SWT.HORIZONTAL;
		clayout.wrap = true;
		clayout.pack = false;
		clientArea.setLayout(clayout);
		sections.add(clientArea);
		
		for(AbstractObject o : objects)
		{
			if (!((o instanceof AbstractNode) || (o instanceof Cluster)))
				continue;

			addObjectElement(clientArea, o);
		}
	}
	
	/**
	 * Build section of the form corresponding to one container
	 */
	private void buildSection(long rootId, String namePrefix)
	{
		AbstractObject root = session.findObjectById(rootId);
		if ((root == null) || !((root instanceof Container) || (root instanceof ServiceRoot) || (root instanceof Cluster)))
			return;
		
		List<AbstractObject> objects = new ArrayList<AbstractObject>(Arrays.asList(root.getChildsAsArray()));
		Collections.sort(objects, new Comparator<AbstractObject>() {
			@Override
			public int compare(AbstractObject o1, AbstractObject o2)
			{
				return o1.getObjectName().compareToIgnoreCase(o2.getObjectName());
			}
		});
		
		Composite section = null;
		Composite clientArea = null;
		
		// Add nodes and clusters
		for(AbstractObject o : objects)
		{
			if (!((o instanceof AbstractNode) || (o instanceof Cluster)))
				continue;
			
			if (((1 << o.getStatus().getValue()) & severityFilter) == 0)
				continue;

			if (section == null)
			{
				section = new Composite(dataArea, SWT.NONE);
				section.setBackground(getBackground());
				GridData gd = new GridData();
				gd.grabExcessHorizontalSpace = true;
				gd.horizontalAlignment = SWT.FILL;
				section.setLayoutData(gd);
				
				GridLayout layout = new GridLayout();
				layout.marginHeight = 0;
				layout.marginWidth = 0;
				section.setLayout(layout);
				
				final Label title = new Label(section, SWT.NONE);
				title.setBackground(getBackground());
				title.setFont(titleFont);
				title.setText(namePrefix + root.getObjectName());

				clientArea = new Composite(section, SWT.NONE);
				clientArea.setBackground(getBackground());
				gd = new GridData();
				gd.grabExcessHorizontalSpace = true;
				gd.horizontalAlignment = SWT.FILL;
				clientArea.setLayoutData(gd);
				RowLayout clayout = new RowLayout();
				clayout.marginBottom = 0;
				clayout.marginTop = 0;
				clayout.marginLeft = 0;
				clayout.marginRight = 0;
				clayout.type = SWT.HORIZONTAL;
				clayout.wrap = true;
				clayout.pack = false;
				clientArea.setLayout(clayout);
				
				sections.add(section);
			}
			
			addObjectElement(clientArea, o);
		}
		
		// Add subcontainers
		for(AbstractObject o : objects)
		{
			if (!(o instanceof Container) && !(o instanceof ServiceRoot) && !(o instanceof Cluster))
				continue;
			
			buildSection(o.getObjectId(), namePrefix + root.getObjectName() + " / "); //$NON-NLS-1$
		}
	}
	
	/**
	 * @param object
	 */
	private void addObjectElement(final Composite parent, final AbstractObject object)
	{
		ObjectStatusWidget w = new ObjectStatusWidget(parent, object);
		w.setBackground(getBackground());
		w.addMouseListener(new MouseListener() {
			@Override
			public void mouseUp(MouseEvent e)
			{
			}
			
			@Override
			public void mouseDown(MouseEvent e)
			{
				setSelection(new StructuredSelection(object));
				if (e.button == 1)
					callDetailsProvider(object);
			}
			
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
			}
		});

		// Create popup menu
		Menu menu = menuManager.createContextMenu(w);
		w.setMenu(menu);

		// Register menu for extension.
		if (viewPart != null)
			viewPart.getSite().registerContextMenu(menuManager, this);
		
		synchronized(statusWidgets)
		{
			statusWidgets.put(object.getObjectId(), w);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
	 */
	@Override
	public void addSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.add(listener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
	 */
	@Override
	public ISelection getSelection()
	{
		return selection;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
	 */
	@Override
	public void removeSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.remove(listener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void setSelection(ISelection selection)
	{
		this.selection = selection;
		SelectionChangedEvent event = new SelectionChangedEvent(this, selection);
		for(ISelectionChangedListener l : selectionListeners)
			l.selectionChanged(event);
	}

	/**
	 * @return the groupObjects
	 */
	public boolean isGroupObjects()
	{
		return groupObjects;
	}

	/**
	 * @param groupObjects the groupObjects to set
	 */
	public void setGroupObjects(boolean groupObjects)
	{
		this.groupObjects = groupObjects;
	}
	
	/**
	 * Initialize object details providers
	 */
	private void initDetailsProviders()
	{
		// Read all registered extensions and create tabs
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.objectview.objectDetailsProvider"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final ObjectDetailsProvider provider = (ObjectDetailsProvider)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
				int priority;
				try
				{
					priority = Integer.parseInt(elements[i].getAttribute("priority")); //$NON-NLS-1$
				}
				catch(NumberFormatException e)
				{
					priority = 65535;
				}
				detailsProviders.put(priority, provider);
			}
			catch(CoreException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	/**
	 * Call object details provider
	 * 
	 * @param node
	 */
	private void callDetailsProvider(AbstractObject object)
	{
		for(ObjectDetailsProvider p : detailsProviders.values())
		{
			if (p.canProvideDetails(object))
			{
				p.provideDetails(object, viewPart);
				break;
			}
		}
	}
	
	/**
	 * Handle object change
	 */
	private void onObjectChange(final AbstractObject object)
	{
		if (!((object instanceof AbstractNode) || (object instanceof Container) || (object instanceof Cluster) || (object instanceof ServiceRoot)))
			return;
		
		synchronized(statusWidgets)
		{
			final ObjectStatusWidget w = statusWidgets.get(object.getObjectId());
			if (w != null)
			{
				getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						if (!w.isDisposed())
							w.updateObject(object);
					}
				});
			}
			else if ((object.getObjectId() == rootObjectId) || object.isChildOf(rootObjectId))
			{
				getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						if (!isDisposed())
							refresh();
					}
				});
			}
		}
	}
	
	/**
	 * Handle object delete
	 */
	private void onObjectDelete(long objectId)
	{
		synchronized(statusWidgets)
		{
			if (statusWidgets.containsKey(objectId))
			{
				getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						if (!isDisposed())
							refresh();
					}
				});
			}
		}
	}

	/**
	 * @return
	 */
	public int getSeverityFilter()
	{
		return severityFilter;
	}

	/**
	 * @param severityFilter
	 */
	public void setSeverityFilter(int severityFilter)
	{
		this.severityFilter = severityFilter;
	}
}
