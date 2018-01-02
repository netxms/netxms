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

import java.util.HashSet;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.NetworkService;
import org.netxms.client.objects.VPNConnector;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.api.ObjectDetailsProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.FilterText;

/**
 * Widget showing "heat" map of nodes under given root object
 */
public class ObjectStatusMapRadial extends Composite implements ISelectionProvider, ObjectStatusMapInterface
{
	private IViewPart viewPart;
	private long rootObjectId;
	private NXCSession session;
   private FilterText filterTextControl;
   private ScrolledComposite scroller;
	private Composite dataArea;
	private ISelection selection = null;
	private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
	private MenuManager menuManager;
	private Font titleFont;
	private boolean filterEnabled = true;
	private int severityFilter = 0xFF;
	private String textFilter = "";
	ObjectStatusRadialWidget widget;
	private SortedMap<Integer, ObjectDetailsProvider> detailsProviders = new TreeMap<Integer, ObjectDetailsProvider>();
	private Set<Runnable> refreshListeners = new HashSet<Runnable>();
	private AbstractObject hoveredObject = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectStatusMapRadial(IViewPart viewPart, Composite parent, int style, boolean allowFilterClose)
	{
		super(parent, style);
		
		initDetailsProviders();
		
		this.viewPart = viewPart;
		session = (NXCSession)ConsoleSharedData.getSession();
		final SessionListener sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == SessionNotification.OBJECT_CHANGED || n.getCode() == SessionNotification.OBJECT_DELETED)
				   getDisplay().asyncExec(new Runnable() {
                  
                  @Override
                  public void run()
                  {
                     refresh();
                  }
               });
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
		
      FormLayout formLayout = new FormLayout();
      setLayout(formLayout);
      
      setBackground(getDisplay().getSystemColor(SWT.COLOR_WHITE));

