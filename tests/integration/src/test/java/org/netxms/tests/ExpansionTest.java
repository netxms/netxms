/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
 * <p/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import java.text.SimpleDateFormat;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.EventInfo;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.client.objecttools.ObjectContextBase;
import org.netxms.utilities.TestHelper;

public class ExpansionTest extends AbstractSessionTest
{
   @Test
   public void testNodeAndAlarmExpanssion() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      session.syncEventTemplates();

      Node node = TestHelper.findManagementServer(session);

      final Map<String, String> inputValues = new HashMap<String, String>();
      inputValues.put("Key1", "Value1");
      inputValues.put("Key2", "Value2");

      NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());

      CustomAttribute customAttribute = new CustomAttribute("Value", 0, 0);
      Map<String, CustomAttribute> customAttributes = new HashMap<>();
      customAttributes.put("customAttributeName", customAttribute);
      md.setCustomAttributes(customAttributes);
      md.setAlias("alias test name");
      md.setSshLogin("testLogin");
      md.setSshPassword("testPassword");
      md.setSshPort(555);
      CustomAttribute customAttribute2 = new CustomAttribute("5", 0, 0);
      customAttributes.put("name1", customAttribute2);
      CustomAttribute customAttribute3 = new CustomAttribute("6", 0, 0);
      customAttributes.put("name1::testInstanceName", customAttribute3);
      session.modifyObject(md);

      session.updateObjectComments(node.getObjectId(), "test node comment");
      Long scriptId = session.modifyScript(0, "scriptForTest", "function main()\n" + "{\n" + "   return \"text from the script\";\n" + "}\n" + "\n" + "function test2()\n" + "{\n" +
            "   return \"text from the script 2\";\n" + "}\n" + "\n" + "function test3(param, param2)\n" + "{\n" + "   return \"func text\" .. $1 .. param2;\n" + "}");
      DataCollectionConfiguration dcc = session.openDataCollectionConfiguration(node.getObjectId());
      final DataCollectionItem dci = new DataCollectionItem(dcc, 0);
      dci.setOrigin(DataOrigin.INTERNAL);
      LocalDateTime currentTime = LocalDateTime.now();
      String dciDescriprion = "testCreateDCI " + currentTime;
      dci.setDescription(dciDescriprion);
      dci.setName("Status");
      Threshold threshold = new Threshold();
      threshold.setValue("100");
      dci.getThresholds().add(threshold);
      dci.setComments("Test \nmultiline\ncomments");
      dci.setInstanceName("testInstanceName");
      long dciID = dcc.modifyObject(dci);
      session.forceDCIPoll(node.getObjectId(), dciID);

      Alarm alarm = null;
      while(alarm == null)
      {
         final Map<Long, Alarm> alarms = session.getAlarms();
         for(Entry<Long, Alarm> entry : alarms.entrySet())
         {
            if (entry.getValue().getMessage().contains(dciDescriprion))
            {
               Thread.sleep(1000);
               alarm = entry.getValue();
            }
         }
      }

      EventInfo event = null;
      List<EventInfo> getAlarmEvents = session.getAlarmEvents(alarm.getId());
      if (!getAlarmEvents.isEmpty())
      {
         event = getAlarmEvents.get(0);
      }
      assertNotNull(event);

      final List<ObjectStatus> eventStatusText = new ArrayList<ObjectStatus>();
      eventStatusText.add(ObjectStatus.NORMAL);
      eventStatusText.add(ObjectStatus.WARNING);
      eventStatusText.add(ObjectStatus.MINOR);
      eventStatusText.add(ObjectStatus.MAJOR);
      eventStatusText.add(ObjectStatus.CRITICAL);
      eventStatusText.add(ObjectStatus.UNKNOWN);
      eventStatusText.add(ObjectStatus.UNMANAGED);
      eventStatusText.add(ObjectStatus.DISABLED);
      eventStatusText.add(ObjectStatus.TESTING);

      SimpleDateFormat outputFormatter = new SimpleDateFormat("dd.MMM.yyyy HH:mm:ss");

      final List<String> stringsToExpand = new ArrayList<String>();
      stringsToExpand.add("%%%a%A");// 0
      stringsToExpand.add("%g%I"); // 1
      stringsToExpand.add("%K%n%U");// 2
      stringsToExpand.add("%(Key1)%(Key2)");// 3
      stringsToExpand.add("%c%i");// 4
      stringsToExpand.add("%N%v");// 5
      stringsToExpand.add("%z%Z");// 6
      stringsToExpand.add("%{name}");// 7
      stringsToExpand.add("%{name:test_default_value}");// 8
      stringsToExpand.add("%{customAttributeName}");// 9
      stringsToExpand.add("%{customAttributeName:test_default_value}");// 10
      stringsToExpand.add("%U");// 11
      stringsToExpand.add("%Y");// 12
      stringsToExpand.add("%y");// 13
      stringsToExpand.add("%1%2%3");// 14
      stringsToExpand.add("%m"); // 15
      stringsToExpand.add("%S"); // 16
      stringsToExpand.add("%t"); // 17
      stringsToExpand.add("%T"); // 18
      stringsToExpand.add("%<dciName>"); // 19
      stringsToExpand.add("%[scriptForTest]"); // 20
      stringsToExpand.add("%[scriptForTest/test2]"); // 21
      stringsToExpand.add("%[scriptForTest/test3(param)]"); // 22
      stringsToExpand.add("%[scriptForTest/test3()]"); // 23
      stringsToExpand.add("%[scriptForTest/test3(param, param2)]"); // 24
      stringsToExpand.add("%[scriptForTest/test3(\"param, param2\")]"); // 25
      stringsToExpand.add("%L"); // 26
      stringsToExpand.add("%C"); // 27
      stringsToExpand.add("%d"); // 28
      stringsToExpand.add("%D"); // 29
      stringsToExpand.add("%{ssh.login}");// 30
      stringsToExpand.add("%{ssh.password}");// 31
      stringsToExpand.add("%{ssh.port}");// 32
      stringsToExpand.add("%{name1}");// 33

      final List<String> expandedStrings = session.substituteMacros(new ObjectContextBase(node, alarm), stringsToExpand, inputValues);
      node = TestHelper.findManagementServer(session); //get latest node version before check

      assertEquals("%" + node.getPrimaryIP().getHostAddress().toString() + alarm.getMessage(), expandedStrings.get(0));
      assertEquals(node.getGuid().toString() + node.getObjectId(), expandedStrings.get(1));
      assertEquals(alarm.getKey() + node.getObjectName() + session.getUserName(), expandedStrings.get(2));
      assertEquals(inputValues.get("Key1") + inputValues.get("Key2"), expandedStrings.get(3));
      assertEquals(alarm.getSourceEventCode() + String.format("0x%08X", (int)node.getObjectId()), expandedStrings.get(4));
      assertEquals(session.getEventName(alarm.getSourceEventCode()) + session.getServerVersion(), expandedStrings.get(5));
      assertEquals(node.getZoneId() + node.getZoneName(), expandedStrings.get(6));
      assertEquals("", expandedStrings.get(7));
      assertEquals("test_default_value", expandedStrings.get(8));
      assertEquals(node.getCustomAttributeValue("customAttributeName"), expandedStrings.get(9));
      assertEquals(node.getCustomAttributeValue("customAttributeName"), expandedStrings.get(10));
      assertEquals(TestConstants.SERVER_LOGIN, expandedStrings.get(11));
      assertEquals(Long.toString(alarm.getId()), expandedStrings.get(12));
      assertEquals(Long.toString(alarm.getState()), expandedStrings.get(13));
      assertEquals(dci.getName() + dci.getDescription() + threshold.getValue(), expandedStrings.get(14));
      assertEquals(event.getMessage(), expandedStrings.get(15));
      assertEquals(eventStatusText.get(event.getSeverity()).toString(), expandedStrings.get(16).toUpperCase());
      assertEquals(outputFormatter.format(event.getTimeStamp()), expandedStrings.get(17));
      assertEquals(Long.toString(event.getTimeStamp().getTime() / 1000), expandedStrings.get(18));
      assertEquals(dci.getName(), expandedStrings.get(19));
      assertEquals("text from the script", expandedStrings.get(20));
      assertEquals("text from the script 2", expandedStrings.get(21));
      assertEquals("func text" + "param", expandedStrings.get(22));
      assertEquals("func text", expandedStrings.get(23));
      assertEquals("func text" + "param" + "param2", expandedStrings.get(24));
      assertEquals("func text" + "param, param2", expandedStrings.get(25));
      assertEquals(md.getAlias(), expandedStrings.get(26));
      assertEquals(node.getComments(), expandedStrings.get(27));
      assertEquals(dci.getDescription(), expandedStrings.get(28));
      assertEquals(dci.getComments(), expandedStrings.get(29));
      assertEquals(node.getSshLogin(), expandedStrings.get(30));
      assertEquals(node.getSshPassword(), expandedStrings.get(31));
      assertEquals(Long.toString(node.getSshPort()), expandedStrings.get(32));
      assertEquals(node.getCustomAttributeValue("name1::testInstanceName"), expandedStrings.get(33));

      dcc.deleteObject(dciID);
      session.deleteScript(scriptId);

      final List<String> expandedStringsWithDeletedDCI = session.substituteMacros(new ObjectContextBase(node, alarm), stringsToExpand, inputValues);

      assertEquals("%" + node.getPrimaryIP().getHostAddress().toString() + "", expandedStringsWithDeletedDCI.get(0));
      assertEquals("" + node.getObjectName() + session.getUserName(), expandedStringsWithDeletedDCI.get(2));
      assertEquals("0" + String.format("0x%08X", (int)node.getObjectId()), expandedStringsWithDeletedDCI.get(4));
      assertEquals("" + session.getServerVersion(), expandedStringsWithDeletedDCI.get(5));
      assertEquals("", expandedStringsWithDeletedDCI.get(12));
      assertEquals("", expandedStringsWithDeletedDCI.get(13));
      assertEquals("", expandedStringsWithDeletedDCI.get(14));
      assertEquals("", expandedStringsWithDeletedDCI.get(15));
      assertEquals("", expandedStringsWithDeletedDCI.get(16));
      assertEquals("", expandedStringsWithDeletedDCI.get(19));
      assertEquals("", expandedStringsWithDeletedDCI.get(28));
      assertEquals("", expandedStringsWithDeletedDCI.get(29));

      final List<String> stringsToExpand2 = new ArrayList<String>();

      stringsToExpand2.add("%A"); // 0
      stringsToExpand2.add("%K"); // 1
      stringsToExpand2.add("%(Key1)%(Key2)"); // 2
      stringsToExpand2.add("%c"); // 3
      stringsToExpand2.add("%N"); // 4
      stringsToExpand2.add("%{name}"); // 5
      stringsToExpand2.add("%{customAttributeName}"); // 6
      stringsToExpand2.add("%{customAttributeName:test_default_value}"); // 7
      stringsToExpand2.add("%{customAttributeName"); // 8
      stringsToExpand2.add("%customAttributeName}"); // 9
      stringsToExpand2.add("%[name"); // 10
      stringsToExpand2.add("%name]"); // 11
      stringsToExpand2.add("%{name"); // 12
      stringsToExpand2.add("%name}"); // 13
      stringsToExpand2.add("%{name:default_value"); // 14
      stringsToExpand2.add("%name:default_value}"); // 15
      stringsToExpand2.add("%[name(123,\"A textual parameter\")]"); // 16
      stringsToExpand2.add("%[name(123,\"A textual parameter\")"); // 17
      stringsToExpand2.add("%[name(123,\"A textual parameter\"]"); // 18
      stringsToExpand2.add("%[name123,\"A textual parameter\")]"); // 19
      stringsToExpand2.add("%<name"); // 20
      stringsToExpand2.add("%<{format-specifier}name>"); // 21
      stringsToExpand2.add("%<{format-specifiername>"); // 22
      stringsToExpand2.add("%<format-specifiername}>"); // 23
      stringsToExpand2.add("%{format-specifier}name>"); // 24
      stringsToExpand2.add("%<{format-specifier}name"); // 25
      stringsToExpand2.add("%1112"); // 26
      stringsToExpand2.add("%-100"); // 27
      stringsToExpand2.add("%0"); // 28
      stringsToExpand2.add("%1.5"); // 29
      stringsToExpand2.add("%"); // 30
      stringsToExpand2.add("%{name1}");// 31

      final List<String> expandedStrings2 = session.substituteMacros(new ObjectContextBase(node, null), stringsToExpand2, null);

      assertEquals("", expandedStrings2.get(0));
      assertEquals("", expandedStrings2.get(1));
      assertEquals("" + "", expandedStrings2.get(2));
      assertEquals("0", expandedStrings2.get(3));
      assertEquals("", expandedStrings2.get(4));
      assertEquals("", expandedStrings2.get(5));
      assertEquals("Value", expandedStrings2.get(6));
      assertEquals("Value", expandedStrings2.get(7));
      assertEquals("", expandedStrings2.get(8));
      assertEquals("0ustomAttributeName}", expandedStrings2.get(9)); // expand macros %c + other text as a string
      assertEquals("", expandedStrings2.get(10));
      assertEquals(node.getObjectName() + "ame]", expandedStrings2.get(11)); // expand macros %n + other text as a string
      assertEquals("", expandedStrings2.get(12));
      assertEquals(node.getObjectName() + "ame}", expandedStrings2.get(13)); // expand macros %n + other text as a string
      assertEquals("", expandedStrings2.get(14));
      assertEquals(node.getObjectName() + "ame:default_value}", expandedStrings2.get(15)); // expand macros %n + other text as a string
      assertEquals("", expandedStrings2.get(16));
      assertEquals("", expandedStrings2.get(17));
      assertEquals("", expandedStrings2.get(18));
      assertEquals("", expandedStrings2.get(19));
      assertEquals("", expandedStrings2.get(20));
      assertEquals("", expandedStrings2.get(21));
      assertEquals("", expandedStrings2.get(22));
      assertEquals("", expandedStrings2.get(23));
      assertEquals("name>", expandedStrings2.get(24)); // expand macros %{name} + other text as a string
      assertEquals("", expandedStrings2.get(25));
      assertEquals("12", expandedStrings2.get(26));// expand macros %11 + other text as a string
      assertEquals("100", expandedStrings2.get(27)); // try to expand macros %- + other text as a string
      assertEquals("", expandedStrings2.get(28));
      assertEquals(".5", expandedStrings2.get(29)); // expand macros %1 + other text as a string
      assertEquals("", expandedStrings2.get(30));
      assertEquals(node.getCustomAttributeValue("name1"), expandedStrings2.get(31));

      dcc.close();
      session.disconnect();
   }

}
