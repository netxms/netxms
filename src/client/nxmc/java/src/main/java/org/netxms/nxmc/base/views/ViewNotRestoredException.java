package org.netxms.nxmc.base.views;

/**
 * Exception thrown when node 
 */
public class ViewNotRestoredException extends Exception
{
   private static final long serialVersionUID = 7195603121566698942L; 
   
   /**
    * Create exception with description message and exception
    * 
    * @param message
    * @param cause
    */
   public ViewNotRestoredException(String message)
   {
      super(message);
   }
   
   /**
    * Create exception with description message and exception
    * 
    * @param message
    * @param cause
    */
   public ViewNotRestoredException(String message, Throwable cause)
   {
      super(message, cause);
   }
}
