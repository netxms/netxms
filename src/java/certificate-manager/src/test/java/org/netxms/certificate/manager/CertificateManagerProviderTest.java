package org.netxms.certificate.manager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.netxms.certificate.TestListener;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.notNullValue;
import static org.hamcrest.MatcherAssert.assertThat;

public class CertificateManagerProviderTest
{
   private CertificateManager manager;
   private final CertificateManagerRequestListener testListener = new TestListener();

   @Before
   public void setUp()
   {
      manager = CertificateManagerProvider.provideCertificateManager(testListener);
   }

   @After
   public void tearDown()
   {
      CertificateManagerProvider.dispose();
   }

   @Test
   public void testProvideCertificateManager_ProvidesManager()
   {
      assertThat(manager, notNullValue());
   }

   @Test
   public void testProvideCertificateManager_SameInstance()
   {
      CertificateManager mgr2 = CertificateManagerProvider.provideCertificateManager(testListener);

      assertThat(manager, equalTo(mgr2));
   }
}
