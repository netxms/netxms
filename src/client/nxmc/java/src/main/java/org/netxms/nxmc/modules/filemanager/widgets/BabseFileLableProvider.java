package org.netxms.nxmc.modules.filemanager.widgets;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.server.AgentFile;
import org.netxms.nxmc.resources.ResourceManager;

public class BabseFileLableProvider extends LabelProvider
{
   private static Map<String, Image> images;
   
   static
   {
      images = new HashMap<String, Image>();
      images.put("folder", ResourceManager.getImageDescriptor("icons/folder.gif").createImage());
      images.put("unknown", ResourceManager.getImageDescriptor("icons/types/unknown.png").createImage());
      images.put("exe", ResourceManager.getImageDescriptor("icons/types/exec.png").createImage());
      images.put("pdf", ResourceManager.getImageDescriptor("icons/types/pdf.png").createImage());
      images.put("xls", ResourceManager.getImageDescriptor("icons/types/excel.png").createImage());
      images.put("ppt", ResourceManager.getImageDescriptor("icons/types/powerpoint.png").createImage());
      images.put("html", ResourceManager.getImageDescriptor("icons/types/html.png").createImage());
      images.put("txt", ResourceManager.getImageDescriptor("icons/types/text.png").createImage());
      images.put("avi", ResourceManager.getImageDescriptor("icons/objects/dashboard.gif").createImage());
      images.put("ac3", ResourceManager.getImageDescriptor("icons/types/audio.png").createImage());
      images.put("tar", ResourceManager.getImageDescriptor("icons/types/archive.png").createImage());
   }
   
   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
   @Override
   public Image getImage(Object element)
   {
      if (((AgentFile)element).isPlaceholder())
         return null;
      
      if (((AgentFile)element).isDirectory())
         return images.get("folder");
      
      String[] parts = ((AgentFile)element).getName().split("\\."); 
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
      return ((AgentFile)element).getName();
   }
}
