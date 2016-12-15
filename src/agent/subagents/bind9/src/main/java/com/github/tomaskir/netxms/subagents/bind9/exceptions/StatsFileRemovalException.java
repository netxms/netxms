package com.github.tomaskir.netxms.subagents.bind9.exceptions;

/**
 * @author Tomas Kirnak
 * @since 2.1-M1
 */
public final class StatsFileRemovalException extends Exception
{
   private static final long serialVersionUID = 1L;

   public StatsFileRemovalException(Throwable cause)
   {
      super(cause);
   }
}
