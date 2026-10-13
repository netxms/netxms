/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.netxms.utilities.TestHelper.assertRcc;
import java.util.AbstractMap;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.Test;
import org.netxms.client.HardwareComponent;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.PhysicalComponent;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.constants.AMDataType;
import org.netxms.client.constants.AMSystemType;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;

/**
 * Tests for assets linked to node components (issue #3594): creation with component link, manual link/unlink by serial number,
 * validation of link requests, and automatic linking by configuration poll.
 */
public class ComponentAssetTest extends AbstractSessionTest
{
   private static final String TEST_ASSET_NAME = "ComponentAssetTest asset";
   private static final String TEST_ATTRIBUTE_NAME = "cat_serial";

   /**
    * Find serial number of any inventoried component of given node (agent hardware inventory first, then ENTITY-MIB tree).
    */
   private static String findComponentSerial(NXCSession session, Node node) throws Exception
   {
      for(HardwareComponent c : session.getNodeHardwareComponents(node.getObjectId()))
      {
         if (!c.getSerialNumber().isEmpty() && !c.getSerialNumber().equals(node.getHardwareSerialNumber()))
            return c.getSerialNumber();
      }
      try
      {
         return findComponentSerial(session.getNodePhysicalComponents(node.getObjectId()), node.getHardwareSerialNumber());
      }
      catch(NXCException e)
      {
         if (e.getErrorCode() != RCC.NO_COMPONENT_DATA)
            throw e;
         return null;
      }
   }

   /**
    * Find serial number in ENTITY-MIB component subtree, excluding node's own serial number.
    */
   private static String findComponentSerial(PhysicalComponent component, String nodeSerial)
   {
      if ((component.getSerialNumber() != null) && !component.getSerialNumber().isEmpty() && !component.getSerialNumber().equals(nodeSerial))
         return component.getSerialNumber();
      for(PhysicalComponent c : component.getSubcomponents())
      {
         String serial = findComponentSerial(c, nodeSerial);
         if (serial != null)
            return serial;
      }
      return null;
   }

   /**
    * Find name of asset attribute with system type "Serial", creating one if schema has none.
    *
    * @return attribute name and flag indicating that attribute was created by this method
    */
   private static Map.Entry<String, Boolean> findOrCreateSerialAttribute(NXCSession session) throws Exception
   {
      for(AssetAttribute a : session.getAssetManagementSchema().values())
      {
         if (a.getSystemType() == AMSystemType.SERIAL)
            return new AbstractMap.SimpleEntry<>(a.getName(), false);
      }
      AssetAttribute a = new AssetAttribute();
      a.setName(TEST_ATTRIBUTE_NAME);
      a.setDisplayName("Serial number (test)");
      a.setDataType(AMDataType.STRING);
      a.setSystemType(AMSystemType.SERIAL);
      session.createAssetAttribute(a);
      return new AbstractMap.SimpleEntry<>(TEST_ATTRIBUTE_NAME, true);
   }

   /**
    * Build property set satisfying all mandatory attributes of current schema.
    */
   private static Map<String, String> buildMandatoryProperties(NXCSession session)
   {
      Map<String, String> properties = new HashMap<>();
      for(AssetAttribute a : session.getAssetManagementSchema().values())
      {
         if (!a.isMandatory())
            continue;
         switch(a.getDataType())
         {
            case INTEGER:
            case NUMBER:
            case OBJECT_REFERENCE:
               properties.put(a.getName(), "1");
               break;
            case BOOLEAN:
               properties.put(a.getName(), "false");
               break;
            case ENUM:
               properties.put(a.getName(), a.getEnumValues().isEmpty() ? "" : a.getEnumValues().keySet().iterator().next());
               break;
            case MAC_ADDRESS:
               properties.put(a.getName(), "00:00:00:00:00:01");
               break;
            case IP_ADDRESS:
               properties.put(a.getName(), "127.0.0.1");
               break;
            case UUID:
               properties.put(a.getName(), UUID.randomUUID().toString());
               break;
            case DATE:
               properties.put(a.getName(), "20260101");
               break;
            default:
               properties.put(a.getName(), "test");
               break;
         }
      }
      return properties;
   }

