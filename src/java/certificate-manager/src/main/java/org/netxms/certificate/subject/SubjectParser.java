package org.netxms.certificate.subject;

public class SubjectParser
{
   public static Subject parseSubject(String subjectString)
   {
      Subject subj;
      String commonName = "";
      String organization = "";
      String state = "";
      String country = "";

      String[] subjectFields = subjectString.split("\\s*,\\s*");
      for(String field : subjectFields)
      {
         String[] keyVal = field.split("\\s*=\\s*");

         if (keyVal[0].equals("CN"))
         {
            commonName = keyVal[1];
         }
         else if (keyVal[0].equals("O"))
         {
            organization = keyVal[1];
         }
         else if (keyVal[0].equals("ST"))
         {
            state = keyVal[1];
         }
         else if (keyVal[0].equals("C"))
         {
            country = keyVal[1];
         }
      }

      subj = new Subject(commonName, organization, state, country);

      return subj;
   }
}
