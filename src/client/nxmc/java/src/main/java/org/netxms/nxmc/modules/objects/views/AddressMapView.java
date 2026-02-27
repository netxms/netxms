/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Subnet;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.widgets.SubnetAddressMap;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Address Map" object tab
 */
public class AddressMapView extends ObjectView implements ISelectionProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(AddressMapView.class);

	private ScrolledComposite scroller;
	private SubnetAddressMap addressMap;
   private Action actionGoToObject;
   private ISelection selection = new StructuredSelection();
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();

   /**
    * Create "Services" view
    */
   public AddressMapView()
   {
      super(LocalizationHelper.getI18n(AddressMapView.class).tr("Address map"), ResourceManager.getImageDescriptor("icons/object-views/address_map.png"), "objects.address-map", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 22;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
		scroller = new ScrolledComposite(parent, SWT.H_SCROLL | SWT.V_SCROLL);
		addressMap = new SubnetAddressMap(scroller, SWT.NONE, this);

		scroller.setContent(addressMap);
		scroller.setExpandVertical(true);
		scroller.setExpandHorizontal(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.HORIZONTAL, 20);
		scroller.addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				scroller.setMinSize(addressMap.computeSize(SWT.DEFAULT, SWT.DEFAULT));
			}
		});
      addressMap.setUpdateListener(() -> scroller.setMinSize(addressMap.computeSize(SWT.DEFAULT, SWT.DEFAULT)));

      addressMap.addSelectionListener((object) -> {
         selection = (object != null) ? new StructuredSelection(object) : new StructuredSelection();
         for(ISelectionChangedListener l : selectionListeners)
            l.selectionChanged(new SelectionChangedEvent(AddressMapView.this, selection));
      });

      createPopupMenu();
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if ((object != null) && isActive())
      {
         refresh();
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      refresh();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
   @Override
   public void refresh()
   {
      addressMap.setSubnet((Subnet)getObject());
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      actionGoToObject = new Action(i18n.tr("Go to &object")) {
         @Override
         public void run()
         {
            IStructuredSelection sel = (IStructuredSelection)getSelection();
            if (sel.size() == 1 && sel.getFirstElement() instanceof AbstractObject)
            {
               MainWindow.switchToObject(((AbstractObject)sel.getFirstElement()).getObjectId(), 0);
            }
         }
      };

      MenuManager menuMgr = new ObjectContextMenuManager(this, this, null) {
         @Override
         protected void fillContextMenu()
         {
            IStructuredSelection sel = (IStructuredSelection)getSelection();
            if (!sel.isEmpty())
            {
               add(actionGoToObject);
               add(new Separator());
            }
            super.fillContextMenu();
         }
      };
      addressMap.setMenu(menuMgr.createContextMenu(addressMap));
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
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void removeSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionListeners.remove(listener);
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
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void setSelection(ISelection selection)
   {
      this.selection = selection;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return context instanceof Subnet;
   }
}
