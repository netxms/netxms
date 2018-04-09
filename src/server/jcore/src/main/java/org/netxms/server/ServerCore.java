package org.netxms.server;

import org.netxms.bridge.Platform;

/**
 * Bridge class for calls from C++ side
 */
public final class ServerCore
{
   private static final String DEBUG_TAG = "java.core";
   
   /**
    * Initialize Java part
    * 
    * @return true on success
    */
   public static boolean initialize()
   {
      Platform.writeDebugLog(DEBUG_TAG,  3, "Java core initialization completed");
      return true;
   }
   
   /**
    * Notify all Java components of server shutdown (called when all components are still active)
    */
   public static void initiateShutdown()
   {  
      Platform.writeDebugLog(DEBUG_TAG,  1, "Java core shutdown initiated");
   }
   
   /**
    * Notify Java components that server shutdown is completed and no activity should be performed beyond that point
    */
   public static void shutdownCompleted()
   { 
      Platform.writeDebugLog(DEBUG_TAG,  1, "Java core shutdown completed");
   }
}
