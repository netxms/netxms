package org.netxms.certificate.loader;

import org.netxms.certificate.Certificate;
import org.netxms.certificate.manager.CertificateManagerProviderRequestListener;
import org.netxms.certificate.subject.Subject;
import org.netxms.certificate.subject.SubjectParser;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.*;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.Enumeration;

public class PKCS12KeyStoreLoader implements KeyStoreLoader
{
   private final CertificateManagerProviderRequestListener listener;

   public PKCS12KeyStoreLoader(
      CertificateManagerProviderRequestListener listener)
   {

      this.listener = listener;
   }

   @Override
   public Certificate[] retrieveCertificates()
   {
      return new Certificate[0];  //To change body of implemented methods use File | Settings | File Templates.
   }

   protected KeyStore getPkcsKeyStore()
      throws KeyStoreException, CertificateException, NoSuchAlgorithmException, IOException
   {

      KeyStore ks = KeyStore.getInstance("PKCS12");
      String ksLocation = listener.keyStoreLocationRequested();
      String keyStorePass = listener.keyStorePasswordRequested();

      FileInputStream fis;
      try
      {
         fis = new FileInputStream(ksLocation);
         try
         {
            ks.load(fis, keyStorePass.toCharArray());
         }
         finally
         {
            fis.close();
         }
      }
      catch(FileNotFoundException fnfe)
      {
         System.out.println(fnfe.getMessage());
         fnfe.printStackTrace();
      }

      return ks;
   }

   protected Certificate[] getCertsFromKeyStore(KeyStore ks)
      throws KeyStoreException, UnrecoverableEntryException, NoSuchAlgorithmException
   {

      int numOfCerts = ks.size();
      final Certificate[] certs = new Certificate[numOfCerts];

      if (numOfCerts == 0)
      {
         return certs;
      }

      Enumeration<String> aliases = ks.aliases();
      KeyStore.ProtectionParameter protParam = new KeyStore.PasswordProtection("helloo".toCharArray());

      for(int i = 0; i < numOfCerts; i++)
      {
         String alias = aliases.nextElement();
         X509Certificate x509Cert = (X509Certificate) ks.getCertificate(alias);
         Principal subjectField = x509Cert.getSubjectDN();

         Subject subject = SubjectParser.parseSubject(subjectField.toString());
         KeyStore.PrivateKeyEntry pkEntry = (KeyStore.PrivateKeyEntry) ks.getEntry(alias, protParam);
         PrivateKey pk = pkEntry.getPrivateKey();

         certs[i] = new Certificate(subject, pk);
      }

      return certs;
   }
}
