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
package org.netxms.nxmc.modules.filemanager.views.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.server.RemoteFile;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Base label provider for various file lists
 */
public class BaseFileLabelProvider extends LabelProvider
{
   private Map<String, Image> images;
   
   /**
    * Constructor
    */
   public BaseFileLabelProvider()
   {
      images = new HashMap<String, Image>();
      images.put("folder", ResourceManager.getImageDescriptor("icons/folder.png").createImage());
      images.put("unknown", ResourceManager.getImageDescriptor("icons/file-types/unknown.png").createImage());
      images.put("exe", ResourceManager.getImageDescriptor("icons/file-types/exec.png").createImage());
      images.put("pdf", ResourceManager.getImageDescriptor("icons/file-types/pdf.png").createImage());
      images.put("xls", ResourceManager.getImageDescriptor("icons/file-types/xls.png").createImage());
      images.put("ppt", ResourceManager.getImageDescriptor("icons/file-types/powerpoint.png").createImage());
      images.put("html", ResourceManager.getImageDescriptor("icons/file-types/html.png").createImage());
      images.put("txt", ResourceManager.getImageDescriptor("icons/file-types/text.png").createImage());
      images.put("avi", ResourceManager.getImageDescriptor("icons/file-types/avi.png").createImage());
      images.put("mp4", ResourceManager.getImageDescriptor("icons/file-types/mp4.png").createImage());
      images.put("ac3", ResourceManager.getImageDescriptor("icons/file-types/audio.png").createImage());
      images.put("tar", ResourceManager.getImageDescriptor("icons/file-types/archive.png").createImage());
   }
   
   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      for(Image img : images.values())
         img.dispose();
      images.clear();
      super.dispose();
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
   @Override
   public Image getImage(Object element)
   {
      if (((RemoteFile)element).isPlaceholder())
         return null;

      if (((RemoteFile)element).isDirectory())
         return images.get("folder");

      String[] parts = ((RemoteFile)element).getName().split("\\.");
      if (parts.length < 2)
         return images.get("unknown");

      String ext = parts[parts.length - 1];
      
      if (ext.equalsIgnoreCase("exe")) 
         return images.get("exe");

      if (ext.equalsIgnoreCase("pdf")) 
         return images.get("pdf");

      if (ext.equalsIgnoreCase("xls") || ext.equalsIgnoreCase("xlsx")) 
         return images.get("xls");

      if (ext.equalsIgnoreCase("ppt") || ext.equalsIgnoreCase("pptx")) 
         return images.get("ppt");

      if (ext.equalsIgnoreCase("html") || ext.equalsIgnoreCase("htm")) 
         return images.get("html");
      
      if (ext.equalsIgnoreCase("txt") || ext.equalsIgnoreCase("log") || ext.equalsIgnoreCase("jrn")) 
         return images.get("txt");

      if (ext.equalsIgnoreCase("avi") || ext.equalsIgnoreCase("mkv") || ext.equalsIgnoreCase("mov") || ext.equalsIgnoreCase("wma")) 
         return images.get("avi");

      if (ext.equalsIgnoreCase("mp4"))
         return images.get("mp4");

      if (ext.equalsIgnoreCase("ac3") || ext.equalsIgnoreCase("mp3") || ext.equalsIgnoreCase("wav")) 
         return images.get("ac3");

      if (ext.equalsIgnoreCase("tar") || ext.equalsIgnoreCase("gz") ||  
          ext.equalsIgnoreCase("tgz") || ext.equalsIgnoreCase("zip") || 
          ext.equalsIgnoreCase("rar") || ext.equalsIgnoreCase("7z") || 
          ext.equalsIgnoreCase("bz2") || ext.equalsIgnoreCase("lzma")) 
         return images.get("tar");

      return images.get("unknown");
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
   @Override
   public String getText(Object element)
   {
      return ((RemoteFile)element).getName();
   }

   /**
    * Get file size as string, with multipliers as needed.
    *
    * @param size file size
    * @return size as string
    */
   protected static String getSizeString(long size)
   {
      if (size >= 10995116277760L)
      {
         return String.format("%.1f TB", (size / 1099511627776.0));
      }
      if (size >= 10737418240L)
      {
         return String.format("%.1f GB", (size / 1073741824.0));
      }
      if (size >= 10485760)
      {
         return String.format("%.1f MB", (size / 1048576.0));
      }
      if (size >= 10240)
      {
         return String.format("%.1f KB", (size / 1024.0));
      }
      return Long.toString(size);
   }
}
