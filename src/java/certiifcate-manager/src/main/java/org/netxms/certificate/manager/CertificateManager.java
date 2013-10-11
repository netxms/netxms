package org.netxms.certificate.manager;

import org.netxms.certificate.Certificate;

import java.util.List;

public class CertificateManager
{
   private final List<Certificate> certs;

   CertificateManager(List<Certificate> certs)
   {
      this.certs = certs;
   }

   public List<Certificate> getCerts()
   {
      return certs;
   }

   public boolean hasNoCertificates()
   {
      return certs.size() == 0;
   }
}
