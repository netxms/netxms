/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.base.propertypages;

import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.resource.ImageDescriptor;

/**
 * Property page - preference page extension that tracks OK/Apply.
 */
public abstract class PropertyPage extends PreferencePage
{
   private boolean changed = false;

   /**
    * @param title
    */
   public PropertyPage(String title)
   {
      super(title);
   }

   /**
    * @param title
    * @param image
    */
   public PropertyPage(String title, ImageDescriptor image)
   {
      super(title, image);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected final void performApply()
   {
      if (!isControlCreated())
         return;

      changed = true;
      applyChanges(true);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public final boolean performOk()
   {
      if (!isControlCreated())
         return true;

      if (!applyChanges(false))
         return false;
      changed = true;
      return super.performOk();
   }

   /**
    * Apply changes.
    *
    * @param isApply true if called by "Apply" button
    * @return false to cancel dialog closing
    */
   protected abstract boolean applyChanges(final boolean isApply);

   /**
    * Check if changes were saved by pressing "Apply" or "OK".
    *
    * @return true if changes were saved by pressing "Apply" or "OK".
    */
   public boolean isChanged()
   {
      return changed;
   }
}
