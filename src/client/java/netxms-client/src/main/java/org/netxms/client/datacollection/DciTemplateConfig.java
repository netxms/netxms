/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.client.datacollection;

import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Configuration for DCI template matching.
 * Contains pattern fields and matching options.
 */
@Root(name = "dciTemplate", strict = false)
public class DciTemplateConfig
{
   @Element(required = false)
   private String dciName = "";

   @Element(required = false)
   private String dciDescription = "";

   @Element(required = false)
   private String dciTag = "";

   @Element(required = false)
   private boolean regexMatch = false;

   @Element(required = false)
   private boolean multiMatch = false;

   /**
    * Default constructor
    */
   public DciTemplateConfig()
   {
   }

   /**
    * Copy constructor
    *
    * @param src source object to copy from
    */
   public DciTemplateConfig(DciTemplateConfig src)
   {
      this.dciName = src.dciName;
      this.dciDescription = src.dciDescription;
      this.dciTag = src.dciTag;
      this.regexMatch = src.regexMatch;
      this.multiMatch = src.multiMatch;
   }

   /**
    * Get DCI name pattern.
    *
    * @return the dciName
    */
   public String getDciName()
   {
      return dciName != null ? dciName : "";
   }

   /**
    * Set DCI name pattern.
    *
    * @param dciName the dciName to set
    */
   public void setDciName(String dciName)
   {
      this.dciName = dciName;
   }

   /**
    * Get DCI description pattern.
    *
    * @return the dciDescription
    */
   public String getDciDescription()
   {
      return dciDescription != null ? dciDescription : "";
   }

   /**
    * Set DCI description pattern.
    *
    * @param dciDescription the dciDescription to set
    */
   public void setDciDescription(String dciDescription)
   {
      this.dciDescription = dciDescription;
   }

   /**
    * Get DCI tag pattern.
    *
    * @return the dciTag
    */
   public String getDciTag()
   {
      return dciTag != null ? dciTag : "";
   }

   /**
    * Set DCI tag pattern.
    *
    * @param dciTag the dciTag to set
    */
   public void setDciTag(String dciTag)
   {
      this.dciTag = dciTag;
   }

   /**
    * Check if regular expression matching is enabled.
    *
    * @return true if regular expression matching is enabled
    */
   public boolean isRegexMatch()
   {
      return regexMatch;
   }

   /**
    * Enable or disable regular expression matching.
    *
    * @param regexMatch true to enable regular expression matching
    */
   public void setRegexMatch(boolean regexMatch)
   {
      this.regexMatch = regexMatch;
   }

   /**
    * Check if multiple match is enabled.
    *
    * @return true if multiple match is enabled
    */
   public boolean isMultiMatch()
   {
      return multiMatch;
   }

   /**
    * Enable or disable multiple match.
    *
    * @param multiMatch true to enable multiple match
    */
   public void setMultiMatch(boolean multiMatch)
   {
      this.multiMatch = multiMatch;
   }

   /**
    * Check if any template pattern is set.
    *
    * @return true if any pattern is set
    */
   public boolean hasPattern()
   {
      return !getDciName().isEmpty() || !getDciDescription().isEmpty() || !getDciTag().isEmpty();
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "DciTemplateConfig [dciName=" + dciName + ", dciDescription=" + dciDescription + ", dciTag=" + dciTag +
            ", regexMatch=" + regexMatch + ", multiMatch=" + multiMatch + "]";
   }
}
