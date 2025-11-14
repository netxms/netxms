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

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;

/**
 * Message bubble widget for chat messages
 */
public class MessageBubble extends Canvas implements PaintListener
{
   public static enum Type
   {
      ASSISTANT, USER
   }

   private static Font headerFont;

   private CLabel header;
   private Text message;
   private Color backgroundColor;
   private Color borderColor;

   /**
    * @param parent
    */
   public MessageBubble(Composite parent, Type type, String label, String text)
   {
      super(parent, SWT.TRANSPARENT | SWT.NO_BACKGROUND);

      if (headerFont == null)
      {
         FontData fd = getFont().getFontData()[0];
         fd.setStyle(SWT.BOLD);
         headerFont = new Font(getDisplay(), fd);
      }

      switch(type)
      {
         case ASSISTANT:
            backgroundColor = ThemeEngine.getBackgroundColor("AiAssistant.AssistantMessage");
            borderColor = ThemeEngine.getForegroundColor("AiAssistant.AssistantMessage");
            break;
         case USER:
            backgroundColor = ThemeEngine.getBackgroundColor("AiAssistant.UserMessage");
            borderColor = ThemeEngine.getForegroundColor("AiAssistant.UserMessage");
            break;
         default:
            backgroundColor = getDisplay().getSystemColor(SWT.COLOR_WHITE);
            borderColor = getDisplay().getSystemColor(SWT.COLOR_GRAY);
            break;
      }

      setBackground(getDisplay().getSystemColor(SWT.COLOR_TRANSPARENT));
      addPaintListener(this);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 12;
      layout.marginHeight = 8;
      setLayout(layout);

      header = new CLabel(this, SWT.NONE);
      header.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      header.setText(label);
      header.setBackground(backgroundColor);
      header.setFont(headerFont);
      header.setImage(SharedIcons.IMG_COMMENTS);

      message = new Text(this, SWT.MULTI | SWT.WRAP | SWT.READ_ONLY);
      message.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      message.setText(text);
      message.setBackground(backgroundColor);
   }

   /**
    * @see org.eclipse.swt.widgets.Widget#dispose()
    */
   @Override
   public void dispose()
   {
      removePaintListener(this);
      super.dispose();
   }

   /**
    * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
    */
   @Override
   public void paintControl(PaintEvent e)
   {
      Rectangle clientArea = getClientArea();
      e.gc.setAntialias(SWT.ON);
      e.gc.setBackground(backgroundColor);
      e.gc.fillRoundRectangle(clientArea.x, clientArea.y, clientArea.width - 1, clientArea.height - 1, 16, 16);
      e.gc.setForeground(borderColor);
      e.gc.drawRoundRectangle(clientArea.x, clientArea.y, clientArea.width - 1, clientArea.height - 1, 16, 16);
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
