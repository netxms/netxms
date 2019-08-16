package org.netxms.certificate.subject;

public class Subject
{
   private final String commonName;
   private final String organization;
   private final String state;
   private final String country;

   public Subject(String commonName, String organization, String state, String country)
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

   @Override
   public String toString()
   {
      return String.format("CN=%s, O=%s, ST=%s, C=%s", getCommonName(), getOrganization(), getState(), getCountry());
   }
}
