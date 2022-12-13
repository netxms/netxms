/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.jar.Attributes;
import java.util.jar.Manifest;

/**
 * Bootstrap loader for standalone JAR
 */
public class BootstrapLoader
{
   public static void main(String[] args)
   {
      final ClassLoader bootClassLoader = BootstrapLoader.class.getClassLoader();
      URLClassLoader classLoader = null;
      try
      {
         List<URL> classPath = new ArrayList<URL>();

         // FInd correct manifest
         Manifest manifest = null;
         Enumeration<URL> manifests = bootClassLoader.getResources("META-INF/MANIFEST.MF");
         while(manifests.hasMoreElements())
         {
            manifest = new Manifest(manifests.nextElement().openStream());
            String mainClass = manifest.getMainAttributes().getValue("Main-Class");
            if (BootstrapLoader.class.getCanonicalName().equals(mainClass))
               break;
            manifest = null;
         }
         if (manifest == null)
         {
            System.err.println("Cannot find correct MANIFEST.MF");
            return;
         }

         // Read dependencies listed in manifest
         Attributes manifestAttributes = manifest.getMainAttributes();
         String dependencies = manifestAttributes.getValue("Class-Path");
         if (dependencies != null)
         {
            for(String jarName : dependencies.split(" "))
            {
               InputStream in = bootClassLoader.getResourceAsStream("BOOT-INF/core/" + jarName);
               if (in != null)
               {
                  File jarFile = extractLibrary(in, jarName.replace(".jar", "-"));
                  classPath.add(jarFile.toURI().toURL());
               }
            }
         }

         // Add core module
         String version = manifestAttributes.getValue("Package-Version");
         if (version != null)
         {
            String bootstrapPackage = manifestAttributes.getValue("Bootstrap-Package");
            if (bootstrapPackage == null)
               bootstrapPackage = "nxmc";
            InputStream in = bootClassLoader.getResourceAsStream("BOOT-INF/core/" + bootstrapPackage + "-" + version + ".jar");
            if (in != null)
            {
               File jarFile = extractLibrary(in, bootstrapPackage + "-");
               classPath.add(jarFile.toURI().toURL());
            }
         }

         // Add SWT module
         String os = System.getProperty("os.name").toLowerCase();
         String arch = System.getProperty("os.arch").toLowerCase();
         String swtVariant = null;
         if (os.contains("windows"))
         {
            swtVariant = "win32.win32.x86_64";
         }
         else if (os.equals("linux"))
         {
            if (arch.equals("amd64"))
               swtVariant = "gtk.linux.x86_64";
            else if (arch.equals("aarch64"))
               swtVariant = "gtk.linux.aarch64";
         }
         else if (os.contains("mac"))
         {
            if (arch.equals("amd64"))
               swtVariant = "cocoa.macosx.x86_64";
            else if (arch.equals("aarch64"))
               swtVariant = "cocoa.macosx.aarch64";
         }
         if (swtVariant == null)
         {
            System.err.println("Unsupported OS/architecture combination (os=" + os + " arch=" + arch + ")");
            return;
         }

         String swtVersion = manifestAttributes.getValue("SWT-Version");
         InputStream in = BootstrapLoader.class.getClassLoader().getResourceAsStream("BOOT-INF/swt/org.eclipse.swt." + swtVariant + "-" + swtVersion + ".jar");
         if (in == null)
         {
            System.err.println("Cannot load SWT library for detected OS/architecture combination (os=" + os + " arch=" + arch + ")");
            return;
         }
         File swtJar = extractLibrary(in, "swt-" + swtVersion + "-");
         classPath.add(swtJar.toURI().toURL());

         // Create class loader and load startup class
         classLoader = new URLClassLoader("nxmc-loader", classPath.toArray(new URL[classPath.size()]), bootClassLoader);
         final Class<?> startupClass = classLoader.loadClass("org.netxms.nxmc.Startup");
         Method main = startupClass.getMethod("main", String[].class);
         main.invoke(null, new Object[] { args });
      }
      catch(Exception e)
      {
         e.printStackTrace();
      }
      finally
      {
         if (classLoader != null)
         {
            try
            {
               classLoader.close();
            }
            catch(IOException e)
            {
               e.printStackTrace();
            }
         }
      }
   }

   /**
    * Extract library from main JAR.
    *
    * @param in input stream for jar file encoded as resource
    * @param name base name for temporary file
    * @return temporary file with extracted library
    * @throws Exception on any error
    */
   private static File extractLibrary(InputStream in, String name) throws Exception
   {
      File jarFile = File.createTempFile(name, ".jar");
      jarFile.deleteOnExit();
      OutputStream out = new FileOutputStream(jarFile);
      try
      {
         byte[] buffer = new byte[16384];
         int length;
         while((length = in.read(buffer)) > 0)
            out.write(buffer, 0, length);
      }
      finally
      {
         out.close();
      }
      return jarFile;
   }
}
