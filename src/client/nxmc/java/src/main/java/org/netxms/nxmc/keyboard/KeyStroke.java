/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
import org.apache.commons.lang3.SystemUtils;
import org.eclipse.swt.SWT;
import org.netxms.nxmc.Registry;

/**
 * Represents keystroke.
 */
public class KeyStroke
{
   private static final Map<String, Integer> keyCodeLookupTable = new HashMap<>();
   private static final Map<Integer, String> keyNameLookupTable = new HashMap<>();

   static
   {
      keyCodeLookupTable.put("BACKSPACE", 8);
      keyCodeLookupTable.put("BREAK", SWT.BREAK);
      keyCodeLookupTable.put("DELETE", 127);
      keyCodeLookupTable.put("DOWN", SWT.ARROW_DOWN);
      keyCodeLookupTable.put("END", SWT.END);
      keyCodeLookupTable.put("ENTER", 13);
      keyCodeLookupTable.put("ESC", 27);
      keyCodeLookupTable.put("F1", SWT.F1);
      keyCodeLookupTable.put("F2", SWT.F2);
      keyCodeLookupTable.put("F3", SWT.F3);
      keyCodeLookupTable.put("F4", SWT.F4);
      keyCodeLookupTable.put("F5", SWT.F5);
      keyCodeLookupTable.put("F6", SWT.F6);
      keyCodeLookupTable.put("F7", SWT.F7);
      keyCodeLookupTable.put("F8", SWT.F8);
      keyCodeLookupTable.put("F9", SWT.F9);
      keyCodeLookupTable.put("F10", SWT.F10);
      keyCodeLookupTable.put("F11", SWT.F11);
      keyCodeLookupTable.put("F12", SWT.F12);
      keyCodeLookupTable.put("F13", SWT.F13);
      keyCodeLookupTable.put("F14", SWT.F14);
      keyCodeLookupTable.put("F15", SWT.F15);
      keyCodeLookupTable.put("F16", SWT.F16);
      keyCodeLookupTable.put("F17", SWT.F17);
      keyCodeLookupTable.put("F18", SWT.F18);
      keyCodeLookupTable.put("F19", SWT.F19);
      keyCodeLookupTable.put("F20", SWT.F20);
      keyCodeLookupTable.put("HELP", SWT.HELP);
      keyCodeLookupTable.put("HOME", SWT.HOME);
      keyCodeLookupTable.put("INSERT", SWT.INSERT);
      keyCodeLookupTable.put("LEFT", SWT.ARROW_LEFT);
      keyCodeLookupTable.put("PAGEDOWN", SWT.PAGE_DOWN);
      keyCodeLookupTable.put("PAGEUP", SWT.PAGE_UP);
      keyCodeLookupTable.put("PAUSE", SWT.PAUSE);
      keyCodeLookupTable.put("PRINTSCREEN", SWT.PRINT_SCREEN);
      keyCodeLookupTable.put("RIGHT", SWT.ARROW_RIGHT);
      keyCodeLookupTable.put("SCROLLLOCK", SWT.SCROLL_LOCK);
      keyCodeLookupTable.put("TAB", 9);
      keyCodeLookupTable.put("UP", SWT.ARROW_UP);

      keyNameLookupTable.put(8, "Backspace");
      keyNameLookupTable.put(9, "Tab");
      keyNameLookupTable.put(13, "Enter");
      keyNameLookupTable.put(27, "Esc");
      keyNameLookupTable.put(127, "Delete");
      keyNameLookupTable.put(SWT.BREAK, "Break");
      keyNameLookupTable.put(SWT.ARROW_DOWN, "Down");
      keyNameLookupTable.put(SWT.END, "End");
      keyNameLookupTable.put(SWT.F1, "F1");
      keyNameLookupTable.put(SWT.F2, "F2");
      keyNameLookupTable.put(SWT.F3, "F3");
      keyNameLookupTable.put(SWT.F4, "F4");
      keyNameLookupTable.put(SWT.F5, "F5");
      keyNameLookupTable.put(SWT.F6, "F6");
      keyNameLookupTable.put(SWT.F7, "F7");
      keyNameLookupTable.put(SWT.F8, "F8");
      keyNameLookupTable.put(SWT.F9, "F9");
      keyNameLookupTable.put(SWT.F10, "F10");
      keyNameLookupTable.put(SWT.F11, "F11");
      keyNameLookupTable.put(SWT.F12, "F12");
      keyNameLookupTable.put(SWT.F13, "F13");
      keyNameLookupTable.put(SWT.F14, "F14");
      keyNameLookupTable.put(SWT.F15, "F15");
      keyNameLookupTable.put(SWT.F16, "F16");
      keyNameLookupTable.put(SWT.F17, "F17");
      keyNameLookupTable.put(SWT.F18, "F18");
      keyNameLookupTable.put(SWT.F19, "F19");
      keyNameLookupTable.put(SWT.F20, "F20");
      keyNameLookupTable.put(SWT.HELP, "Help");
      keyNameLookupTable.put(SWT.HOME, "Home");
      keyNameLookupTable.put(SWT.INSERT, "Insert");
      keyNameLookupTable.put(SWT.ARROW_LEFT, "Left");
      keyNameLookupTable.put(SWT.PAGE_DOWN, "PageDown");
      keyNameLookupTable.put(SWT.PAGE_UP, "PageUp");
      keyNameLookupTable.put(SWT.PAUSE, "Pause");
      keyNameLookupTable.put(SWT.PRINT_SCREEN, "PrintScreen");
      keyNameLookupTable.put(SWT.ARROW_RIGHT, "Right");
      keyNameLookupTable.put(SWT.SCROLL_LOCK, "ScrollLock");
      keyNameLookupTable.put(SWT.ARROW_UP, "Up");
   }

