/**
 * 
 */
package org.netxms.reporting;

/**
 * Reporting server exception
 *
 */
public class ServerException extends Exception
{
   private static final long serialVersionUID = 1L;

   /**
    * @param msg error message
    */
   public ServerException(String msg)
   {
      super(msg);
   }

   /**
    * @param msg error message
    * @param e root cause exception
    */
   public ServerException(String msg, Throwable e)
   {
      super(msg, e);
   }
}
