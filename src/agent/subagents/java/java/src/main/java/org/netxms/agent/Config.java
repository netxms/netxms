/**
 * Java-Bridge NetXMS subagent
 * Copyright (C) 2013 TEMPEST a.s.
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
package org.netxms.agent;

public final class Config
{
   private volatile long configHandle; // actually a pointer to native Config

   private Config(long configHandle)
   {
      this.configHandle = configHandle;
   }

   public native void lock();

   public native void unlock();

   public native void deleteEntry(String path);

   public native ConfigEntry getEntry(String path);

   public native ConfigEntry[] getSubEntries(String path, String mask);

   public native ConfigEntry[] getOrderedSubEntries(String path, String mask);

   public native String getValue(String path, String defaultValue); // string

   public native int getValueInt(String path, int defaultValue); // signed int32

   public native long getValueLong(String path, long defaultValue); // signed int64

   public native boolean getValueBoolean(String path, boolean defaultValue); // boolean
   // public native double getValueDouble(String path, double defaultValue); // double
   // why is there no getValueDouble in nxconfig.h while there is setValue(path, double) ?

   public native boolean setValue(String path, String value); // string

   public native boolean setValue(String path, int value); // signed int32

   public native boolean setValue(String path, long value); // signed int64
   // public native boolean setValue(String path, boolean value); // boolean
   // why is there no setValue(path, boolean) in nxconfig.h while there is getValueBoolean?

   public native boolean setValue(String path, double value); // double
}
