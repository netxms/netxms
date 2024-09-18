/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.base.widgets.helpers;

import java.util.Stack;
import org.apache.commons.lang3.SystemUtils;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ExtendedModifyEvent;
import org.eclipse.swt.custom.ExtendedModifyListener;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;

/**
 * Undo/Redo implementation for styled text widget. Based on implementation by Petr Bodnar
 * 
 * @see {@linkplain https://sourceforge.net/p/etinyplugins/blog/2013/02/add-undoredo-support-to-your-swt-styledtext-s/)
 */
public class StyledTextUndoManager implements KeyListener, ExtendedModifyListener
{
   private StyledText editor;
   private Stack<ExtendedModifyEvent> undoStack = new Stack<>();
   private Stack<ExtendedModifyEvent> redoStack = new Stack<>();
   private boolean isUndo;
   private boolean isRedo;

   /**
    * Creates a new instance of this class. Automatically starts listening to corresponding key and modify events coming from the
    * given <var>editor</var>.
    * 
    * @param editor the text field to which the Undo-Redo functionality should be added
    */
   public StyledTextUndoManager(StyledText editor)
   {
      this.editor = editor;
      editor.addExtendedModifyListener(this);
      editor.addKeyListener(this);
   }

   /**
    * Reset undo/redo history
    */
   public void reset()
   {
      undoStack.clear();
      redoStack.clear();
   }

   /**
    * @see org.eclipse.swt.events.KeyListener#keyPressed(org.eclipse.swt.events.KeyEvent)
    */
   public void keyPressed(KeyEvent e)
   {
      // Listen to Ctrl/Command+Z for Undo, to Ctrl/Command+Y or Ctrl/Command+Shift+Z for Redo
      boolean isCtrl = SystemUtils.IS_OS_MAC_OSX ? ((e.stateMask & SWT.COMMAND) != 0) : ((e.stateMask & SWT.CTRL) != 0);
      if (isCtrl && ((e.stateMask & SWT.ALT) == 0))
      {
         boolean isShift = (e.stateMask & SWT.SHIFT) != 0;
         if (!isShift && (e.keyCode == 'z'))
         {
            undo();
         }
         else if ((!isShift && (e.keyCode == 'y')) || (isShift && (e.keyCode == 'z')))
         {
            redo();
         }
      }
   }

   /**
    * @see org.eclipse.swt.events.KeyListener#keyReleased(org.eclipse.swt.events.KeyEvent)
    */
   public void keyReleased(KeyEvent e)
   {
   }

   /**
    * Creates a corresponding Undo or Redo step from the given event and pushes it to the stack. The Redo stack is, logically,
    * emptied if the event comes from a normal user action.
    * 
    * @param event
    * @see org.eclipse.swt.custom.ExtendedModifyListener#modifyText(org.eclipse. swt.custom.ExtendedModifyEvent)
    */
   public void modifyText(ExtendedModifyEvent event)
   {
      if (isUndo)
      {
         redoStack.add(event);
      }
      else
      {
         // isRedo or a normal user action
         undoStack.add(event);
         if (!isRedo)
         {
            redoStack.clear();
         }
      }
   }

   /**
    * Performs the Undo action. A new corresponding Redo step is automatically pushed to the stack.
    */
   private void undo()
   {
      if (!undoStack.isEmpty())
      {
         isUndo = true;
         revertEvent(undoStack.pop());
         isUndo = false;
      }
   }

   /**
    * Performs the Redo action. A new corresponding Undo step is automatically pushed to the stack.
    */
   private void redo()
   {
      if (!redoStack.isEmpty())
      {
         isRedo = true;
         revertEvent(redoStack.pop());
         isRedo = false;
      }
   }

   /**
    * Reverts the given modify event, in the way as the Eclipse text editor does it.
    * 
    * @param event
    */
   private void revertEvent(ExtendedModifyEvent event)
   {
      editor.replaceTextRange(event.start, event.length, event.replacedText);
      // (causes the modifyText() listener method to be called)

      editor.setSelectionRange(event.start, event.replacedText.length());
   }
}
