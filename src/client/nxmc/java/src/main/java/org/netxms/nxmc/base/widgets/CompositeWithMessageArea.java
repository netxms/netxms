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
package org.netxms.nxmc.base.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;

/**
 * Composite capable of showing message bar on top of main content. Can be either subclassed (then createContent method should be
 * overridden) or used directly (then getContent method should be used to obtain parent for containing widgets).
 */
public class CompositeWithMessageArea extends Composite implements MessageAreaHolder
{
   private MessageArea messageArea;
   private Composite content;

   /**
    * @param parent
    * @param style
    */
   public CompositeWithMessageArea(Composite parent, int style)
   {
      super(parent, style);
      
      setLayout(new FormLayout());
      
      messageArea = new MessageArea(this, SWT.NONE);
      FormData fd = new FormData();
      fd.top = new FormAttachment(0, 0);
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      
      content = createContent(this);
      fd = new FormData();
      fd.top = new FormAttachment(messageArea, 0, SWT.BOTTOM);
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      content.setLayoutData(fd);
   }
   
   /**
    * Create content. Default implementation creates empty composite.
    * 
    * @param parent
    */
   protected Composite createContent(Composite parent)
   {
      Composite c = new Composite(parent, SWT.NONE);
      c.setLayout(new FillLayout());
      return c;
   }

   /**
    * Set given composite as new content
    * 
    * @param c
    */
   public void setContent(Composite c)
   {
      if (content != null)
         content.dispose();
      content = c;
      FormData fd = new FormData();
      fd.top = new FormAttachment(messageArea, 0, SWT.BOTTOM);
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      content.setLayoutData(fd);
   }

   /**
    * @return the content
    */
   public Composite getContent()
   {
      return content;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String)
    */
   @Override
   public int addMessage(int level, String text)
   {
      return messageArea.addMessage(level, text);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String, boolean)
    */
   @Override
   public int addMessage(int level, String text, boolean sticky)
   {
      return messageArea.addMessage(level, text, sticky);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#deleteMessage(int)
    */
   @Override
   public void deleteMessage(int id)
   {
      messageArea.deleteMessage(id);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#clearMessages()
    */
   @Override
   public void clearMessages()
   {
      messageArea.clearMessages();
   }
}
