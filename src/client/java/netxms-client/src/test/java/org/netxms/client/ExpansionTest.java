package org.netxms.client;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectContextBase;

public class ExpansionTest extends AbstractSessionTest
{
   public void testNodeAndAlarmExpanssion() throws Exception
   {
      final NXCSession session = connect();      
      session.syncObjects();
      
      final AbstractNode object = (AbstractNode)session.findObjectById(TestConstants.NODE_ID);
      final Map<Long, Alarm> alarms = session.getAlarms();
      final Map<String, String> inputValues = new HashMap<String, String>();
      final Alarm alarm = alarms.values().iterator().next();
      inputValues.put("Key1", "Value1");
      inputValues.put("Key2", "Value2");
      final List<String> stringsToExpand = new ArrayList<String>();
      
      stringsToExpand.add("%%%a%A");
      stringsToExpand.add("%g%I");
      stringsToExpand.add("%K%n%U");
      stringsToExpand.add("%(Key1)%(Key2)");
      final List<String> expandedStrings = session.substitureMacross(new ObjectContextBase(object, alarm), stringsToExpand, inputValues);
      
      assertEquals("%" + object.getPrimaryIP().getHostAddress().toString()+alarm.getMessage(), expandedStrings.get(0));
      assertEquals(object.getGuid().toString()+object.getObjectId(), expandedStrings.get(1));
      assertEquals(alarm.getKey()+object.getObjectName()+session.getUserName(), expandedStrings.get(2));

      session.disconnect();
   }
}
