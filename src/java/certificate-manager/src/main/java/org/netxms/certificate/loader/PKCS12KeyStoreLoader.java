package org.netxms.certificate.loader;

import org.netxms.certificate.manager.CertificateManagerProviderRequestListener;
import org.netxms.certificate.subject.Subject;
import org.netxms.certificate.subject.SubjectParser;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.*;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public class PKCS12KeyStoreLoader implements KeyStoreLoader
{
   private final CertificateManagerProviderRequestListener listener;
   private String keyStorePassword;

   public PKCS12KeyStoreLoader(
      CertificateManagerProviderRequestListener listener)
   {
      this.listener = listener;
   }

   @Override
   public List<Certificate> retrieveCertificates()
   {
      List<Certificate> certs;

      try
      {
         KeyStore ks = loadKeyStore();
         certs = getCertsFromKeyStore(ks);
      }
      catch(KeyStoreException e)
      {
         e.printStackTrace();
         certs = new ArrayList<Certificate>(0);
      }
      catch(CertificateException e)
      {
         e.printStackTrace();
         certs = new ArrayList<Certificate>(0);
      }
      catch(NoSuchAlgorithmException e)
      {
         e.printStackTrace();
         certs = new ArrayList<Certificate>(0);
      }
      catch(IOException e)
      {
         e.printStackTrace();
         certs = new ArrayList<Certificate>(0);
      }
      catch(UnrecoverableEntryException e)
      {
         e.printStackTrace();
         certs = new ArrayList<Certificate>(0);
      }

      return certs;
   }

   protected KeyStore loadKeyStore()
      throws KeyStoreException, CertificateException, NoSuchAlgorithmException, IOException
   {
      KeyStore ks = KeyStore.getInstance("PKCS12");
      String ksLocation = listener.keyStoreLocationRequested();
      this.keyStorePassword = listener.keyStorePasswordRequested();

      FileInputStream fis;
      try
      {
         fis = new FileInputStream(ksLocation);
         try
         {
            ks.load(fis, this.keyStorePassword.toCharArray());
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

   protected List<Certificate> getCertsFromKeyStore(KeyStore ks)
      throws KeyStoreException, UnrecoverableEntryException, NoSuchAlgorithmException
   {
      if (ks.size() == 0)
      {
         return new ArrayList<Certificate>(0);
      }

      List<Certificate> certs = new ArrayList<Certificate>();
      Enumeration<String> aliases = ks.aliases();
      KeyStore.ProtectionParameter protParam = new KeyStore.PasswordProtection(this.keyStorePassword.toCharArray());

      while(aliases.hasMoreElements())
      {
         String alias = aliases.nextElement();
         //X509Certificate x509Cert = (X509Certificate) ks.getCertificate(alias);
         //Principal subjectField = x509Cert.getSubjectDN();

         //Subject subject = SubjectParser.parseSubject(subjectField.toString());
         Certificate cert = ks.getCertificate(alias);
         KeyStore.PrivateKeyEntry pkEntry = (KeyStore.PrivateKeyEntry) ks.getEntry(alias, protParam);
         PrivateKey pk = pkEntry.getPrivateKey();

         certs.add(cert);
      }

      return certs;
   }
}