   /**
    * Parse keystroke definition in form (Modifier+)*Key. The recognized modifiers keys are M1, M2, M3, M4, ALT, COMMAND, CTRL, and
    * SHIFT. The "M" modifier keys are a platform-independent way of representing keys, and these are generally preferred. M1 is the
    * COMMAND key on MacOS X, and the CTRL key on most other platforms. M2 is the SHIFT key. M3 is the Option key on MacOS X, and
    * the ALT key on most other platforms. M4 is the CTRL key on MacOS X, and is undefined on other platforms.<br>
    * Examples:<br>
    * Ctrl+A<br>
    * Alt+Shift+F11<br>
    * M1+F3<br>
    *
    * @param definition keystroke definition
    * @return keystroke object
    */
   public static KeyStroke parse(String definition)
   {
      int modifiers = 0;
      boolean isMacOS = SystemUtils.IS_OS_MAC_OSX && !Registry.IS_WEB_CLIENT;

      String[] parts = definition.split("\\+");
      for(int i = 0; i < parts.length - 1; i++)
      {
         String m = parts[i];
         if (m.equalsIgnoreCase("Alt") || (m.equalsIgnoreCase("M3") && !isMacOS))
            modifiers |= SWT.ALT;
         else if (m.equalsIgnoreCase("Command") || (m.equalsIgnoreCase("M1") && isMacOS))
            modifiers |= SWT.COMMAND;
         else if (m.equalsIgnoreCase("Ctrl") || (m.equalsIgnoreCase("M1") && !isMacOS) || (m.equalsIgnoreCase("M4") && isMacOS))
            modifiers |= SWT.CTRL;
         else if (m.equalsIgnoreCase("Shift") || m.equalsIgnoreCase("M2"))
            modifiers |= SWT.SHIFT;
      }

      int key;
      String k = parts[parts.length - 1].toUpperCase();
      if (k.length() == 1)
      {
         key = k.charAt(0);
      }
      else
      {
         Integer code = keyCodeLookupTable.get(k);
         key = (code != null) ? code : 0;
      }

      return new KeyStroke(modifiers, key);
   }

   /**
    * Normalize keystroke definition.
    *
    * @param definition keystroke definition
    * @return normalized keystroke definition
    */
   public static String normalizeDefinition(String definition)
   {
      return parse(definition).toString();
   }

   private int modifiers;
   private int key;

   /**
    * Create keystroke from single key without modifiers
    * 
    * @param key key code
    */
   public KeyStroke(int key)
   {
      this.modifiers = 0;
      this.key = key;
   }

   /**
    * Create keystroke with modifiers
    * 
    * @param modifiers modifiers (bit mask from keyboard event)
    * @param key key code
    */
   public KeyStroke(int modifiers, int key)
   {
      this.modifiers = modifiers & (SWT.CTRL | SWT.ALT | SWT.SHIFT | SWT.COMMAND);
      if ((key >= 'a') && (key <= 'z'))
         this.key = Character.toUpperCase((char)key);
      else
         this.key = key;
   }

   /**
    * Check if this keystroke definition is valid.
    *
    * @return true if this keystroke definition is valid
    */
   public boolean isValid()
   {
      return key != 0;
   }

   /**
    * Get modifier bits.
    *
    * @return modifier bits
    */
   public int getModifiers()
   {
      return modifiers;
   }

   /**
    * Get key code.
    *
    * @return key code
    */
   public int getKey()
   {
      return key;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      StringBuilder sb = new StringBuilder();
      if ((modifiers & SWT.SHIFT) != 0)
         sb.append("Shift+");
      if ((modifiers & SWT.COMMAND) != 0)
         sb.append("Command+");
      if ((modifiers & SWT.CTRL) != 0)
         sb.append("Ctrl+");
      if ((modifiers & SWT.ALT) != 0)
         sb.append("Alt+");
      String keyName = keyNameLookupTable.get(key);
      if (keyName != null)
         sb.append(keyName);
      else
         sb.append(Character.toUpperCase((char)key));
      return sb.toString();
   }

   /**
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      final int prime = 31;
      int result = 1;
      result = prime * result + key;
      result = prime * result + modifiers;
      return result;
   }

   /**
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object object)
   {
      if (this == object)
         return true;
      if (object == null)
         return false;
      if (getClass() != object.getClass())
         return false;
      KeyStroke other = (KeyStroke)object;
      if (key != other.key)
         return false;
      if (modifiers != other.modifiers)
         return false;
      return true;
   }
}
