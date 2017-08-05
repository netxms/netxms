package com.rfelements.cache;

import java.util.concurrent.ConcurrentHashMap;
import org.netxms.bridge.LogLevel;
import org.netxms.bridge.Platform;
import com.rfelements.model.DeviceCredentials;

/**
 * @author Pichanič Ján
 */
public class CacheImpl implements Cache {

    private final static int DEBUG_LEVEL = 5;

    private final ConcurrentHashMap<String, Object> cache = new ConcurrentHashMap<>();

    private static CacheImpl instance;

    public static CacheImpl getInstance() {
        if (instance == null)
            instance = new CacheImpl();
        return instance;
    }

    private CacheImpl() {
       Platform.writeLog(LogLevel.INFO, "[" + this.getClass().getName() + "] Data provider initialized !");
    }

    @Override
    public boolean hasDeviceCredentials(String key) {
        if (cache.containsKey(key)) {
            StorableItem store = (StorableItem) cache.get(key);
            if (store.getDeviceCredentials() != null)
                return true;
        }
        return false;
    }

    @Override
    public DeviceCredentials getDeviceCredentials(String key) {
        StorableItem store = (StorableItem) cache.get(key);
        if (store == null)
            return null;
        return store.getDeviceCredentials();
    }

    @Override
    public DeviceCredentials putDeviceCredentials(String key, DeviceCredentials deviceCredentials) {
        if (!cache.containsKey(key)) {
            StorableItem store = new StorableItem();
            store.setDeviceCredentials(deviceCredentials);
            cache.put(key, store);
        } else {
            StorableItem store = (StorableItem) cache.get(key);
            store.setDeviceCredentials(deviceCredentials);
        }
        return deviceCredentials;
    }

    @Override
    public Object putJsonObject(String key, Object object) {
        // DEBUG OUTPUT
        if (object != null)
           Platform.writeDebugLog(DEBUG_LEVEL,
                    "[" + this.getClass().getName() + "] Storing object into cache , key : " + key + " JSON obj : " + object.getClass().getName());
        else {
           Platform.writeDebugLog(DEBUG_LEVEL, "[" + this.getClass().getName() + "] Storing object into cache , key : " + key + " JSON obj : null");
        }

        if (!cache.containsKey(key)) {
            StorableItem store = new StorableItem();
            store.setJsonObject(object);
            cache.put(key, store);
        } else {
            StorableItem store = (StorableItem) cache.get(key);
            store.setJsonObject(object);
        }
        return object;
    }

    @Override
    public Object getJsonObject(String key) {
        StorableItem store = (StorableItem) cache.get(key);
        if (store == null) {
           Platform.writeDebugLog(DEBUG_LEVEL, "[" + this.getClass().getName() + "] Getting object from cache, key : " + key + " JSON obj : null");
            return null;
        }
        if (store.getJsonObject() == null) {
           Platform.writeDebugLog(DEBUG_LEVEL, "[" + this.getClass().getName() + "] Getting object from cache, key : " + key + " JSON obj : null");
            return null;
        }
        Platform.writeDebugLog(DEBUG_LEVEL,
                "[" + this.getClass().getName() + "] Getting object from cache, key : " + key + " JSON obj : " + store.getJsonObject().getClass()
                        .getName());
        return store.getJsonObject();
    }

    @Override
    public boolean removeValue(String key) {
        if (cache.containsKey(key)) {
            cache.remove(key);
            return true;
        }
        return false;
    }
}

class StorableItem {

    private DeviceCredentials deviceCredentials;

    private Object jsonObject;

    public DeviceCredentials getDeviceCredentials() {
        return deviceCredentials;
    }

    public void setDeviceCredentials(DeviceCredentials deviceCredentials) {
        this.deviceCredentials = deviceCredentials;
    }

    public Object getJsonObject() {
        return jsonObject;
    }

    public void setJsonObject(Object jsonObject) {
        this.jsonObject = jsonObject;
    }
}
