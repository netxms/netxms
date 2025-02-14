package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import org.apache.commons.io.IOUtils;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.maps.MapType;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.utilities.TestHelper;

public class NetworkMapScript extends AbstractSessionTest
{
   final static String TEST_DCI_DESCRIPTION = "NetworkMapStylingScript";
   final static String TEST_MAP_NAME = "StylingScriptTest";
   final static String TEST_NODE = "TestNetworkMap";

   @Test
   public void testNetworkMapStylingScript() throws Exception
   {
      NXCSession session = connectAndLogin();
      
      //Delete old map 
      AbstractObject oldMap = session.findObjectByName(TEST_MAP_NAME);
      if (oldMap != null)
         session.deleteObject(oldMap.getObjectId());
      
      AbstractObject managementNode = TestHelper.findManagementServer(session);
      assertNotNull(managementNode);       
      TestHelper.findOrCreateDCI(session, managementNode, TEST_DCI_DESCRIPTION);
      
      TestHelper.checkNodeCreatedOrCreate(session, TEST_NODE);
      AbstractObject secondObject = session.findObjectByName(TEST_NODE);      
      TestHelper.findOrCreateDCI(session, secondObject, TEST_DCI_DESCRIPTION);

      AbstractObject netMap = session.findObjectByName(TEST_MAP_NAME);
      if (netMap != null)
      {
         session.deleteObject(netMap.getObjectId());
      }
      
      NXCObjectCreationData cd = new NXCObjectCreationData(GenericObject.OBJECT_NETWORKMAP, TEST_MAP_NAME, GenericObject.NETWORKMAPROOT);
      cd.setMapType(MapType.CUSTOM);
      NetworkMapPage map = ((NetworkMap)session.createObjectSync(cd)).createMapPage();

      long elementId1 = map.createElementId();
      map.addElement(new NetworkMapObject(elementId1, secondObject.getObjectId()));
      long elementId2 = map.createElementId();
      map.addElement(new NetworkMapObject(elementId2, managementNode.getObjectId()));
      map.addLink(new NetworkMapLink(map.createLinkId(), 0, elementId1, elementId2));
      
      final NXCObjectModificationData md = new NXCObjectModificationData(map.getMapObjectId());
      md.setMapContent(map.getElements(), map.getLinks());
      String script = IOUtils.toString(this.getClass().getResourceAsStream("/networkMapStylingScript.nxsl"), "UTF-8");
      md.setLinkStylingScript(script);
      session.modifyObject(md);

      session.pollObject(map.getMapObjectId(), ObjectPollType.MAP_UPDATE, null);
      Thread.sleep(3000);
      
      netMap = session.findObjectById(map.getMapObjectId());
      
      assertEquals(netMap.getCustomAttribute("NetworkMapScript").getValue(), "1");
   }
}
