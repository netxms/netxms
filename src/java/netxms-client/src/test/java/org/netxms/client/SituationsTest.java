/**
 * 
 */
package org.netxms.client;

import java.util.List;
import org.netxms.client.situations.Situation;
import org.netxms.client.situations.SituationInstance;

/**
 * Situations test
 */
public class SituationsTest extends SessionTest
{
	public void testGetSituations() throws Exception
	{
		NXCSession session = connect();
		
		List<Situation> situations = session.getSituations();
		for(Situation s : situations)
		{
			System.out.println(s.getName() + " [" + s.getId() + "]");
			for(SituationInstance si : s.getInstances())
			{
				System.out.println("   " + si.getName() + " (" + si.getAttributeCount() + ")");
			}
		}
		
		session.disconnect();
	}
}
