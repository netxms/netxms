package org.netxms.client;

import java.io.File;

/**
 * Class that returns received file and 
 *
 */
public class ReceivedFile
{
   public static final int SUCCESS = 0;
   public static final int CANCELED = 1;
   public static final int FAILED = 2;
   public static final int TIMEOUT = 3;
   
   private File file;
   private int status;
   
   /*
    * Constructor
    */
   public ReceivedFile(File f, int status)
   {
      file = f;
      this.status = status;
   }

   /**
    * Get status of receive operation
    * 
    * @return the status
    */
   public int getStatus()
   {
      return status;
   }

   /**
    * Get received file
    * 
    * @return the file
    */
   public File getFile()
   {
      return file;
   }
   
   /**
    * Method that checks if error should be generated
    * 
    * @return true if file transfer operation has failed
    */
   public boolean isFailed()
   {
      return status == FAILED || status == TIMEOUT;
   }
}
