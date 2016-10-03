package com.rfelements.cache;

import com.rfelements.model.DeviceCredentials;

/**
 * @author Pichanič Ján
 */
public interface Cache {

    boolean hasDeviceCredentials(String key);

    DeviceCredentials getDeviceCredentials(String key);

    DeviceCredentials putDeviceCredentials(String key, DeviceCredentials deviceCredentials);

    Object putJsonObject(String key, Object object);

    Object getJsonObject(String key);

    boolean removeValue(String key);
}
