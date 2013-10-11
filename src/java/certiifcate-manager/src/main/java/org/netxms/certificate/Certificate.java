package org.netxms.certificate;

import org.netxms.certificate.subject.Subject;

import java.security.PrivateKey;

public class Certificate
{
   private final Subject subject;
   private final PrivateKey privateKey;

   public Certificate(Subject subject, PrivateKey privateKey)
   {
      this.subject = subject;
      this.privateKey = privateKey;
   }

   public String getCommonName()
   {
      return subject.getCommonName();
   }

   public String getOrganization()
   {
      return subject.getOrganization();
   }

   public String getState()
   {
      return subject.getState();
   }

   public String getCountry()
   {
      return subject.getCountry();
   }
}
