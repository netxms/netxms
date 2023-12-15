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
package org.netxms.nxmc.modules.objects.propertypages;

import org.eclipse.jface.preference.PreferencePage;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;

/**
 * Abstract base class for object property page
 */
public abstract class ObjectPropertyPage extends PreferencePage
{
   protected AbstractObject object;
   protected MessageAreaHolder messageArea;

   /**
    * Create new page.
    *
    * @param title page title
    * @param object object to edit
    */
   public ObjectPropertyPage(String title, AbstractObject object)
   {
      super(title);
      this.object = object;
   }

   /**
    * Get page ID.
    *
    * @return page ID
    */
   public abstract String getId();

   /**
    * Get parent page ID.
    *
    * @return parent page ID or null
    */
   public String getParentId()
   {
      return null;
   }

   /**
    * Check if this page should be visible for current object.
    *
    * @return true if this page should be visible
    */
   public boolean isVisible()
   {
      return true;
   }

   /**
    * Get page priority. Default is 65535.
    *
    * @return page priority
    */
   public int getPriority()
   {
      return 65535;
   }

   /**
    * @return the messageArea
    */
   public MessageAreaHolder getMessageArea()
   {
      return messageArea;
   }

   /**
    * @param messageArea the messageArea to set
    */
   public void setMessageArea(MessageAreaHolder messageArea)
   {
      this.messageArea = messageArea;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      if (isControlCreated())
         return applyChanges(false);
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      if (isControlCreated())
         applyChanges(true);
   }

   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected abstract boolean applyChanges(final boolean isApply);
}
