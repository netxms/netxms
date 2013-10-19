package org.netxms.certificate.manager;

import org.netxms.certificate.loader.KeyStoreLoader;
import org.netxms.certificate.loader.KeyStoreRequestListener;
import org.netxms.certificate.loader.exception.KeyStoreLoaderException;
import org.netxms.certificate.manager.exception.CertificateHasNoPrivateKeyException;
import org.netxms.certificate.manager.exception.CertificateNotInKeyStoreException;
import org.netxms.certificate.manager.exception.SignatureImpossibleException;
import org.netxms.certificate.manager.exception.SignatureVerificationImpossibleException;
import org.netxms.certificate.request.KeyStoreEntryPasswordRequestListener;

import java.security.*;
import java.security.cert.Certificate;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public class CertificateManager
{
   private KeyStore keyStore;
   private Certificate[] certs;
   private final KeyStoreLoader loader;
   private KeyStoreEntryPasswordRequestListener entryListener;

   CertificateManager(KeyStoreLoader loader)
   {
      this.loader = loader;
   }

   public void setEntryListener(KeyStoreEntryPasswordRequestListener entryListener)
   {
      this.entryListener = entryListener;
   }

   public void setKeyStoreRequestListener(KeyStoreRequestListener keyStoreListener)
   {
      loader.setKeyStoreRequestListener(keyStoreListener);
   }

   public Certificate[] getCerts()
   {
      return certs;
   }

   private void loadCerts()
   {
      try
      {
         certs = getCertsFromKeyStore();
      }
      catch(KeyStoreException e)
      {
         //e.printStackTrace();
         certs = new Certificate[0];
      }
      catch(UnrecoverableEntryException e)
      {
         //e.printStackTrace();
         certs = new Certificate[0];
      }
      catch(NoSuchAlgorithmException e)
      {
         //e.printStackTrace();
         certs = new Certificate[0];
      }
   }

   public boolean hasNoCertificates()
   {
      return (certs == null || certs.length == 0);
   }

   public byte[] sign(Certificate cert, byte[] challenge) throws SignatureImpossibleException
   {
      byte[] signedChallenge;

      try
      {
         PrivateKey pk = getPrivateKey(cert);
         Signature signature = Signature.getInstance("SHA1withRSA");
         signature.initSign(pk);
         signature.update(challenge);
         signedChallenge = signature.sign();
      }
      catch(Exception e)
      {
         throw new SignatureImpossibleException(e.getMessage());
      }

      return signedChallenge;
   }

   public Signature extractSignature(Certificate cert) throws SignatureImpossibleException
   {
      Signature signature;

      try
      {
         PrivateKey pk = getPrivateKey(cert);
         signature = Signature.getInstance("SHA1withRSA");
         signature.initSign(pk);
      }
      catch(Exception e)
      {
         throw new SignatureImpossibleException(e.getMessage());
      }

      return signature;
   }

   public boolean verify(Certificate cert, byte[] original, byte[] signed)
      throws SignatureVerificationImpossibleException
   {
      boolean verified;

      try
      {
         Signature verifier = Signature.getInstance("SHA1withRSA");
         verifier.initVerify(cert);
         verifier.update(original);

         verified = verifier.verify(signed);
      }
      catch(Exception e)
      {
         throw new SignatureVerificationImpossibleException();
      }

      return verified;
   }

   protected PrivateKey getPrivateKey(Certificate cert)
      throws KeyStoreException, CertificateNotInKeyStoreException, CertificateHasNoPrivateKeyException,
      NoSuchAlgorithmException, UnrecoverableEntryException
   {
      String alias = keyStore.getCertificateAlias(cert);

      if (alias == null) throw new CertificateNotInKeyStoreException();

      KeyStore.PrivateKeyEntry pkEntry;

      try
      {
         pkEntry = (KeyStore.PrivateKeyEntry) keyStore.getEntry(alias, null);
      }
      catch(UnrecoverableEntryException uee)
      {
         String password = getEntryPassword();
         KeyStore.ProtectionParameter parameter = new KeyStore.PasswordProtection(password.toCharArray());

         pkEntry = (KeyStore.PrivateKeyEntry) keyStore.getEntry(alias, parameter);
      }

      PrivateKey pk = pkEntry.getPrivateKey();

      if (pk == null) throw new CertificateHasNoPrivateKeyException();

      return pk;
   }

   protected String getEntryPassword()
   {
      if (entryListener == null) return "";

      return entryListener.keyStoreEntryPasswordRequested();
   }

   public void load() throws KeyStoreLoaderException
   {
      keyStore = loader.loadKeyStore();

      loadCerts();
   }

   protected Certificate[] getCertsFromKeyStore()
      throws KeyStoreException, UnrecoverableEntryException, NoSuchAlgorithmException
   {
      if (keyStore == null)
      {
         throw new KeyStoreException();
      }

      if (keyStore.size() == 0)
      {
         return new Certificate[0];
      }

      List<Certificate> certList = new ArrayList<Certificate>();
      Enumeration<String> aliases = keyStore.aliases();

      while(aliases.hasMoreElements())
      {
         String alias = aliases.nextElement();

         if (!keyStore.isKeyEntry(alias)) continue;

         Certificate cert = keyStore.getCertificate(alias);

         certList.add(cert);
      }

      Certificate[] certs = new Certificate[certList.size()];

      certList.toArray(certs);

      return certs;
   }
}
