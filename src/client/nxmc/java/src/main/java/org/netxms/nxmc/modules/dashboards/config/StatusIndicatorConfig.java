/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.config;

import java.util.Map;
import java.util.Set;
import org.netxms.client.objects.interfaces.NodeItemPair;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;

/**
 * Configuration for status indicator widget
 */
@Root(name = "element", strict = false)
public class StatusIndicatorConfig extends DashboardElementConfig
{
   public static final int SHAPE_CIRCLE = 0;
   public static final int SHAPE_RECTANGLE = 1;
   public static final int SHAPE_ROUNDED_RECTANGLE = 2;

   public static final int LABEL_NONE = 0;
   public static final int LABEL_INSIDE = 1;
   public static final int LABEL_OUTSIDE = 2;

   public static final int ELEMENT_TYPE_OBJECT = 0;
   public static final int ELEMENT_TYPE_DCI = 1;
   public static final int ELEMENT_TYPE_DCI_TEMPLATE = 2;
   public static final int ELEMENT_TYPE_SCRIPT = 3;

   @ElementArray(required = false)
   private StatusIndicatorElementConfig[] elements = new StatusIndicatorElementConfig[0];

   @Element(required = false)
   private long objectId = 0; /* for compatibility with old versions */

   @Element(required = false)
   private String script = null;

   @Element(required = false)
   private long scriptContextObjectId = 0;

   @Element(required = false)
   private int numColumns = 1;

	@Element(required = false)
	private boolean fullColorRange = false;

   @Element(required = false)
   private int shape = SHAPE_CIRCLE;

   @Element(required = false)
   private int labelType = LABEL_NONE;

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#getObjects()
    */
	@Override
	public Set<Long> getObjects()
	{
		Set<Long> objects = super.getObjects();
		objects.add(objectId);
		return objects;
	}

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#remapObjects(java.util.Map)
    */
	@Override
	public void remapObjects(Map<Long, ObjectIdMatchingData> remapData)
	{
		super.remapObjects(remapData);
		ObjectIdMatchingData md = remapData.get(objectId);
		if (md != null)
			objectId = md.dstId;
	}

   /**
    * @return the fullColorRange
    */
   public boolean isFullColorRange()
   {
      return fullColorRange;
   }

   /**
    * @param fullColorRange the fullColorRange to set
    */
   public void setFullColorRange(boolean fullColorRange)
   {
      this.fullColorRange = fullColorRange;
   }

   /**
    * @return the elements
    */
   public StatusIndicatorElementConfig[] getElements()
   {
      if ((elements.length == 0) && (objectId != 0))
      {
         StatusIndicatorElementConfig element = new StatusIndicatorElementConfig();
         element.setType(ELEMENT_TYPE_OBJECT);
         element.setObjectId(objectId);
         elements = new StatusIndicatorElementConfig[] { element };
      }
      return elements;
   }

   /**
    * @param elements the elements to set
    */
   public void setElements(StatusIndicatorElementConfig[] elements)
   {
      this.elements = elements;
   }

   /**
    * @return the numColumns
    */
   public int getNumColumns()
   {
      return numColumns;
   }

   /**
    * @param numColumns the numColumns to set
    */
   public void setNumColumns(int numColumns)
   {
      this.numColumns = numColumns;
   }

   /**
    * @return the shape
    */
   public int getShape()
   {
      return shape;
   }

   /**
    * @param shape the shape to set
    */
   public void setShape(int shape)
   {
      this.shape = shape;
   }

   /**
    * @return the labelType
    */
   public int getLabelType()
   {
      return labelType;
   }

   /**
    * @param labelType the labelType to set
    */
   public void setLabelType(int labelType)
   {
      this.labelType = labelType;
   }

   /**
    * @return the script
    */
   public String getScript()
   {
      return script;
   }

   /**
    * @param script the script to set
    */
   public void setScript(String script)
   {
      this.script = script;
   }

   /**
    * @return the scriptContextObjectId
    */
   public long getScriptContextObjectId()
   {
      return scriptContextObjectId;
   }

   /**
    * @param scriptContextObjectId the scriptContextObjectId to set
    */
   public void setScriptContextObjectId(long scriptContextObjectId)
   {
      this.scriptContextObjectId = scriptContextObjectId;
   }

   /**
    * Represents single element for status indicator
    */
   @Root(name = "element", strict = false)
   public static class StatusIndicatorElementConfig implements NodeItemPair
   {
      @Element
      private int type;

      @Element(required = false)
      private String label;

      @Element(required = false)
      private long objectId;

      @Element(required = false)
      private long dciId;

      @Element(required = false)
      private String dciName;

      @Element(required = false)
      private String dciDescription;

      @Element(required = false)
      private String tag;

      /**
       * Default constructor
       */
      public StatusIndicatorElementConfig()
      {
         type = ELEMENT_TYPE_OBJECT;
         label = null;
         objectId = 0;
         dciId = 0;
         dciName = null;
         dciDescription = null;
         tag = null;
      }

      /**
       * Copy constructor.
       *
       * @param src source element
       */
      public StatusIndicatorElementConfig(StatusIndicatorElementConfig src)
      {
         type = src.type;
         label = src.label;
         objectId = src.objectId;
         dciId = src.dciId;
         dciName = src.dciName;
         dciDescription = src.dciDescription;
         tag = src.tag;
      }

      /**
       * @return the type
       */
      public int getType()
      {
         return type;
      }

      /**
       * @param type the type to set
       */
      public void setType(int type)
      {
         this.type = type;
      }

      /**
       * @return the label
       */
      public String getLabel()
      {
         if ((label != null) && !label.isEmpty())
            return label;
         if (type == ELEMENT_TYPE_SCRIPT)
            return tag;
         if (type == ELEMENT_TYPE_DCI_TEMPLATE)
         {
            if ((dciDescription != null) && !dciDescription.isEmpty())
               return dciDescription;
            if ((dciName != null) && !dciName.isEmpty())
               return dciName;
         }
         return "";
      }

      /**
       * @param label the label to set
       */
      public void setLabel(String label)
      {
         this.label = label;
      }

      /**
       * @return the objectId
       */
      public long getObjectId()
      {
         return objectId;
      }

      /**
       * @param objectId the objectId to set
       */
      public void setObjectId(long objectId)
      {
         this.objectId = objectId;
      }

      /**
       * @see org.netxms.client.objects.interfaces.NodeItemPair#getDciId()
       */
      @Override
      public long getDciId()
      {
         return dciId;
      }

      /**
       * @param dciId the dciId to set
       */
      public void setDciId(long dciId)
      {
         this.dciId = dciId;
      }

      /**
       * @return the dciName
       */
      public String getDciName()
      {
         return (dciName != null) ? dciName : "";
      }

      /**
       * @param dciName the dciName to set
       */
      public void setDciName(String dciName)
      {
         this.dciName = dciName;
      }

      /**
       * @return the dciDescription
       */
      public String getDciDescription()
      {
         return (dciDescription != null) ? dciDescription : "";
      }

      /**
       * @param dciDescription the dciDescription to set
       */
      public void setDciDescription(String dciDescription)
      {
         this.dciDescription = dciDescription;
      }

      /**
       * Get element's tag.
       *
       * @return element's tag
       */
      public String getTag()
      {
         return tag;
      }

      /**
       * Set element tag.
       *
       * @param tag new tag
       */
      public void setTag(String tag)
      {
         this.tag = tag;
      }

      /**
       * @see org.netxms.client.objects.interfaces.NodeItemPair#getNodeId()
       */
      @Override
      public long getNodeId()
      {
         return objectId;
      }
   }
}
