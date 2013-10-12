package org.netxms.certificate.subject;

public class Subject
{
   private String commonName;
   private String organization;
   private String state;
   private String country;

   public Subject(
      String commonName, String organization, String state, String country)
   {

      this.commonName = commonName;
      this.organization = organization;
      this.state = state;
      this.country = country;
   }

   public String getCommonName()
   {
      return commonName;
   }

   public String getOrganization()
   {
      return organization;
   }

   public String getState()
   {
      return state;
   }

   public String getCountry()
   {
      return country;
   }
}
