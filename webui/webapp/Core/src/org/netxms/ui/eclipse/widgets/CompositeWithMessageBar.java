/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.constants.Severity;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * Composite capable of showing message bar on top of main content.
 * Can be either subclassed (then createContent method should be overriden)
 * or used directly (then getContent method should be used to obtain parent for containing widgets).
 */
public class CompositeWithMessageBar extends Composite
{
   public static final int INFORMATION = 0;
   public static final int WARNING = 1;
   public static final int ERROR = 2;
   
   private Composite messageBar;
   private CLabel messageBarLabel;
   private Label closeButton;
   private Composite content;
   
   /**
    * @param parent
    * @param style
    */
   public CompositeWithMessageBar(Composite parent, int style)
   {
      super(parent, style);
      
      setLayout(new FormLayout());
      
      messageBar = new Composite(this, SWT.NONE);
      messageBar.setBackground(SharedColors.getColor(SharedColors.MESSAGE_BAR_BACKGROUND, getDisplay()));
      messageBar.setForeground(SharedColors.getColor(SharedColors.MESSAGE_BAR_TEXT, getDisplay()));
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      layout.numColumns = 2;
      messageBar.setLayout(layout);
      
      messageBarLabel = new CLabel(messageBar, SWT.NONE);
      messageBarLabel.setBackground(messageBar.getBackground());
      messageBarLabel.setForeground(messageBar.getForeground());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      messageBarLabel.setLayoutData(gd);
      
      closeButton = new Label(messageBar, SWT.NONE);
      closeButton.setBackground(messageBar.getBackground());
      closeButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      closeButton.setImage(SharedIcons.IMG_CLOSE);
      closeButton.setToolTipText("Hide message");
      gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      closeButton.setLayoutData(gd);
      closeButton.addMouseListener(new MouseListener() {
         private boolean doAction = false;
         
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            if (e.button == 1)
               doAction = false;
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 1)
               doAction = true;
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
            if ((e.button == 1) && doAction)
               hideMessage();
         }
      });
      
      Label separator = new Label(messageBar, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      separator.setLayoutData(gd);
      
      FormData fd = new FormData();
      fd.top = new FormAttachment(0, 0);
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      messageBar.setLayoutData(fd);
      
      content = createContent(this);
      fd = new FormData();
      fd.top = new FormAttachment(0, 0);
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
    * Show message in message bar
    * 
    * @param severity
    * @param text
    */
   public void showMessage(int severity, String text)
   {
      switch(severity)
      {
         case WARNING:
            messageBarLabel.setImage(StatusDisplayInfo.getStatusImage(Severity.WARNING));
            break;
         case ERROR:
            messageBarLabel.setImage(StatusDisplayInfo.getStatusImage(Severity.CRITICAL));
            break;
         default:
            messageBarLabel.setImage(SharedIcons.IMG_INFORMATION);
            break;
      }
      messageBarLabel.setText(text);
      messageBar.setVisible(true);
      ((FormData)content.getLayoutData()).top = new FormAttachment(messageBar, 0, SWT.BOTTOM);
      layout(true, true);
   }
   
   /**
    * Hide message bar
    */
   public void hideMessage()
   {
      messageBar.setVisible(false);
      ((FormData)content.getLayoutData()).top = new FormAttachment(0, 0);
      layout(true, true);
   }

   /**
    * @return the content
    */
   public Composite getContent()
   {
      return content;
   }
}
