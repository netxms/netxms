package org.netxms.certificate.manager;

import org.netxms.certificate.request.KeyStoreLocationRequestListener;
import org.netxms.certificate.request.KeyStorePasswordRequestListener;

public interface CertificateManagerProviderRequestListener
   extends KeyStoreLocationRequestListener, KeyStorePasswordRequestListener
{}
