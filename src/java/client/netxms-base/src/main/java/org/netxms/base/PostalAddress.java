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
   
   public PostalAddress(String country, String city, String streetAddress, String postcode)
   {
      this.country = country;
      this.city = city;
      this.streetAddress = streetAddress;
      this.postcode = postcode;
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
      country = msg.getFieldAsString(NXCPCodes.VID_COUNTRY);
      city = msg.getFieldAsString(NXCPCodes.VID_CITY);
      streetAddress = msg.getFieldAsString(NXCPCodes.VID_STREET_ADDRESS);
      postcode = msg.getFieldAsString(NXCPCodes.VID_POSTCODE);
   }

   /**Fill NXCP message
    * 
    * @param msg
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setField(NXCPCodes.VID_COUNTRY, country);
      msg.setField(NXCPCodes.VID_CITY, city);
      msg.setField(NXCPCodes.VID_STREET_ADDRESS, streetAddress);
      msg.setField(NXCPCodes.VID_POSTCODE, postcode);
   }
   
   /**
    * Get address as one line
    * 
    * @return
    */
   public String getAddressLine()
   {
      StringBuilder sb = new StringBuilder();
      
      if (!streetAddress.isEmpty())
         sb.append(streetAddress);
      
      if (!city.isEmpty())
      {
         if (sb.length() > 0)
            sb.append(", ");
         sb.append(city);
      }
      
      if (!postcode.isEmpty())
      {
         if (sb.length() > 0)
            sb.append(", ");
         sb.append(postcode);
      }
      
      if (!country.isEmpty())
      {
         if (sb.length() > 0)
            sb.append(", ");
         sb.append(country);
      }
      
      return sb.toString();
   }
   
   /**
    * @return
    */
   public boolean isEmpty()
   {
      return country.isEmpty() && city.isEmpty() && streetAddress.isEmpty() && postcode.isEmpty();
   }
}
