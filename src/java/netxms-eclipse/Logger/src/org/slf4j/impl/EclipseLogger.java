/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.slf4j.impl;

import org.eclipse.core.runtime.ILog;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.netxms.ui.eclipse.console.Activator;
import org.slf4j.helpers.FormattingTuple;
import org.slf4j.helpers.MarkerIgnoringBase;
import org.slf4j.helpers.MessageFormatter;

/**
 * Logger for SLF4J -> Eclipse log
 */
public class EclipseLogger extends MarkerIgnoringBase
{
   private static final long serialVersionUID = 1L;

   private String name;
   private ILog log = Activator.getDefault().getLog();

   /**
    * Create new logger.
    *
    * @param name logger name
    */
   public EclipseLogger(String name)
   {
      super();
      this.name = name;
   }

   /**
    * Write message to log
    *
    * @param severity message severity
    * @param message message text
    * @param exception associated low-level exception
    */
   private void writeLog(int severity, String message, Throwable exception)
   {
      log.log(new Status(severity, name, message, exception));
   }

   /**
    * For formatted messages, first substitute arguments and then log.
    *
    * @param level
    * @param format
    * @param arg1
    * @param arg2
    */
   private void formatAndWriteLog(int severity, String format, Object arg1, Object arg2)
   {
      FormattingTuple tp = MessageFormatter.format(format, arg1, arg2);
      writeLog(severity, tp.getMessage(), tp.getThrowable());
   }

   /**
    * For formatted messages, first substitute arguments and then log.
    *
    * @param level
    * @param format
    * @param arguments a list of 3 ore more arguments
    */
   private void formatAndWriteLog(int severity, String format, Object... arguments)
   {
      FormattingTuple tp = MessageFormatter.arrayFormat(format, arguments);
      writeLog(severity, tp.getMessage(), tp.getThrowable());
   }

   /**
    * @see org.slf4j.Logger#debug(java.lang.String)
    */
   @Override
   public void debug(String message)
   {
      writeLog(IStatus.INFO, message, null);
   }

   /**
    * @see org.slf4j.Logger#debug(java.lang.String, java.lang.Object)
    */
   @Override
   public void debug(String message, Object arg1)
   {
      formatAndWriteLog(IStatus.INFO, message, arg1, null);
   }

   /**
    * @see org.slf4j.Logger#debug(java.lang.String, java.lang.Object[])
    */
   @Override
   public void debug(String message, Object... args)
   {
      formatAndWriteLog(IStatus.INFO, message, args);
   }

   /**
    * @see org.slf4j.Logger#debug(java.lang.String, java.lang.Throwable)
    */
   @Override
   public void debug(String message, Throwable exception)
   {
      formatAndWriteLog(IStatus.INFO, message, exception);
   }

   /**
    * @see org.slf4j.Logger#debug(java.lang.String, java.lang.Object, java.lang.Object)
    */
   @Override
   public void debug(String message, Object arg1, Object arg2)
   {
      formatAndWriteLog(IStatus.INFO, message, arg1, arg2);
   }

   /**
    * @see org.slf4j.Logger#error(java.lang.String)
    */
   @Override
   public void error(String message)
   {
      writeLog(IStatus.ERROR, message, null);
   }

   /**
    * @see org.slf4j.Logger#error(java.lang.String, java.lang.Object)
    */
   @Override
   public void error(String message, Object arg1)
   {
      formatAndWriteLog(IStatus.ERROR, message, arg1, null);
   }

   /**
    * @see org.slf4j.Logger#error(java.lang.String, java.lang.Object[])
    */
   @Override
   public void error(String message, Object... args)
   {
      formatAndWriteLog(IStatus.ERROR, message, args);
   }

   /**
    * @see org.slf4j.Logger#error(java.lang.String, java.lang.Throwable)
    */
   @Override
   public void error(String message, Throwable exception)
   {
      formatAndWriteLog(IStatus.ERROR, message, exception);
   }

   /**
    * @see org.slf4j.Logger#error(java.lang.String, java.lang.Object, java.lang.Object)
    */
   @Override
   public void error(String message, Object arg1, Object arg2)
   {
      formatAndWriteLog(IStatus.ERROR, message, arg1, arg2);
   }

