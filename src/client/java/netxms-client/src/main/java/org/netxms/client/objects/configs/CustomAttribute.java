package org.netxms.client.objects.configs;

import org.netxms.base.NXCPMessage;

/**
 * Custom attribute class
 */
public class CustomAttribute
{
   public static final long INHERITABLE = 1;
   public static final long REDEFINED = 2;
   
   protected String value;
   protected long flags;
   protected long sourceObject; 
   
   /**
    * Constructor form NXCPMessage
    * 
    * @param msg
    * @param base
    */
   public CustomAttribute(final NXCPMessage msg, long base)
   {
      value = msg.getFieldAsString(base);
      flags = msg.getFieldAsInt32(base + 1);
      sourceObject = msg.getFieldAsInt32(base + 2);
   }
   
   /**
    * Copy contructor
    */
   public CustomAttribute(CustomAttribute attr)
   {
      value = attr.value;
      flags = attr.flags;
      sourceObject = attr.sourceObject;
   }

   /**
    * Constructor 
    * 
    * @param value value
    * @param flags flags
    * @param inheritedFrom 
    */
   public CustomAttribute(String value, long flags, long inheritedFrom)
   {
      this.value = value;
      this.flags = flags;
      this.sourceObject = inheritedFrom;
   }

   /**
    * @return the value
    */
   public String getValue()
   {
      return value;
   }

   /**
    * @return the flags
    */
   public long getFlags()
   {
      return flags;
   }

   /**
    * @return the inheritedFrom
    */
   public long getSourceObject()
   {
      return sourceObject;
   }

   /**
    * If attribute is redefined
    * @return true if redefined
    */
   public boolean isRedefined()
   {
      return (flags & REDEFINED) > 0;
   }   

   /**
    * If attribute is inherited
    * @return true if has source
    */
   public boolean isInherited()
   {
      return getSourceObject() != 0;
   } 

   /**
    * If attribute is inheritable
    * @return true if inheritable flag
    */
   public boolean isInheritable()
   {
      return (flags & INHERITABLE) > 0;
   } 
}
