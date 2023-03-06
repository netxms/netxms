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
package org.netxms.nxmc.keyboard;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.jface.action.IAction;
import org.eclipse.swt.widgets.Display;

/**
 * Manager for key bindings
 */
public class KeyBindingManager
{
   private Map<KeyStroke, ActionBinding> bindings = new HashMap<>();

   /**
    * Add key binding to this manager.
    *
    * @param keyStroke activation keystroke
    * @param action action to execute
    */
   public void addBinding(KeyStroke keyStroke, IAction action)
   {
      addBinding(keyStroke, action, null);
   }

   /**
    * Add key binding to this manager.
    *
    * @param keyStroke activation keystroke
    * @param action action to execute
    * @param radioGroup group of radio button actions this action belongs to
    */
   public void addBinding(KeyStroke keyStroke, IAction action, String radioGroup)
   {
      bindings.put(keyStroke, new ActionBinding(action, radioGroup));
      String text = action.getText();
      if ((text != null) && (text.indexOf('\t') == -1))
         action.setText(text + "\t" + keyStroke.toString());
   }

   /**
    * Add key binding to this manager.
    *
    * @param definition activation keystroke definition
    * @param action action to execute
    */
   public void addBinding(String definition, IAction action)
   {
      addBinding(definition, action, null);
   }

   /**
    * Add key binding to this manager.
    *
    * @param definition activation keystroke definition
    * @param action action to execute
    * @param radioGroup group of radio button actions this action belongs to
    */
   public void addBinding(String definition, IAction action, String radioGroup)
   {
      KeyStroke keyStroke = KeyStroke.parse(definition);
      if (keyStroke.isValid())
         addBinding(keyStroke, action, radioGroup);
   }

   /**
    * Add key binding to this manager.
    *
    * @param modifiers modifier bits for keystroke definition
    * @param key key code for keystroke definition
    * @param action action to execute
    */
   public void addBinding(int modifiers, int key, IAction action)
   {
      addBinding(new KeyStroke(modifiers, key), action, null);
   }

   /**
    * Add key binding to this manager.
    *
    * @param modifiers modifier bits for keystroke definition
    * @param key key code for keystroke definition
    * @param action action to execute
    * @param radioGroup group of radio button actions this action belongs to
    */
   public void addBinding(int modifiers, int key, IAction action, String radioGroup)
   {
      addBinding(new KeyStroke(modifiers, key), action, radioGroup);
   }

   /**
    * Remove binding for given keystroke.
    *
    * @param keyStroke keystroke to remove
    */
   public void removeBinding(KeyStroke keyStroke)
   {
      bindings.remove(keyStroke);
   }

   /**
    * Remove binding for given keystroke.
    *
    * @param definition keystroke definition
    */
   public void removeBinding(String definition)
   {
      bindings.remove(KeyStroke.parse(definition));
   }

   /**
    * Remove binding for given keystroke.
    *
    * @param modifiers modifier bits for keystroke definition
    * @param key key code for keystroke definition
    */
   public void removeBinding(int modifiers, int key)
   {
      bindings.remove(new KeyStroke(modifiers, key));
   }

   /**
    * Remove binding for given action.
    *
    * @param action action to remove binding for
    */
   public void removeBinding(IAction action)
   {
      KeyStroke key = null;
      for(Entry<KeyStroke, ActionBinding> e : bindings.entrySet())
      {
         if (action.equals(e.getValue().action))
         {
            key = e.getKey();
            break;
         }
      }
      if (key != null)
         bindings.remove(key);
   }

   /**
    * Process keystroke.
    *
    * @param keyStroke keystroke to process
    * @return true if keystroke was mapped to action
    */
   public boolean processKeyStroke(KeyStroke keyStroke)
   {
      final ActionBinding actionBinding = bindings.get(keyStroke);
      if ((actionBinding == null) || !actionBinding.action.isEnabled())
         return false;

      Display.getCurrent().asyncExec(new Runnable() {
         @Override
         public void run()
         {
            IAction action = actionBinding.action;
            if ((action.getStyle() & IAction.AS_RADIO_BUTTON) != 0)
            {
               if (action.isChecked())
                  return; // Already selected, nothing to do

               if (actionBinding.radouGroup != null)
               {
                  for(ActionBinding b : bindings.values())
                     if (actionBinding.radouGroup.equals(b.radouGroup))
                        b.action.setChecked(false);
               }

               action.setChecked(true);
            }
            else if ((action.getStyle() & IAction.AS_CHECK_BOX) != 0)
            {
               action.setChecked(!action.isChecked());
            }
            action.run();
         }
      });
      return true;
   }

   /**
    * Action binding information
    */
   private static class ActionBinding
   {
      IAction action;
      String radouGroup;

      public ActionBinding(IAction action, String radouGroup)
      {
         this.action = action;
         this.radouGroup = radouGroup;
      }
   }
}
