/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import java.util.HashSet;
import java.util.Set;
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
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.widgets.DeviceViewWidget;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortInfo;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortSelectionListener;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Ports" view
 */
public class PortView extends NodeSubObjectView implements ISelectionProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(PortView.class);

	private ScrolledComposite scroller;
   private DeviceViewWidget deviceView;
	private ISelection selection = new StructuredSelection();
	private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();

   /**
    * Create "Ports" view
    */
   public PortView()
   {
      super(LocalizationHelper.getI18n(PortView.class).tr("Ports"), ResourceManager.getImageDescriptor("icons/object-views/ports.png"), "objects.ports", false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && ((Node)context).isBridge();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 90;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      deviceView.setNodeId((object != null) ? object.getObjectId() : 0);
      scroller.setMinSize(deviceView.computeSize(SWT.DEFAULT, SWT.DEFAULT));
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   protected void createContent(Composite parent)
	{
      super.createContent(parent);

      scroller = new ScrolledComposite(mainArea, SWT.H_SCROLL | SWT.V_SCROLL);
      deviceView = new DeviceViewWidget(scroller, SWT.NONE);
		deviceView.addSelectionListener(new PortSelectionListener() {
			@Override
			public void portSelected(PortInfo port)
			{
				if (port != null)
				{
               Interface iface = session.findObjectById(port.getInterfaceObjectId(), Interface.class);
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
					listener.selectionChanged(new SelectionChangedEvent(PortView.this, selection));
			}
		});
      scroller.setBackground(deviceView.getBackground());

		scroller.setContent(deviceView);
      scroller.setExpandVertical(true);
      scroller.setExpandHorizontal(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.HORIZONTAL, 20);
		scroller.addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				scroller.setMinSize(deviceView.computeSize(SWT.DEFAULT, SWT.DEFAULT));
			}
		});

		createContextMenu();
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
   @Override
   public void refresh()
   {
      deviceView.setNodeId(getObjectId());
      scroller.setMinSize(deviceView.computeSize(SWT.DEFAULT, SWT.DEFAULT));
   }

	/**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#needRefreshOnObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean needRefreshOnObjectChange(AbstractObject object)
   {
      return (object instanceof Interface) && object.isDirectChildOf(getObjectId());
   }

   /**
    * Create pop-up menu
    */
	private void createContextMenu()
	{
		// Create menu manager.
      MenuManager menuMgr = new ObjectContextMenuManager(this, this, null);
      deviceView.setMenu(menuMgr.createContextMenu(deviceView));
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
