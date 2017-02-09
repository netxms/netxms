package org.netxms.client;

import java.io.StringWriter;
import java.io.Writer;
import java.util.Set;
import java.util.regex.Pattern;
import org.netxms.base.Glob;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Class created to store menu filter
 */
@Root(name="objectMenuFilter")
public class ObjectMenuFilter
{
   @Element(required=false, name="toolOS")
   public String toolNodeOS;

   @Element(required=false)
   public String toolWorkstationOS;
   
   @Element(required=false)
   public String toolTemplate;
   
   @Element(required=false)
   public String snmpOid;

   @Element(required=false)
   public int flags;

   public static final int REQUIRES_SNMP                    = 0x00000001;
   public static final int REQUIRES_AGENT                   = 0x00000002;
   public static final int REQUIRES_OID_MATCH               = 0x00000004;
   public static final int REQUIRES_NODE_OS_MATCH           = 0x00000008;
   public static final int REQUIRES_TEMPLATE_MATCH          = 0x00000010;
   public static final int REQUIRES_WORKSTATION_OS_MATCH    = 0x00000020;
   
   /**
    * Create ObjectToolFilter object from XML document
    * 
    * @param xml XML document
    * @return deserialized object
    * @throws Exception if the object cannot be fully deserialized
    */
   public static ObjectMenuFilter createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(ObjectMenuFilter.class, xml);
   }
   
   /**
    * Create XML from configuration.
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      Serializer serializer = new Persister();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }
   
   public ObjectMenuFilter()
   {
      toolNodeOS = "";
      toolWorkstationOS = "";
      toolTemplate = "";
      snmpOid = "";
   }
   
   /**
    * Check if tool is applicable for given node.
    * 
    * @param node AbstractNode object
    * @return true if tool is applicable for given node
    */
   public boolean isApplicableForNode(AbstractNode node)
   {
      if (((flags & REQUIRES_SNMP) != 0) &&
          ((node.getFlags() & AbstractNode.NF_IS_SNMP) == 0))
         return false;  // Node does not support SNMP
      
      if (((flags & REQUIRES_AGENT) != 0) &&
             ((node.getFlags() & AbstractNode.NF_IS_NATIVE_AGENT) == 0))
            return false;  // Node does not have NetXMS agent
      
      if ((flags & REQUIRES_OID_MATCH) != 0)
      {
         if (!Glob.matchIgnoreCase(snmpOid, node.getSnmpOID()))
            return false;  // OID does not match
      }
      
      if ((flags & REQUIRES_NODE_OS_MATCH) != 0)
      {
	      boolean match = false;
	      String[] substrings = toolNodeOS.split(",");
	      for(int i = 0; i < substrings.length; i++)
	      {;
	         if (Pattern.matches(substrings[i], node.getPlatformName()))
	         {
	            match = true;
	         }
	      }
	      if (!match)
	         return false;  //Not correct type of OS
      }
	   
	   if ((flags & REQUIRES_WORKSTATION_OS_MATCH) != 0)
      {
         boolean match = false;
         String[] substrings = toolWorkstationOS.split(",");
         for(int i = 0; i < substrings.length; i++)
         {;
            if (Pattern.matches(substrings[i], System.getProperty("os.name")))
            {
               match = true;
            }
         }
         if (!match)
            return false;  //Not correct type of OS
      }
	    
	   if ((flags & REQUIRES_TEMPLATE_MATCH) != 0)
      {
         boolean match = false;
         String[] substrings = toolTemplate.split(",");
         Set<AbstractObject> parents = node.getAllParents(AbstractObject.OBJECT_TEMPLATE);
         for(AbstractObject parent : parents)
         {
            for(int i = 0; i < substrings.length; i++)
            {
               if (Pattern.matches(substrings[i], parent.getObjectName()))
               {
                  match = true;
               }
            }
         }
         if (!match)
            return false;  // Does not belong to those templates
      }
      
      return true;
   }
   
   public void setFilter(String filterText, int filterType)
   {
      switch(filterType)
      {
         case ObjectMenuFilter.REQUIRES_OID_MATCH:
            snmpOid = filterText;
            break;
         case ObjectMenuFilter.REQUIRES_NODE_OS_MATCH:
            toolNodeOS = filterText;
            break;
         case ObjectMenuFilter.REQUIRES_WORKSTATION_OS_MATCH:
            toolWorkstationOS = filterText;
            break;
         case ObjectMenuFilter.REQUIRES_TEMPLATE_MATCH:
            toolTemplate = filterText;
            break;
         default:
            //do nothing no sutch filter   
      }
   }
}
