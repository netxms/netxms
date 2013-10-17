package org.netxms.certificate.loader;

import org.netxms.certificate.loader.exception.KeyStoreLoaderException;

import java.security.KeyStore;

public interface KeyStoreLoader
{
   KeyStore loadKeyStore() throws KeyStoreLoaderException;

   void setKeyStoreRequestListener(KeyStoreRequestListener listener);
}
