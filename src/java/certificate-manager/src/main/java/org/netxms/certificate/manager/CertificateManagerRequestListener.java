package org.netxms.certificate.manager;

import org.netxms.certificate.loader.KeyStoreRequestListener;
import org.netxms.certificate.request.KeyStoreEntryPasswordRequestListener;


public interface CertificateManagerRequestListener extends KeyStoreRequestListener, KeyStoreEntryPasswordRequestListener
{}
