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

import org.slf4j.ILoggerFactory;
import org.slf4j.spi.LoggerFactoryBinder;

/**
 * Static logger binder for SLF4J
 *
 */
public class StaticLoggerBinder implements LoggerFactoryBinder
{
   private static final StaticLoggerBinder singleton = new StaticLoggerBinder();
   private static final String loggerFactoryClassStr = EclipseLoggerFactory.class.getName();

   /**
    * The ILoggerFactory instance returned by the {@link #getLoggerFactory} method should always be the same object
    */
   private final ILoggerFactory loggerFactory;

   /**
    * Declare the version of the SLF4J API this implementation is compiled against. The value of this field is modified with each
    * major release. To avoid constant folding by the compiler, this field must *not* be final.
    */
   public static String REQUESTED_API_VERSION = "1.6.99"; // !final

   /**
    * Return the singleton of this class.
    * 
    * @return the StaticLoggerBinder singleton
    */
   public static final StaticLoggerBinder getSingleton()
   {
      return singleton;
   }

   /**
    * 
    */
   private StaticLoggerBinder()
   {
      loggerFactory = new EclipseLoggerFactory();
   }

   /**
    * @see org.slf4j.spi.LoggerFactoryBinder#getLoggerFactory()
    */
   @Override
   public ILoggerFactory getLoggerFactory()
   {
      return loggerFactory;
   }

   /**
    * @see org.slf4j.spi.LoggerFactoryBinder#getLoggerFactoryClassStr()
    */
   @Override
   public String getLoggerFactoryClassStr()
   {
      return loggerFactoryClassStr;
   }
}
