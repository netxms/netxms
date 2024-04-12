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

import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;

/**
 * Property page - preference page extension that tracks OK/Apply.
 */
public abstract class PropertyPage extends PreferencePage
{
   private MessageAreaHolder messageArea;
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
    * @param title
    */
   public PropertyPage(String title, MessageAreaHolder messageArea)
   {
      super(title);
      this.messageArea = messageArea;
   }

   /**
    * @param title
    * @param image
    */
   public PropertyPage(String title, ImageDescriptor image, MessageAreaHolder messageArea)
   {
      super(title, image);
      this.messageArea = messageArea;
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

   /**
    * Get message area for use by background jobs.
    *
    * @param isApply true if job is part of "apply" operation
    * @return message area to use
    */
   protected MessageAreaHolder getMessageArea(boolean isApply)
   {
      if (isApply)
      {
         return new MessageAreaHolder() {
            @Override
            public void deleteMessage(int id)
            {
            }

            @Override
            public void clearMessages()
            {
               setErrorMessage(null);
            }

            @Override
            public int addMessage(int level, String text, boolean sticky, String buttonText, Runnable action)
            {
               int type;
               switch(level)
               {
                  case MessageArea.INFORMATION:
                  case MessageArea.SUCCESS:
                     type = IMessageProvider.INFORMATION;
                     break;
                  case MessageArea.WARNING:
                     type = IMessageProvider.WARNING;
                     break;
                  case MessageArea.ERROR:
                     type = IMessageProvider.ERROR;
                     break;
                  default:
                     type = IMessageProvider.NONE;
                     break;
               }
               setMessage(text, type);
               return 0;
            }

            @Override
            public int addMessage(int level, String text, boolean sticky)
            {
               return addMessage(level, text, false, null, null);
            }

            @Override
            public int addMessage(int level, String text)
            {
               return addMessage(level, text, false, null, null);
            }
         };
      }
      return messageArea;
   }
}
