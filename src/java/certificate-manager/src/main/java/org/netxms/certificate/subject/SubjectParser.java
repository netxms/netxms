package org.netxms.certificate.subject;

public class SubjectParser
{
   public static Subject parseSubject(String subjectString)
   {
      String commonName = "";
      String organization = "";
      String state = "";
      String country = "";

      String[] fields = subjectString.split("\\s*,\\s*");

      for(String field : fields)
      {
         String[] keyVal = field.split("\\s*=\\s*");
         if (keyVal.length != 2) continue;

         if (keyVal[0].equalsIgnoreCase("CN"))
         {
            commonName = keyVal[1];
         }
         else if (keyVal[0].equalsIgnoreCase("O"))
         {
            organization = keyVal[1];
         }
         else if (keyVal[0].equalsIgnoreCase("ST"))
         {
            state = keyVal[1];
         }
         else if (keyVal[0].equalsIgnoreCase("C"))
         {
            country = keyVal[1];
         }
      }

      return new Subject(commonName, organization, state, country);
   }
}
