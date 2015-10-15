package com.rfelements.cache;
import com.rfelements.model.Access;
import org.netxms.agent.SubAgent;

import java.util.concurrent.ConcurrentHashMap;
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
		SubAgent.writeLog(SubAgent.LogLevel.INFO, "[" + this.getClass().getName() + "] Data provider initialized !");
	}

	@Override
	public boolean containsAccess(String key) {
		if (cache.containsKey(key)) {
			StorableItem store = (StorableItem) cache.get(key);
			if (store.getAccess() != null)
				return true;
		}
		return false;
	}

	@Override
	public Access getAccess(String key) {
		StorableItem store = (StorableItem) cache.get(key);
		if (store == null)
			return null;
		return store.getAccess();
	}

	@Override
	public Access putAccess(String key, Access access) {
		if (!cache.containsKey(key)) {
			StorableItem store = new StorableItem();
			store.setAccess(access);
			cache.put(key, store);
		} else {
			StorableItem store = (StorableItem) cache.get(key);
			store.setAccess(access);
		}
		return access;
	}

	@Override
	public Object putJsonObject(String key, Object object) {
		// DEBUG OUTPUT
		if (object != null)
			SubAgent.writeDebugLog(DEBUG_LEVEL,
					"[" + this.getClass().getName() + "] Storing object into cache , key : " + key + " JSON obj : " + object.getClass().getName());
		else {
			SubAgent.writeDebugLog(DEBUG_LEVEL, "[" + this.getClass().getName() + "] Storing object into cache , key : " + key + " JSON obj : null");
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
			SubAgent.writeDebugLog(DEBUG_LEVEL, "[" + this.getClass().getName() + "] Getting object from cache, key : " + key + " JSON obj : null");
			return null;
		}
		if (store.getJsonObject() == null) {
			SubAgent.writeDebugLog(DEBUG_LEVEL, "[" + this.getClass().getName() + "] Getting object from cache, key : " + key + " JSON obj : null");
			return null;
		}
		SubAgent.writeDebugLog(DEBUG_LEVEL,
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

	private Access access;

	private Object jsonObject;

	public Access getAccess() {
		return access;
	}

	public void setAccess(Access access) {
		this.access = access;
	}

	public Object getJsonObject() {
		return jsonObject;
	}

	public void setJsonObject(Object jsonObject) {
		this.jsonObject = jsonObject;
	}
}
