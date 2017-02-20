/**
 * 
 */
package org.netxms.websvc.json;

/**
 * Generic container for named response.
 */
public class ResponseContainer
{
   private String name;
   private Object value;

   /**
    * Create container.
    * 
    * @param name
    * @param value
    */
   public ResponseContainer(String name, Object value)
   {
      this.name = name;
      this.value = value;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the value
    */
   public Object getValue()
   {
      return value;
   }
   
   /**
    * Create JSON code from container.
    * 
    * @return JSON code
    */
   public String toJson()
   {
      StringBuilder sb = new StringBuilder("{ \"");
      sb.append(name);
      sb.append("\":");
      sb.append(JsonTools.jsonFromObject(value));
      sb.append(" }");
      return sb.toString();
   }
}
