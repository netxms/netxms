/**
 * 
 */
package org.netxms.client.objecttools;

import org.netxms.base.NXCPMessage;

/**
 * Input field definition
 */
public class InputField
{
   private String name;
   private InputFieldType type;
   private String displayName;
   private int sequence;
   private InputFieldOptions options;
   
   /**
    * Create text input field with default settings
    * 
    * @param name field name
    */
   public InputField(String name)
   {
      this.name = name;
      this.type = InputFieldType.TEXT;
      this.displayName = name;
      this.sequence = 0;
      this.options = new InputFieldOptions();
   }

   /**
    * Create input field
    * 
    * @param name
    * @param type
    * @param displayName
    * @param config
    */
   public InputField(String name, InputFieldType type, String displayName, String config)
   {
      this.name = name;
      this.type = type;
      this.displayName = displayName;
      this.sequence = 0;
      this.options = InputFieldOptions.createFromXml(config);
   }
   
   /**
    * Copy constructor
    * 
    * @param src source object
    */
   public InputField(InputField src)
   {
      this.name = src.name;
      this.type = src.type;
      this.displayName = src.displayName;
      this.sequence = src.sequence;
      this.options = new InputFieldOptions(src.options);
   }

   /**
    * Create input field from NXCP message
    * 
    * @param msg
    * @param baseId
    */
   protected InputField(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      type = InputFieldType.getByValue(msg.getFieldAsInt32(baseId + 1));
      displayName = msg.getFieldAsString(baseId + 2);
      options = InputFieldOptions.createFromXml(msg.getFieldAsString(baseId + 3));
      sequence = msg.getFieldAsInt32(baseId + 4);
   }
   
   /**
    * Fill NXCP message with field data
    * 
    * @param msg
    * @param baseId
    */
   protected void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setField(baseId, name);
      msg.setFieldInt16(baseId + 1, type.getValue());
      msg.setField(baseId + 2, displayName);
      msg.setField(baseId + 3, options.createXml());
      msg.setFieldInt16(baseId + 4, sequence);
   }

   /**
    * @return the type
    */
   public InputFieldType getType()
   {
      return type;
   }

   /**
    * @param type the type to set
    */
   public void setType(InputFieldType type)
   {
      this.type = type;
   }

   /**
    * @return the displayName
    */
   public String getDisplayName()
   {
      return displayName;
   }

   /**
    * @param displayName the displayName to set
    */
   public void setDisplayName(String displayName)
   {
      this.displayName = displayName;
   }

   /**
    * @return the options
    */
   public InputFieldOptions getOptions()
   {
      return options;
   }

   /**
    * @param options the options to set
    */
   public void setOptions(InputFieldOptions options)
   {
      this.options = options;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @return the sequence
    */
   public int getSequence()
   {
      return sequence;
   }

   /**
    * @param sequence the sequence to set
    */
   public void setSequence(int sequence)
   {
      this.sequence = sequence;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "InputField [name=" + name + ", type=" + type + ", displayName=" + displayName + "]";
   }
}
