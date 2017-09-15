package org.netxms.certificate.manager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.netxms.certificate.TestListener;

import java.security.*;
import java.security.cert.Certificate;
import java.security.cert.CertificateEncodingException;
import java.security.cert.CertificateException;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class CertificateManagerTest
{
   private CertificateManager manager;
   private final TestListener listener = new TestListener();

   @Before
   public void setUp() throws Exception
   {
      manager = CertificateManagerProvider.provideCertificateManager();
      manager.setKeyStoreRequestListener(listener);
      manager.setPasswordRequestListener(listener);
      manager.load();
   }

   @After
   public void tearDown() throws Exception
   {
      CertificateManagerProvider.dispose();
   }

   @Test
   public void testHasNoCertificates()
   {
      boolean hasNoCertificates = manager.hasNoCertificates();

      assertFalse(hasNoCertificates);
   }

   @Test
   public void testGetCerts_HasExpectedNumberOfCerts()
   {
      Certificate[] certs = manager.getCerts();
      int numCerts = certs.length;

      assertThat(numCerts, equalTo(1));
   }

   @Test
   public void testSign() throws Exception
   {
      Certificate cert = manager.getCerts()[0];
      byte[] challenge = "Sign this!".getBytes();
      byte[] signed = manager.sign(cert, challenge);

      Signature verifier = Signature.getInstance("SHA1withRSA");
      verifier.initVerify(cert);
      verifier.update(challenge);

      assertTrue(verifier.verify(signed));
   }

   @Test
   public void testVerify_VerifyLegit() throws Exception
   {
      byte[] challenge = "Sign this!".getBytes();

      KeyPairGenerator keyGen = KeyPairGenerator.getInstance("RSA");
      SecureRandom rnd = SecureRandom.getInstance("SHA1PRNG");
      keyGen.initialize(1024, rnd);
      KeyPair pair = keyGen.generateKeyPair();
      final PrivateKey pk = pair.getPrivate();
      final PublicKey pub = pair.getPublic();

      Certificate dummy = new Certificate("")
      {
         @Override
         public byte[] getEncoded() throws CertificateEncodingException
         {
            return new byte[0];
         }

         @Override
         public void verify(PublicKey publicKey)
            throws CertificateException, NoSuchAlgorithmException, InvalidKeyException, NoSuchProviderException,
            SignatureException
         {}

         @Override
         public void verify(PublicKey publicKey, String s)
            throws CertificateException, NoSuchAlgorithmException, InvalidKeyException, NoSuchProviderException,
            SignatureException
         {}

         @Override
         public String toString()
         {
            return "";
         }

         @Override
         public PublicKey getPublicKey()
         {
            return pub;
         }
      };

      Signature signature = Signature.getInstance("SHA1withRSA");
      signature.initSign(pk);
      signature.update(challenge);
      byte[] signed = signature.sign();

      assertTrue(manager.verify(dummy, challenge, signed));
   }

   @Test
   public void testVerify_VerifyBastard() throws Exception
   {
      byte[] challenge = "Sign this!".getBytes();
      Certificate cert = manager.getCerts()[0];

      KeyPairGenerator keyGen = KeyPairGenerator.getInstance("RSA");
      SecureRandom rnd = SecureRandom.getInstance("SHA1PRNG");
      keyGen.initialize(1024, rnd);
      KeyPair pair = keyGen.generateKeyPair();
      final PrivateKey pk = pair.getPrivate();

      Signature signature = Signature.getInstance("SHA1withRSA");
      signature.initSign(pk);
      signature.update(challenge);
      byte[] signed = signature.sign();

      assertFalse(manager.verify(cert, challenge, signed));
   }
}
