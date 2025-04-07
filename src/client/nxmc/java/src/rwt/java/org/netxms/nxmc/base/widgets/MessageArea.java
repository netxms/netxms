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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * Message area
 */
public class MessageArea extends Canvas implements MessageAreaHolder
{
   private final I18n i18n = LocalizationHelper.getI18n(MessageArea.class);

   public static final int INFORMATION = 0;
   public static final int SUCCESS = 1;
   public static final int WARNING = 2;
   public static final int ERROR = 3;

   private static final int MARGIN_WIDTH = 4;
   private static final int MARGIN_HEIGHT = 4;
   private static final int MESSAGE_SPACING = 4;
   private static final int TEXT_MARGIN_WIDTH = 5;
   private static final int TEXT_MARGIN_HEIGHT = 5;

   private static Image[] icons = null;
   private static Image iconClose = null;
   private static Image iconShowAll = null;

   private int nextMessageId = 1;
   private List<Message> messages = new ArrayList<>(0);
   private long messageTimeout = 20000; // 20 seconds by default
   private Runnable timer = null;
   private ImageHyperlink buttonShowAll = null;
   private Shell popupListShell = null;

   /**
    * Create message area.
    *
    * @param parent parent composite
    * @param style style bits
    */
   public MessageArea(Composite parent, int style)
   {
      super(parent, style);

      if (icons == null)
      {
         icons = new Image[4];
         icons[INFORMATION] = ResourceManager.getImage("icons/messages/info.png");
         icons[SUCCESS] = ResourceManager.getImage("icons/messages/success.png");
         icons[WARNING] = ResourceManager.getImage("icons/messages/warning.png");
         icons[ERROR] = ResourceManager.getImage("icons/messages/error.png");
         iconClose = ResourceManager.getImage("icons/messages/close.png");
         iconShowAll = ResourceManager.getImage("icons/messages/show-all.png");
      }

      GridLayout layout = new GridLayout();
      layout.marginHeight = MARGIN_HEIGHT;
      layout.marginWidth = MARGIN_WIDTH;
      layout.verticalSpacing = MESSAGE_SPACING;
      layout.numColumns = 2;
      setLayout(layout);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String)
    */
   @Override
   public int addMessage(int level, String text)
   {
      return addMessage(level, text, false);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String, boolean)
    */
   @Override
   public int addMessage(int level, String text, boolean sticky)
   {
      return addMessage(level, text, sticky, null, null);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String, boolean, java.lang.String,
    *      java.lang.Runnable)
    */
   @Override
   public int addMessage(int level, String text, boolean sticky, String buttonText, Runnable action)
   {
      if (isDisposed())
         return -1;

      Message m = new Message(nextMessageId++, level, text, sticky, buttonText, action);

      if (!messages.isEmpty())
         messages.get(0).disposeControl();
      messages.add(0, m);

      m.control = new MessageComposite(this, m, (mc) -> deleteMessage(m.id));
      m.control.moveAbove(null);
      m.control.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      if (buttonShowAll == null)
      {
         buttonShowAll = new ImageHyperlink(this, SWT.NONE);
         buttonShowAll.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));
         buttonShowAll.setImage(iconShowAll);
         buttonShowAll.setToolTipText(i18n.tr("Show all notifications"));
         buttonShowAll.setText("(1)");
         buttonShowAll.addHyperlinkListener(new HyperlinkAdapter() {
            @Override
            public void linkActivated(HyperlinkEvent e)
            {
               showAllMessages();
            }
         });
      }
      else
      {
         buttonShowAll.setText("(" + Integer.toString(messages.size()) + ")");
      }

      if (timer == null)
      {
         timer = () -> onTimer();
         getDisplay().timerExec(1000, timer); // First message, start expiration check timer
      }

      getParent().layout(true, true);
      return m.id;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#deleteMessage(int)
    */
   @Override
   public void deleteMessage(int id)
   {
      if (id < 1)
         return;

      for(int i = 0; i < messages.size(); i++)
      {
         Message m = messages.get(i);
         if (m.id == id)
         {
            messages.remove(i);
            if (m.disposeControl() && !messages.isEmpty())
            {
               // Re-create composite for next hidden message, if any
               m = messages.get(0);
               m.control = new MessageComposite(this, m, (mc) -> deleteMessage(mc.message.id));
               m.control.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
               m.control.moveAbove(null);
            }
            if (messages.isEmpty() && (buttonShowAll != null))
            {
               buttonShowAll.dispose();
               buttonShowAll = null;
            }
            if (!messages.isEmpty())
            {
               buttonShowAll.setText("(" + Integer.toString(messages.size()) + ")");
            }
            getParent().layout(true, true);
            break;
         }
      }
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#clearMessages()
    */
   @Override
   public void clearMessages()
   {      
      if (messages.isEmpty())
         return;

      for(Message m : messages)
         m.disposeControl();
      messages.clear();

      if (buttonShowAll != null)
      {
         buttonShowAll.dispose();
         buttonShowAll = null;
      }

      getParent().layout(true, true);
   }

   /**
    * @return the messageTimeout
    */
   public long getMessageTimeout()
   {
      return messageTimeout;
   }

   /**
    * @param messageTimeout the messageTimeout to set
    */
   public void setMessageTimeout(long messageTimeout)
   {
      this.messageTimeout = messageTimeout;
   }

   /**
    * Process fired timer
    */
   private void onTimer()
   {
      if (isDisposed())
         return;

      long now = System.currentTimeMillis();
      for(int i = 0; i < messages.size(); i++)
      {
         Message m = messages.get(i);
         if (!m.sticky && (m.timestamp + messageTimeout < now))
         {
            deleteMessage(m.id);
            i--;
         }
      }

      if (!messages.isEmpty())
         getDisplay().timerExec(1000, timer);
      else
         timer = null;
   }

   /**
    * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      return messages.isEmpty() ? new Point(0, 0) : super.computeSize(wHint, hHint, changed);
   }

   /**
    * Show all messages
    */
   private void showAllMessages()
   {
      if (popupListShell != null)
      {
         if (!popupListShell.isDisposed())
            popupListShell.close();
         popupListShell = null;
      }

      final Rectangle bounds = buttonShowAll.getBounds();
      final Point p = toDisplay(new Point(bounds.x + bounds.width, bounds.y + bounds.height + 2));

      popupListShell = new Shell(getDisplay(), SWT.ON_TOP | SWT.MODELESS | SWT.DIALOG_TRIM);
      popupListShell.setText(i18n.tr("Messages"));

      GridLayout layout = new GridLayout();
      layout.marginHeight = MARGIN_HEIGHT;
      layout.marginWidth = MARGIN_WIDTH;
      layout.verticalSpacing = MESSAGE_SPACING;
      popupListShell.setLayout(layout);

      int count = 0;
      for(Message m : messages)
      {
         if ((count++ == 10) && (messages.size() > 11))
         {
            int r = messages.size() - 10;
            new Label(popupListShell, SWT.NONE).setText(i18n.tr("{0} more messages", r));
            break;
         }

         MessageComposite messageComposite = new MessageComposite(popupListShell, m, (mc) -> {
            deleteMessage(m.id);
            mc.dispose();
            if (popupListShell.getChildren().length > 0)
            {
               popupListShell.pack();
               popupListShell.setLocation(p.x - popupListShell.getBounds().width, p.y);
               popupListShell.forceFocus();
            }
            else
            {
               popupListShell.close();
               popupListShell = null;
            }
         });
         messageComposite.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }

      popupListShell.pack();
      popupListShell.open();

      popupListShell.setLocation(p.x - popupListShell.getBounds().width, p.y);
      popupListShell.forceFocus();
   }

   /**
    * Message to display
    */
   private static class Message
   {
      int id;
      int level;
      String text;
      long timestamp;
      boolean sticky;
      MessageComposite control;
      String buttonText;
      Runnable buttonAction;

      /**
       * Create new message.
       *
       * @param id assigned message ID
       * @param level message level
       * @param text message text
       */
      Message(int id, int level, String text, boolean sticky, String buttonText, Runnable buttonAction)
      {
         this.id = id;
         this.level = level;
         this.text = text;
         this.timestamp = System.currentTimeMillis();
         this.sticky = sticky;
         this.buttonText = buttonText;
         this.buttonAction = buttonAction;
      }

      /**
       * Dispose control displaying this message, if any
       * 
       * @return true if control was disposed
       */
      boolean disposeControl()
      {
         if (control == null)
            return false;

         control.dispose();
         control = null;
         return true;
      }

      /**
       * Get background color for this message.
       *
       * @return background color for this message
       */
      Color getBackgroundColor()
      {
         switch(level)
         {
            case SUCCESS:
               return ThemeEngine.getBackgroundColor("MessageArea.Success");
            case WARNING:
               return ThemeEngine.getBackgroundColor("MessageArea.Warning");
            case ERROR:
               return ThemeEngine.getBackgroundColor("MessageArea.Error");
            default:
               return ThemeEngine.getBackgroundColor("MessageArea.Info");
         }
      }

      /**
       * Get border color for this message.
       *
       * @return border color for this message
       */
      Color getBorderColor()
      {
         switch(level)
         {
            case SUCCESS:
               return ThemeEngine.getForegroundColor("MessageArea.Success");
            case WARNING:
               return ThemeEngine.getForegroundColor("MessageArea.Warning");
            case ERROR:
               return ThemeEngine.getForegroundColor("MessageArea.Error");
            default:
               return ThemeEngine.getForegroundColor("MessageArea.Info");
         }
      }
   }

   /**
    * Message composite - contains text and controls for single message
    */
   private class MessageComposite extends Canvas implements PaintListener
   {
      private Message message;

      /**
       * Create message composite for given message.
       *
       * @param message message
       */
      public MessageComposite(Composite parent, Message message, CloseHandler closeHandler)
      {
         super(parent, SWT.NONE);
         this.message = message;

         setBackground(message.getBackgroundColor());

         GridLayout layout = new GridLayout();
         layout.numColumns = (message.buttonAction != null) ? 4 : 3;
         layout.marginWidth = TEXT_MARGIN_WIDTH;
         layout.marginHeight = TEXT_MARGIN_HEIGHT;
         layout.horizontalSpacing = TEXT_MARGIN_WIDTH;
         setLayout(layout);

         Label icon = new Label(this, SWT.NONE);
         icon.setBackground(getBackground());
         icon.setImage(icons[message.level]);
         icon.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));

         Label text = new Label(this, SWT.NONE);
         text.setBackground(getBackground());
         text.setText(message.text);
         text.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

         if (message.buttonAction != null)
         {
            ImageHyperlink actionButton = new ImageHyperlink(this, SWT.NONE);
            actionButton.setBackground(getBackground());
            actionButton.setToolTipText(message.buttonText);
            actionButton.setText(message.buttonText);
            actionButton.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));
            actionButton.addHyperlinkListener(new HyperlinkAdapter() {
               @Override
               public void linkActivated(HyperlinkEvent e)
               {
                  message.buttonAction.run();
               }
            });
         }

         ImageHyperlink closeButton = new ImageHyperlink(this, SWT.NONE);
         closeButton.setBackground(getBackground());
         closeButton.setImage(iconClose);
         closeButton.setToolTipText(i18n.tr("Dismiss"));
         closeButton.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));
         closeButton.addHyperlinkListener(new HyperlinkAdapter() {
            @Override
            public void linkActivated(HyperlinkEvent e)
            {
               closeHandler.onClose(MessageComposite.this);
            }
         });

         addPaintListener(this);
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
         e.gc.setBackground(message.getBackgroundColor());
         e.gc.fillRoundRectangle(clientArea.x, clientArea.y, clientArea.width - 1, clientArea.height - 1, 4, 4);
         e.gc.setForeground(message.getBorderColor());
         e.gc.drawRoundRectangle(clientArea.x, clientArea.y, clientArea.width - 1, clientArea.height - 1, 4, 4);
      }
   }

   /**
    * Close handler for message composite
    */
   private interface CloseHandler
   {
      /**
       * Called when user press "close" button.
       *
       * @param mc owning message composite
       */
      public void onClose(MessageComposite mc);
   }
}
