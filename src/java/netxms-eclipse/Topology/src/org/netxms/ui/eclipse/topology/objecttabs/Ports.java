/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.topology.objecttabs;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.objecttabs.NodeComponentTab;
import org.netxms.ui.eclipse.topology.widgets.DeviceView;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortInfo;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortSelectionListener;

/**
 * "Ports" object tab
 */
public class Ports extends NodeComponentTab implements ISelectionProvider
{
	private ScrolledComposite scroller;
	private DeviceView deviceView;
	private ISelection selection = new StructuredSelection();
	private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected void createTabContent(Composite parent)
	{
      super.createTabContent(parent);

      scroller = new ScrolledComposite(mainArea, SWT.H_SCROLL | SWT.V_SCROLL);
		deviceView = new DeviceView(scroller, SWT.NONE);
		deviceView.addSelectionListener(new PortSelectionListener() {
			@Override
			public void portSelected(PortInfo port)
			{
				if (port != null)
				{
					Interface iface = (Interface)session.findObjectById(port.getInterfaceObjectId(), Interface.class);
					if (iface != null)
					{
						selection = new StructuredSelection(iface);
					}
					else
					{
						selection = new StructuredSelection();
					}
				}
				else
				{
					selection = new StructuredSelection();
				}
				
				for(ISelectionChangedListener listener : selectionListeners)
					listener.selectionChanged(new SelectionChangedEvent(Ports.this, selection));
			}
		});
      mainArea.setBackground(deviceView.getBackground());

		scroller.setContent(deviceView);
		scroller.setExpandVertical(true);
		scroller.setExpandHorizontal(true);
		scroller.getVerticalBar().setIncrement(20);
		scroller.getHorizontalBar().setIncrement(20);
		scroller.addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				scroller.setMinSize(deviceView.computeSize(SWT.DEFAULT, SWT.DEFAULT));
			}
		});
		
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.bottom = new FormAttachment(100, 0);
      fd.right = new FormAttachment(100, 0);
      scroller.setLayoutData(fd);
		
		createPopupMenu();
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
   @Override
   public void refresh()
   {
      deviceView.setNodeId((getObject() != null) ? getObject().getObjectId() : 0);
      scroller.setMinSize(deviceView.computeSize(SWT.DEFAULT, SWT.DEFAULT));
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.NodeComponentTab#needRefreshOnObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean needRefreshOnObjectChange(AbstractObject object)
   {
      return (object instanceof Interface) && object.isDirectChildOf(deviceView.getNodeId());
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
    */
	@Override
	public boolean showForObject(AbstractObject object)
	{
		return (object instanceof Node) ? ((Node)object).isBridge() : false;
	}

	/**
	 * Create pop-up menu
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
		Menu menu = menuMgr.createContextMenu(deviceView);
		deviceView.setMenu(menu);

		// Register menu for extension.
		getViewPart().getSite().registerContextMenu(menuMgr, this);
	}
	
	/**
	 * Fill port context menu
	 * 
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
	   ObjectContextMenu.fill(manager, getViewPart().getSite(), this);
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
	@Override
	public void addSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.add(listener);
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
    */
	@Override
	public ISelection getSelection()
	{
		return selection;
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
	@Override
	public void removeSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.remove(listener);
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
	@Override
	public void setSelection(ISelection selection)
	{
		this.selection = selection;
	}
}
