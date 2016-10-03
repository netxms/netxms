package com.rfelements.cache;

import com.rfelements.model.DeviceCredentials;

/**
 * @author Pichanič Ján
 */
public interface Cache {

    boolean containsAccess(String key);

    DeviceCredentials getAccess(String key);

    DeviceCredentials putAccess(String key, DeviceCredentials deviceCredentials);

    Object putJsonObject(String key, Object object);

    Object getJsonObject(String key);

    boolean removeValue(String key);
}
