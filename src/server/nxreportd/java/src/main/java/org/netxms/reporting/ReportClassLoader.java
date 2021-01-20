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
package org.netxms.reporting;

import java.net.URL;
import java.net.URLClassLoader;

/**
 * Custom class loader for reporting engine
 */
public class ReportClassLoader extends URLClassLoader
{
   /**
    * Create new class loader.
    *
    * @param urls list of URLs
    * @param parent parent class loader
    */
   public ReportClassLoader(URL[] urls, ClassLoader parent)
   {
      super(urls, parent);
   }

   /**
    * @see java.lang.ClassLoader#loadClass(java.lang.String)
    */
   @Override
   public Class<?> loadClass(String name) throws ClassNotFoundException
   {
      if (!"report.DataSource".equals(name))
      {
         return super.loadClass(name);
      }
      return findClass(name);
   }
}
