package org.netxms.certificate.manager;

import java.security.cert.Certificate;
import java.util.ArrayList;
import java.util.List;

public class CertificateManager
{
   private final List<CertificateEntry> entries;

   CertificateManager(List<CertificateEntry> entries)
   {
      this.entries = entries;
   }

   public List<Certificate> getCerts()
   {
      final List<Certificate> certs = new ArrayList<Certificate>(entries.size());

      for(CertificateEntry entry : entries)
      {
         certs.add(entry.getCert());
      }

      return certs;
   }

   public boolean hasNoCertificates()
   {
      return entries.size() == 0;
   }
}
