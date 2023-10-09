/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * Implements command, text box - vertical list of status texts and hyperlink actions to update them
 */
public class CommandTextBox extends Composite implements DisposeListener
{
   private List<CommandEntry> actions = new ArrayList<CommandEntry>();
   private Map<ImageDescriptor, Image> imageCache = new HashMap<ImageDescriptor, Image>();
   private Cursor cursor;

   /**
    * @param parent
    * @param style
    */
   public CommandTextBox(Composite parent, int style)
   {
      super(parent, style);

      setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      cursor = new Cursor(getDisplay(), SWT.CURSOR_HAND);

      RowLayout layout = new RowLayout();
      layout.type = SWT.VERTICAL;
      setLayout(layout);

      addDisposeListener(this);
   }

   /**
    * Rebuild command box
    */
   public void rebuild()
   {
      for(CommandEntry entry : actions)
      {
         entry.textLabel = new Text(this, SWT.LEFT);
         entry.textLabel.setText(entry.text);
         entry.textLabel.setForeground(getDisplay().getSystemColor(SWT.COLOR_LIST_FOREGROUND));
         entry.textLabel.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

         CLabel label = new CLabel(this, SWT.LEFT);
         label.setText(entry.action.getText());
         label.setImage(getImage(entry.action));
         label.setCursor(cursor);
         label.setForeground(getDisplay().getSystemColor(SWT.COLOR_LIST_FOREGROUND));
         label.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
         label.addMouseListener(new MouseListener() {
            @Override
            public void mouseDoubleClick(MouseEvent e)
            {
            }

            @Override
            public void mouseDown(MouseEvent e)
            {
            }

            @Override
            public void mouseUp(MouseEvent e)
            {
               if (e.button == 1)
                  entry.action.run();
            }
         });
      }
      layout();
   }

   /**
    * Get (possibly cached) image for given action
    * 
    * @param action action
    * @return image for given action
    */
   private Image getImage(Action action)
   {
      ImageDescriptor d = action.getImageDescriptor();
      Image img;
      if (d != null)
      {
         img = imageCache.get(d);
         if (img == null)
         {
            img = d.createImage();
            imageCache.put(d, img);
         }
      }
      else
      {
         img = SharedIcons.IMG_EMPTY;
      }
      return img;
   }

   /**
    * Add new action or update text of existing one
    * 
    * @param action action
    * @param doRebuild if set to true, rebuild() will be called
    */
   public void addOrUpdate(Action action, String text, boolean doRebuild)
   {
      boolean found = false;
      for(CommandEntry entry : actions)
      {
         if (action == entry.action)
         {
            entry.text = text;
            if (entry.textLabel != null)
            {
               entry.textLabel.setText(text);
               found = true;
               if (doRebuild)
                  rebuild();
               else
                  layout();
            }
         }
      }

      if (!found)
      {
         actions.add(new CommandEntry(action, text));
         if (doRebuild)
            rebuild();
      }
   }

   /**
    * Delete all actions
    * 
    * @param doRebuild if set to true, rebuild() will be called
    */
   public void deleteAll(boolean doRebuild)
   {
      actions.clear();
      for(Control c : getChildren())
         c.dispose();
      if (doRebuild)
         rebuild();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
    */
   @Override
   public void widgetDisposed(DisposeEvent e)
   {
      for(Image i : imageCache.values())
         i.dispose();
      cursor.dispose();
   }

   class CommandEntry
   {
      public Action action;
      public String text;
      Text textLabel;

      public CommandEntry(Action action, String text)
      {
         this.action = action;
         this.text = text;
         textLabel = null;
      }
   }
}
