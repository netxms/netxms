/**
 * 
 */
package org.netxms.server;

/**
 * Access to server configuration
 */
public final class ServerConfiguration
{
   /**
    * Read server configuration variable
    * 
    * @param name variable name
    * @return variable value or null if not exist
    */
   private static native String read(String name);

   /**
    * Read server configuration variable as string
    * 
    * @param name variable name
    * @param defaultValue default value
    * @return variable value or default value if not exist
    */
   public static String readAsString(String name, String defaultValue)
   {
      String v = read(name);
      return (v != null) ? v : defaultValue;
   }
   
   /**
    * Read server configuration variable as integer
    * 
    * @param name variable name
    * @param defaultValue default value
    * @return variable value or default value if not exist or not a number
    */
   public static int readAsInteger(String name, int defaultValue)
   {
      String v = read(name);
      if (v == null)
         return defaultValue;
      try
      {
         return Integer.parseInt(v);
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }
   
   /**
    * Read server configuration variable as long
    * 
    * @param name variable name
    * @param defaultValue default value
    * @return variable value or default value if not exist or not a number
    */
   public static long readAsLong(String name, long defaultValue)
   {
      String v = read(name);
      if (v == null)
         return defaultValue;
      try
      {
         return Long.parseLong(v);
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }
   
   /**
    * Read server configuration variable as boolean
    * 
    * @param name variable name
    * @param defaultValue default value
    * @return variable value or default value if not exist or not a number
    */
   public static boolean readAsBoolean(String name, boolean defaultValue)
   {
      String v = read(name);
      if (v == null)
         return defaultValue;
      if (v.equalsIgnoreCase("true"))
         return true;
      if (v.equalsIgnoreCase("false"))
         return false;
      try
      {
         return Integer.parseInt(v) != 0;
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }

   /**
    * Write server configuration variable
    * 
    * @param name variable name
    * @param value new variable value
    */
   public static native void write(String name, String value);

   /**
    * Write server configuration variable
    * 
    * @param name variable name
    * @param value new variable value
    */
   public static void write(String name, int value)
   {
      write(name, Integer.toString(value));
   }

   /**
    * Write server configuration variable
    * 
    * @param name variable name
    * @param value new variable value
    */
   public static void write(String name, long value)
   {
      write(name, Long.toString(value));
   }

   /**
    * Write server configuration variable
    * 
    * @param name variable name
    * @param value new variable value
    */
   public static void write(String name, boolean value)
   {
      write(name, value ? "1" : "0");
   }
}
