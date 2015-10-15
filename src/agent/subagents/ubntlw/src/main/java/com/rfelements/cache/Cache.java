package com.rfelements.cache;
import com.rfelements.model.Access;
/**
 * @author Pichanič Ján
 */
public interface Cache {

	boolean containsAccess(String key);

	Access getAccess(String key);

	Access putAccess(String key, Access access);

	Object putJsonObject(String key, Object object);

	Object getJsonObject(String key);

	boolean removeValue(String key);
}
