package org.netxms.certificate.loader;

import org.netxms.certificate.request.KeyStoreLocationRequestListener;
import org.netxms.certificate.request.KeyStorePasswordRequestListener;

public interface KeyStoreRequestListener extends KeyStoreLocationRequestListener, KeyStorePasswordRequestListener
{
}
