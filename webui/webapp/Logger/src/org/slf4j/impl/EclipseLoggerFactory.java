/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.slf4j.impl;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import org.slf4j.ILoggerFactory;
import org.slf4j.Logger;

/**
 * Logger factory for use with Eclipse plugins
 *
 */
public class EclipseLoggerFactory implements ILoggerFactory
{
   private ConcurrentMap<String, Logger> loggerMap = new ConcurrentHashMap<String, Logger>();

   /**
    * @see org.slf4j.ILoggerFactory#getLogger(java.lang.String)
    */
   @Override
   public Logger getLogger(String name)
   {
      Logger simpleLogger = loggerMap.get(name);
      if (simpleLogger != null)
      {
         return simpleLogger;
      }
      else
      {
         Logger newInstance = new EclipseLogger(name);
         Logger oldInstance = loggerMap.putIfAbsent(name, newInstance);
         return oldInstance == null ? newInstance : oldInstance;
      }
   }
}
