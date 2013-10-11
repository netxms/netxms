package org.netxms.certificate.loader;

import org.netxms.certificate.Certificate;

import java.util.List;

public interface KeyStoreLoader
{
   List<Certificate> retrieveCertificates();
}