   /**
    * Wait until asset object reaches expected link state (object updates are delivered asynchronously).
    */
   private static Asset waitForLinkState(NXCSession session, long assetId, long linkedObjectId, String componentSerial) throws Exception
   {
      Asset asset = null;
      for(int i = 0; i < 50; i++)
      {
         asset = session.findObjectById(assetId, Asset.class);
         if ((asset != null) && (asset.getLinkedObjectId() == linkedObjectId) && asset.getComponentSerial().equals(componentSerial))
            return asset;
         Thread.sleep(200);
      }
      assertNotNull(asset);
      assertEquals(linkedObjectId, asset.getLinkedObjectId());
      assertEquals(componentSerial, asset.getComponentSerial());
      return asset;
   }

   @Test
   public void testComponentAssetLinks() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      session.syncAssetManagementSchema();

      final Node node = TestHelper.findManagementServer(session);
      final String serial = findComponentSerial(session, node);
      Assumptions.assumeTrue(serial != null, "Management server node has no inventoried component with serial number");

      // Remove leftovers from previous failed run
      for(AbstractObject o : session.getAllObjects())
      {
         if ((o instanceof Asset) && o.getObjectName().equals(TEST_ASSET_NAME))
         {
            session.unlinkAsset(o.getObjectId());  // linked assets cannot be deleted by default
            session.deleteObject(o.getObjectId());
         }
      }

      Map.Entry<String, Boolean> serialAttribute = findOrCreateSerialAttribute(session);
      long assetId = 0;
      try
      {
         final long primaryAssetId = node.getAssetId();

         // Create asset linked to component
         NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_ASSET, TEST_ASSET_NAME, AbstractObject.ASSETS);
         cd.setAssetProperties(buildMandatoryProperties(session));
         cd.setLinkedObjectId(node.getObjectId());
         cd.setComponentSerial(serial);
         assetId = session.createObject(cd);

         Asset asset = waitForLinkState(session, assetId, node.getObjectId(), serial);
         assertTrue(asset.isComponent());
         assertEquals(serial, asset.getProperties().get(serialAttribute.getKey()));
         assertEquals(primaryAssetId, session.findObjectById(node.getObjectId()).getAssetId()); // node's own asset link not affected
         assertTrue(session.getComponentAssets(node.getObjectId()).stream().anyMatch(a -> a.getObjectId() == asset.getObjectId()));

         // Unlink
         session.unlinkAsset(assetId);
         Asset unlinked = waitForLinkState(session, assetId, 0, "");
         assertFalse(unlinked.isComponent());
         final long id = assetId;
         assertFalse(session.getComponentAssets(node.getObjectId()).stream().anyMatch(a -> a.getObjectId() == id));

         // Link requests validation
         assertRcc(RCC.INVALID_ARGUMENT, () -> session.linkAsset(id, node.getObjectId(), "ComponentAssetTest-no-such-serial"));
         assertRcc(RCC.INCOMPATIBLE_OPERATION, () -> session.linkAsset(id, AbstractObject.NETWORK, serial));
         waitForLinkState(session, assetId, 0, "");

         // Manual link to component
         session.linkAsset(assetId, node.getObjectId(), serial);
         waitForLinkState(session, assetId, node.getObjectId(), serial);

         // Automatic link by configuration poll: asset with matching serial number attribute should be linked back
         session.unlinkAsset(assetId);
         waitForLinkState(session, assetId, 0, "");
         session.pollObject(node.getObjectId(), ObjectPollType.CONFIGURATION, null);
         waitForLinkState(session, assetId, node.getObjectId(), serial);
      }
      finally
      {
         try
         {
            if (assetId != 0)
            {
               session.unlinkAsset(assetId);  // linked assets cannot be deleted by default
               session.deleteObject(assetId);
            }
         }
         finally
         {
            if (serialAttribute.getValue())
               session.deleteAssetAttribute(serialAttribute.getKey());
         }
      }
   }
}
