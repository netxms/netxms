package org.netxms.certificate.request;

/**
 * Interface for key store password request listeners
 */
public interface KeyStoreEntryPasswordRequestListener
{
   /**
    * Called when key store entry password is needed
    *  
    * @return password for key store
    */
   String keyStoreEntryPasswordRequested();
}
