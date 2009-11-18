/**
 * 
 */
package org.netxms.client;

/**
 * @author victor
 *
 */
public class EventDatabaseSyncTest extends SessionTest
{
	public void testSyncEventDatabase() throws Exception
	{
		final NXCSession session = connect();
		
		assertNull(session.findEventTemplateByCode(1L));
		session.syncEventTemplates();
		assertNotNull(session.findEventTemplateByCode(1L));
		
		session.disconnect();
	}
}
