package org.netxms.certificate.loader;

import org.netxms.certificate.Certificate;

public interface KeyStoreLoader
{
   Certificate[] retrieveCertificates();
}