   /**
    * @see org.slf4j.Logger#info(java.lang.String)
    */
   @Override
   public void info(String message)
   {
      writeLog(IStatus.INFO, message, null);
   }

   /**
    * @see org.slf4j.Logger#info(java.lang.String, java.lang.Object)
    */
   @Override
   public void info(String message, Object arg1)
   {
      formatAndWriteLog(IStatus.INFO, message, arg1, null);
   }

   /**
    * @see org.slf4j.Logger#info(java.lang.String, java.lang.Object[])
    */
   @Override
   public void info(String message, Object... args)
   {
      formatAndWriteLog(IStatus.INFO, message, args);
   }

   /**
    * @see org.slf4j.Logger#info(java.lang.String, java.lang.Throwable)
    */
   @Override
   public void info(String message, Throwable exception)
   {
      writeLog(IStatus.INFO, message, exception);
   }

   /**
    * @see org.slf4j.Logger#info(java.lang.String, java.lang.Object, java.lang.Object)
    */
   @Override
   public void info(String message, Object arg1, Object arg2)
   {
      formatAndWriteLog(IStatus.INFO, message, arg1, arg2);
   }

   /**
    * @see org.slf4j.Logger#isDebugEnabled()
    */
   @Override
   public boolean isDebugEnabled()
   {
      return true;
   }

   /**
    * @see org.slf4j.Logger#isErrorEnabled()
    */
   @Override
   public boolean isErrorEnabled()
   {
      return true;
   }

   /**
    * @see org.slf4j.Logger#isInfoEnabled()
    */
   @Override
   public boolean isInfoEnabled()
   {
      return true;
   }

   /**
    * @see org.slf4j.Logger#isTraceEnabled()
    */
   @Override
   public boolean isTraceEnabled()
   {
      return false;
   }

   /**
    * @see org.slf4j.Logger#isWarnEnabled()
    */
   @Override
   public boolean isWarnEnabled()
   {
      return true;
   }

   /**
    * @see org.slf4j.Logger#trace(java.lang.String)
    */
   @Override
   public void trace(String message)
   {
      writeLog(IStatus.INFO, message, null);
   }

   /**
    * @see org.slf4j.Logger#trace(java.lang.String, java.lang.Object)
    */
   @Override
   public void trace(String message, Object arg1)
   {
      formatAndWriteLog(IStatus.INFO, message, arg1, null);
   }

   /**
    * @see org.slf4j.Logger#trace(java.lang.String, java.lang.Object[])
    */
   @Override
   public void trace(String message, Object... args)
   {
      formatAndWriteLog(IStatus.INFO, message, args);
   }

   /**
    * @see org.slf4j.Logger#trace(java.lang.String, java.lang.Throwable)
    */
   @Override
   public void trace(String message, Throwable exception)
   {
      writeLog(IStatus.INFO, message, exception);
   }

   /**
    * @see org.slf4j.Logger#trace(java.lang.String, java.lang.Object, java.lang.Object)
    */
   @Override
   public void trace(String message, Object arg1, Object arg2)
   {
      formatAndWriteLog(IStatus.INFO, message, arg1, arg2);
   }

   /**
    * @see org.slf4j.Logger#warn(java.lang.String)
    */
   @Override
   public void warn(String message)
   {
      writeLog(IStatus.WARNING, message, null);
   }

   /**
    * @see org.slf4j.Logger#warn(java.lang.String, java.lang.Object)
    */
   @Override
   public void warn(String message, Object arg1)
   {
      formatAndWriteLog(IStatus.WARNING, message, arg1, null);
   }

   /**
    * @see org.slf4j.Logger#warn(java.lang.String, java.lang.Object[])
    */
   @Override
   public void warn(String message, Object... args)
   {
      formatAndWriteLog(IStatus.WARNING, message, args);
   }

   /**
    * @see org.slf4j.Logger#warn(java.lang.String, java.lang.Throwable)
    */
   @Override
   public void warn(String message, Throwable exception)
   {
      writeLog(IStatus.WARNING, message, exception);
   }

   /**
    * @see org.slf4j.Logger#warn(java.lang.String, java.lang.Object, java.lang.Object)
    */
   @Override
   public void warn(String message, Object arg1, Object arg2)
   {
      formatAndWriteLog(IStatus.WARNING, message, arg1, arg2);
   }
}
