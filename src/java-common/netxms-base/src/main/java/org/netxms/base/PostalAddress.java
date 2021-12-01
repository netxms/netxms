/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.base;

/**
 * Postal address representation
 */
public class PostalAddress
{
   public String country;
   public String region;
   public String city;
   public String district;
   public String streetAddress;
   public String postcode;

   /**
    * Create empty postal address
    */
   public PostalAddress()
   {
      country = "";
      region = "";
      city = "";
      district = "";
      streetAddress = "";
      postcode = "";
   }

   /**
    * Create postal address with given data
    * 
    * @param country country code/name
    * @param region region name
    * @param city city name
    * @param district district name
    * @param streetAddress street address
    * @param postcode post code
    */
   public PostalAddress(String country, String region, String city, String district, String streetAddress, String postcode)
   {
      this.country = country;
      this.region = region;
      this.city = city;
      this.district = district;
      this.streetAddress = streetAddress;
      this.postcode = postcode;
   }

   /**
    * Create copy of given postal address object
    * 
    * @param src source object
    */
   public PostalAddress(PostalAddress src)
   {
      country = src.country;
      region = src.region;
      city = src.city;
      district = src.district;
      streetAddress = src.streetAddress;
      postcode = src.postcode;
   }

   /**
    * Create from NXCP message
    *  
    * @param msg NXCP message
    */
   public PostalAddress(NXCPMessage msg)
   {
      country = msg.getFieldAsString(NXCPCodes.VID_COUNTRY);
      region = msg.getFieldAsString(NXCPCodes.VID_REGION);
      city = msg.getFieldAsString(NXCPCodes.VID_CITY);
      district = msg.getFieldAsString(NXCPCodes.VID_DISTRICT);
      streetAddress = msg.getFieldAsString(NXCPCodes.VID_STREET_ADDRESS);
      postcode = msg.getFieldAsString(NXCPCodes.VID_POSTCODE);
   }

   /**
    * Fill NXCP message
    * 
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setField(NXCPCodes.VID_COUNTRY, country);
      msg.setField(NXCPCodes.VID_REGION, region);
      msg.setField(NXCPCodes.VID_CITY, city);
      msg.setField(NXCPCodes.VID_DISTRICT, district);
      msg.setField(NXCPCodes.VID_STREET_ADDRESS, streetAddress);
      msg.setField(NXCPCodes.VID_POSTCODE, postcode);
   }

   /**
    * Get address as one line with elements separated by commas. Empty elements will be omitted.
    * 
    * @return address as one line
    */
   public String getAddressLine()
   {
      StringBuilder sb = new StringBuilder();
      
      if (!streetAddress.isEmpty())
         sb.append(streetAddress);

      if (!district.isEmpty())
      {
         if (sb.length() > 0)
            sb.append(", ");
         sb.append(district);
      }

      if (!city.isEmpty())
      {
         if (sb.length() > 0)
            sb.append(", ");
         sb.append(city);
      }

      if (!region.isEmpty())
      {
         if (sb.length() > 0)
            sb.append(", ");
         sb.append(region);
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
    * Check if all elements are empty.
    * 
    * @return true if all elements are empty
    */
   public boolean isEmpty()
   {
      return country.isEmpty() && region.isEmpty() && city.isEmpty() && district.isEmpty() && streetAddress.isEmpty() && postcode.isEmpty();
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return getAddressLine();
   }
}
