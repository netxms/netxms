/**
 * 
 */
package org.netxms.base;

/**
 * Postal address representation
 */
public class PostalAddress
{
   public String country;
   public String city;
   public String streetAddress;
   public String postcode;
   
   public PostalAddress()
   {
      country = "";
      city = "";
      streetAddress = "";
      postcode = "";
   }
   
   public PostalAddress(PostalAddress src)
   {
      country = src.country;
      city = src.city;
      streetAddress = src.streetAddress;
      postcode = src.postcode;
   }
   
   public PostalAddress(NXCPMessage msg)
   {
      country = msg.getVariableAsString(NXCPCodes.VID_COUNTRY);
      city = msg.getVariableAsString(NXCPCodes.VID_CITY);
      streetAddress = msg.getVariableAsString(NXCPCodes.VID_STREET_ADDRESS);
      postcode = msg.getVariableAsString(NXCPCodes.VID_POSTCODE);
   }

   /**Fill NXCP message
    * 
    * @param msg
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setVariable(NXCPCodes.VID_COUNTRY, country);
      msg.setVariable(NXCPCodes.VID_CITY, city);
      msg.setVariable(NXCPCodes.VID_STREET_ADDRESS, streetAddress);
      msg.setVariable(NXCPCodes.VID_POSTCODE, postcode);
   }
}
