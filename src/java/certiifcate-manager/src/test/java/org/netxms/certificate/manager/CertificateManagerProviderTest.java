package org.netxms.certificate.manager;

import org.junit.After;
import org.junit.Test;
import org.netxms.certificate.TestListener;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.notNullValue;
import static org.hamcrest.MatcherAssert.assertThat;

public class CertificateManagerProviderTest
{
   private CertificateManager manager;
   private final CertificateManagerProviderRequestListener testListener = new TestListener();

   @After
   public void tearDown()
   {
      CertificateManagerProvider.dispose();
   }

   @Test
   public void testProvideCertificateManager_ProvidesManager()
   {
      manager = CertificateManagerProvider.provideCertificateManager(testListener);

      assertThat(manager, notNullValue());
   }

   @Test
   public void testProvideCertificateManager_SameInstance()
   {
      CertificateManager mgr1 = CertificateManagerProvider.provideCertificateManager(testListener);
      CertificateManager mgr2 = CertificateManagerProvider.provideCertificateManager(testListener);

      assertThat(mgr1, equalTo(mgr2));
   }
}
