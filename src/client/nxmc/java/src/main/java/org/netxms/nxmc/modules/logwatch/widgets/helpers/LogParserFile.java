/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2024 RadenSolutions
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
package org.netxms.nxmc.modules.logwatch.widgets.helpers;

import org.netxms.nxmc.modules.logwatch.widgets.LogParserFileEditor;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Text;

/**
 * File in log parser rule
 */
public class LogParserFile
{
   @Text(required = false)
   private String file = null;

   @Attribute(required = false)
   private String encoding = null;

   @Attribute(required = false)
   private String preallocated = null;

   @Attribute(required = false)
   private String snapshot = null;

   @Attribute(required = false)
   private String keepOpen = null;

   @Attribute(required = false)
   private String ignoreModificationTime = null;

   @Attribute(required = false)
   private String rescan = null;

   @Attribute(required = false)
   private String followSymlinks = null;

   @Attribute(required = false)
   private String removeEscapeSequences = null;

   private LogParserFileEditor editor;

   /**
    * @return
    */
   public String getFile()
   {
      return file != null ? file : "";
   }
   
   /**
    * @return
    */
   public String getEncoding()
   {
      return encoding;
   }
   
   /**
    * @param file
    */
   public void setFile(String file)
   {
      this.file = file;
   }
   
   /**
    * @param encoding
    */
   public void setEncoding(String encoding)
   {
      this.encoding = encoding;
   }

   /**
    * @return the preallocated
    */
   public boolean getPreallocated()
   {
      return LogParser.stringToBoolean(preallocated);
   }

   /**
    * @param preallocated the preallocated to set
    */
   public void setPreallocated(boolean preallocated)
   {
      this.preallocated = LogParser.booleanToString(preallocated);
   }

   /**
    * @return the snapshot
    */
   public boolean getSnapshot()
   {
      return LogParser.stringToBoolean(snapshot);
   }

   /**
    * @param snapshot the snapshot to set
    */
   public void setSnapshot(boolean snapshot)
   {
      this.snapshot = LogParser.booleanToString(snapshot);
   }

   /**
    * @return the keepOpen
    */
   public boolean getKeepOpen()
   {
      return keepOpen == null || keepOpen.isEmpty() ? true : LogParser.stringToBoolean(keepOpen);
   }

   /**
    * @param keepOpen the keepOpen to set
    */
   public void setKeepOpen(boolean keepOpen)
   {
      this.keepOpen = keepOpen ? null : "false";
   }

   /**
    * @return the ignoreModificationTime
    */
   public boolean getIgnoreModificationTime()
   {
      return LogParser.stringToBoolean(ignoreModificationTime);
   }

   /**
    * @param ignoreModificationTime the ignoreModificationTime to set
    */
   public void setIgnoreModificationTime(boolean ignoreModificationTime)
   {
      this.ignoreModificationTime = LogParser.booleanToString(ignoreModificationTime);
   }

   /**
    * Get value of "rescan" flag.
    * 
    * @return value of "rescan" flag
    */
   public boolean getRescan()
   {
      return LogParser.stringToBoolean(rescan);
   }

   /**
    * Set value of "rescan" flag.
    * 
    * @param value new value of "rescan" flag
    */
   public void setRescan(boolean value)
   {
      this.rescan = LogParser.booleanToString(value);
   }

   /**
    * Get value of "followSymlinks" flag.
    * 
    * @return value of "followSymlinks" flag
    */
   public boolean getFollowSymlinks()
   {
      return LogParser.stringToBoolean(followSymlinks);
   }

   /**
    * Set value of "followSymlinks" flag.
    * 
    * @param value new value of "followSymlinks" flag
    */
   public void setFollowSymlinks(boolean value)
   {
      this.followSymlinks = LogParser.booleanToString(value);
   }

   /**
    * Get value of "removeEscapeSequences" flag.
    * 
    * @return value of "removeEscapeSequences" flag
    */
   public boolean getRemoveEscapeSequences()
   {
      return LogParser.stringToBoolean(removeEscapeSequences);
   }

   /**
    * Set value of "removeEscapeSequences" flag.
    * 
    * @param value new value of "removeEscapeSequences" flag
    */
   public void setRemoveEscapeSequences(boolean value)
   {
      this.removeEscapeSequences = LogParser.booleanToString(value);
   }

   /**
    * @return the editor
    */
   public LogParserFileEditor getEditor()
   {
      return editor;
   }

   /**
    * @param editor the editor to set
    */
   public void setEditor(LogParserFileEditor editor)
   {
      this.editor = editor;
   }
}
