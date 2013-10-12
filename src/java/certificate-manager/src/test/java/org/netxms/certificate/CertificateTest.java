package org.netxms.certificate;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.netxms.certificate.manager.CertificateManager;
import org.netxms.certificate.manager.CertificateManagerProvider;

import java.security.PublicKey;
import java.security.cert.Certificate;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.notNullValue;
import static org.hamcrest.MatcherAssert.assertThat;

public class CertificateTest
{
   private CertificateManager manager;
   private Certificate cert;
   private final TestListener listener = new TestListener();

   @Before
   public void setUp()
   {
      manager = CertificateManagerProvider.provideCertificateManager(listener);
      cert = manager.getCerts().get(0);
   }

   @After
   public void tearDown()
   {
      CertificateManagerProvider.dispose();
   }

   @Test
   public void testCertificate_TestKeyPair()
   {
      PublicKey pk = cert.getPublicKey();
   }
}
