/**
 * NetXMS Java Bridge
 * Copyright (C) 2013 TEMPEST a.s.
 * Copyright (C) 2014-2017 Raden Solutions
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
package org.netxms.bridge;

public class ConfigEntry
{
   volatile long configEntryHandle; // actually a pointer to native ConfigEntry

   ConfigEntry(long configEntryHandle)
   {
      this.configEntryHandle = configEntryHandle;
   }

   public native ConfigEntry getNext();

   public native ConfigEntry getParent();

   public native String getName();

   public native void setName(String name);

   public native int getId();

   public native int getValueCount();

   public native int getLine();

   public native void addValue(String value);

   public native void setValue(String value);

   public native String getFile();

   public native ConfigEntry createEntry(String name);

   public native ConfigEntry findEntry(String name);

   public native ConfigEntry[] getSubEntries(String mask);

   public native ConfigEntry[] getOrderedSubEntries(String mask);

   public native void unlinkEntry(ConfigEntry configEntry);

   public native String getValue(int index); // string
   // public native String getValue(int index, String defaultValue); // string with default value - does not actully exist in
   // nxcofig.h

   public native int getValueInt(int index, int defaultValue); // signed int32

   public native long getValueLong(int index, long defaultValue); // singed int64

   public native boolean getValueBoolean(int index, boolean defaultValue); // boolean
   // public native double getValueDouble(int index, double defaultValue); // double

   public native String getSubEntryValue(String name, int index, String defaultValue); // string

   public native int getSubEntryValueInt(String name, int index, int defaultValue); // signed int32

   public native long getSubEntryValueLong(String name, int index, long defaultValue); // singed int64

   public native boolean getSubEntryValueBoolean(String name, int index, boolean defaultValue); // boolean
   // public native double getSubEntryValueDouble(String name, int index, double defaultValue); // double
}
