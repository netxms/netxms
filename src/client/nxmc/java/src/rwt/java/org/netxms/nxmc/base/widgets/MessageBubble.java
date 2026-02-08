/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * Message bubble widget for chat messages
 */
public class MessageBubble extends Composite
{
   public static enum Type
   {
      ASSISTANT, USER
   }

   private CLabel header;
   private Text message;

   /**
    * @param parent
    */
   public MessageBubble(Composite parent, Type type, String label, String text)
   {
      super(parent, SWT.NONE);

      setData(RWT.CUSTOM_VARIANT, "MessageBubble_" + type.toString());

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      setLayout(layout);

      header = new CLabel(this, SWT.NONE);
      header.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      header.setBackground(getBackground());
      header.setImage(SharedIcons.IMG_COMMENTS);
      header.setText(label);

      message = new Text(this, SWT.MULTI | SWT.WRAP | SWT.READ_ONLY);
      message.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      message.setBackground(getBackground());
      message.setText(text);
   }

   /**
    * Set message text
    *
    * @param text new message text
    */
   public void setText(String text)
   {
      message.setText(text);
   }

   /**
    * Get message text
    * 
    * @return current message text
    */
   public String getText()
   {
      return message.getText();
   }
}
