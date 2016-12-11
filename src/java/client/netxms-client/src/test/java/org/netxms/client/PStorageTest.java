/**
 * 
 */
package org.netxms.client;

import java.util.HashMap;
import java.util.List;
import org.eclipse.core.runtime.Assert;
import org.netxms.client.NXCSession;
import org.netxms.client.situations.Situation;
import org.netxms.client.situations.SituationInstance;

/**
 * Persistent storage test
 */
public class PStorageTest extends AbstractSessionTest
{
	public void testGetSituations() throws Exception
	{
		NXCSession session = connect();
		String key = "PStorageTestKey";
		Stying value = "PStorageTestValue"
		
		//check that there is no entry with "PStorageTestKey" key
		HashMap<String, String> map = session.getPersistentStorageList();		
		assertFalse(map.containsKey(key));
		
		//add enry with "PStorageTestKey" key
		session.setPersistentStorageValue(key, value);	
		
		//check that entry with "PStorageTestKey" key was added and contains "PStorageTestValue" value
      map = session.getPersistentStorageList();    
      assertTrue(map.containsKey(key));
      assertTrue(map.get(key).equals(value));
     
      //delete entry with "PStorageTestKey" key
		session.deletePersistentStorageValue(key);
      
		//check that there is no entry with "PStorageTestKey" key
      map = session.getPersistentStorageList();    
      assertFalse(map.containsKey(key));
		
		session.disconnect();
	}
}
