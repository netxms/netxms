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

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Subnet;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.SubnetAddressMap;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * "Address Map" object tab
 */
public class AddressMapView extends ObjectView
{   
	private ScrolledComposite scroller;
	private SubnetAddressMap addressMap;

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
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return context instanceof Subnet;
   }
}
