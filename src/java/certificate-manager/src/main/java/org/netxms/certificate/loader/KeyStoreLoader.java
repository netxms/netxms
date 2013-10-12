package org.netxms.certificate.loader;

import java.security.cert.Certificate;
import java.util.List;

public interface KeyStoreLoader
{
   List<Certificate> retrieveCertificates();
}
