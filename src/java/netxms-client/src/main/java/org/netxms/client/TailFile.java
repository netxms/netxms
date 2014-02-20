/**
 * 
 */
package org.netxms.client;

import java.io.File;

/**
 * Information about file and it's id
 * 
 */
public class TailFile
{
   private String id;
   private File file;

   /**
    * @return the id
    */
   public String getId()
   {
      return id;
   }

   /**
    * @param id the id to set
    */
   public void setId(String id)
   {
      this.id = id;
   }

   /**
    * @return the file
    */
   public File getFile()
   {
      return file;
   }

   /**
    * @param file the file to set
    */
   public void setFile(File file)
   {
      this.file = file;
   }

}
