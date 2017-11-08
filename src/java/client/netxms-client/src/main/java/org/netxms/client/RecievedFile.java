package org.netxms.client;

import java.io.File;

/**
 * Class that returns recieved file and 
 *
 */
public class RecievedFile
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
   public RecievedFile(File f, int status)
   {
      file = f;
      this.status = status;
   }

   /**
    * @return the status
    */
   public int getStatus()
   {
      return status;
   }

   /**
    * @return the file
    */
   public File getFile()
   {
      return file;
   }
   
   /**
    * Method that checks if error should be generated
    */
   public boolean isErrorRecieved()
   {
      return status == FAILED || status == TIMEOUT;
   }
}
