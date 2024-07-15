/**
 * 
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.HashMap;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;

/**
 * Persistent storage test
 */
public class PStorageTest extends AbstractSessionTest
{
   @Test
	public void testGetSituations() throws Exception
	{
		NXCSession session = connectAndLogin();
		String key = "PStorageTestKey";
		String value = "PStorageTestValue";
		
		try
		{
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
		}
		finally
		{
         session.deletePersistentStorageValue(key);		   
		}
		
		session.disconnect();
	}
}