      // Create filter area
      filterTextControl = new FilterText(this, SWT.NONE, null, allowFilterClose);
      filterTextControl.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterTextControl.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
         }
      });
      
      scroller = new ScrolledComposite(this, SWT.V_SCROLL);
		scroller.setBackground(getBackground());
		scroller.setExpandHorizontal(true);
		scroller.setExpandVertical(true);
		addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				Rectangle r = getClientArea();
				scroller.setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
			}
		});
		scroller.getVerticalBar().setIncrement(30);
		
		dataArea = new Composite(scroller, SWT.NONE);
		scroller.setContent(dataArea);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = 10;
		dataArea.setLayout(layout);
		dataArea.setBackground(getBackground());
		
		if (Platform.getOS().equals(Platform.OS_WIN32))
		{
			titleFont = new Font(parent.getDisplay(), "Verdana", 10, SWT.BOLD); //$NON-NLS-1$
			addDisposeListener(new DisposeListener() {
				@Override
				public void widgetDisposed(DisposeEvent e)
				{
					titleFont.dispose();
				}
			});
		}
		else
		{
			titleFont = JFaceResources.getFontRegistry().getBold(JFaceResources.BANNER_FONT);
		}
		
		menuManager = new MenuManager();
		menuManager.setRemoveAllWhenShown(true);
		menuManager.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager manager)
			{
				fillContextMenu(manager);
			}
		});

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterTextControl);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      scroller.setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterTextControl.setLayoutData(fd);

      // Set initial focus to filter input line
      if (filterEnabled)
         filterTextControl.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
	}
	
	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		ObjectContextMenu.fill(manager, (viewPart != null) ? viewPart.getSite() : null, this); 
	}

	/**
	 * @param objectId
	 */
	public void setRootObject(long objectId)
	{
		rootObjectId = objectId;
		refresh();
	}
	
   public Set<Long> createFilteredList(AbstractObject root, boolean flterByTest)
   {
      Set<Long> aceptedlist = new HashSet<Long>();
      for(AbstractObject obj : root.getAllChilds(-1))
      {
         if(obj instanceof Interface || obj instanceof NetworkService || obj instanceof VPNConnector)
            continue;
         if(flterByTest)
         {
            if(obj.getObjectName().toLowerCase().contains(textFilter))
               aceptedlist.add(obj.getObjectId());
         }
         else
            if(((1 << obj.getStatus().getValue()) & severityFilter) != 0)
               aceptedlist.add(obj.getObjectId());
      }
      return aceptedlist;
   }
	
	/**
	 * Refresh form
	 */
	public void refresh()
	{
      AbstractObject root = session.findObjectById(rootObjectId);
      Set<Long> aceptedlist = null;
      
      if(!textFilter.isEmpty())
         aceptedlist = createFilteredList(root, true);
      
      if(severityFilter != 0xFF)
         aceptedlist = createFilteredList(root, false);
      
      if (widget == null)
      {
         widget = new ObjectStatusRadialWidget(dataArea, root, aceptedlist);
         widget.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

         widget.addMouseListener(new MouseListener() {
            @Override
            public void mouseUp(MouseEvent e)
            {
            }
            
            @Override
            public void mouseDown(MouseEvent e)
            {
               AbstractObject curr = widget.getObjectFromPoint(e.x, e.y);
               if (curr != null)
               {
                  setSelection(new StructuredSelection(curr));
                  if (e.button == 1)
                     callDetailsProvider(curr);
               }
               else
               {
                  setSelection(new StructuredSelection());
               }
               hoveredObject = null;
               widget.removeTooltip();
            }
            
            @Override
            public void mouseDoubleClick(MouseEvent e)
            {
            }
         });
         
         widget.addMouseMoveListener(new MouseMoveListener() {
            
            @Override
            public void mouseMove(MouseEvent e)
            {
               hoveredObject = null;
               widget.removeTooltip();
            }
         });
         
         widget.addMouseTrackListener( new MouseTrackListener() {
            
            @Override
            public void mouseHover(MouseEvent e)
            {
               AbstractObject curr = widget.getObjectFromPoint(e.x, e.y);
               if (curr == hoveredObject || curr == null) // ignore hover if tooltip already open
                  return;
               
               hoveredObject = curr;
               widget.setHoveredObject(hoveredObject, new Point(e.x, e.y));
            }
            
            @Override
            public void mouseExit(MouseEvent e)
            {
               hoveredObject = null;
               widget.removeTooltip();
            }
            
            @Override
            public void mouseEnter(MouseEvent e)
            {
            }
         });

         // Create popup menu
         Menu menu = menuManager.createContextMenu(widget);
         widget.setMenu(menu);

         // Register menu for extension.
         if (viewPart != null)
            viewPart.getSite().registerContextMenu(menuManager, this);
      }
      else
      {
         widget.updateObject(root, aceptedlist);
         widget.redraw();
      }
		
		dataArea.layout(true, true);
		
		Rectangle r = getClientArea();
		scroller.setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
		
		for(Runnable l : refreshListeners)
		   l.run();
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
	
	/**
	 * @param listener
	 */
	public void addRefreshListener(Runnable listener)
	{
	   refreshListeners.add(listener);
	}
	
	/**
	 * @param listener
	 */
	public void removeRefreshListener(Runnable listener)
	{
	   refreshListeners.remove(listener);
	}

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      filterEnabled = enable;
      filterTextControl.setVisible(filterEnabled);
      FormData fd = (FormData)scroller.getLayoutData();
      fd.top = enable ? new FormAttachment(filterTextControl) : new FormAttachment(0, 0);
      layout();
      if (enable)
         filterTextControl.setFocus();
      else
         setFilterText(""); //$NON-NLS-1$
   }

   /**
    * @return the filterEnabled
    */
   public boolean isFilterEnabled()
   {
      return filterEnabled;
   }
   
   /**
    * Set action to be executed when user press "Close" button in object filter.
    * Default implementation will hide filter area without notifying parent.
    * 
    * @param action
    */
   public void setFilterCloseAction(Action action)
   {
      filterTextControl.setCloseAction(action);
   }
   
   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   public void setFilterText(final String text)
   {
      filterTextControl.setText(text);
      onFilterModify();
   }

   /**
    * Get filter text
    * 
    * @return Current filter text
    */
   public String getFilterText()
   {
      return filterTextControl.getText();
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      String text = filterTextControl.getText().trim().toLowerCase();
      if (!textFilter.equals(text))
      {
         textFilter = text;
         refresh();
      }
   }
}
