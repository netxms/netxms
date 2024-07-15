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
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.io.IOException;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.TemplateGroup;
import org.netxms.utilities.TestHelper;

/**
 * Testing inheritance of custom attributes.
 */
public class CustomAttributeInheritance extends AbstractSessionTest
{
   private final static long MILLIS_300 = 300;

   private final static String CONTAINER_A = "A-container";
   private final static String CONTAINER_B = "B-container";
   private final static String CONTAINER_C = "C-container";
   private final static String CONTAINER_V = "V-container";
   private final static String CONTAINER_D = "D-container";
   private final static String CONTAINER_E = "E-container";
   private final static String CONTAINER_F = "F-container";
   private final static String CONTAINER_G = "G-container";
   private final static String CONTAINER_H = "H-container";
   private final static String CONTAINER_I = "I-container";
   private final static String CONTAINER_J = "J-container";
   private final String[] containers = { CONTAINER_A, CONTAINER_B, CONTAINER_C, CONTAINER_V };
   
   private final static String TEMPLATE_GROUP = "Test template group";
   private final static String TEMPLATE_A = "Test template A";
   private final static String TEMPLATE_B = "Test template B";
   private final static String TEST_NODE = "Test node";

   private final static String CA_NAME_1 = "1";
   private final static String CA_NAME_2 = "2";

   private final static String CA_VALUE_A = "A";
   private final static String CA_VALUE_B = "B";
   private final static String CA_VALUE_V = "V";
   private final static String CA_VALUE_D = "D";
   private final static String CA_VALUE_E = "E";
   private final static String CA_VALUE_F = "F";
   private final static String CA_VALUE_I = "I";
   private final static String CA_VALUE_TG = "TG";
   private final static String CA_VALUE_T = "T";
   
   /**
    * Checks if a template or template group with the given name exists; if it doesn't exist, creates one.
    * 
    * @param session
    * @param objectName
    * @param objectType
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void checkObjectCreatedOrCreate(NXCSession session, String objectName, int objectType) throws IOException, NXCException, InterruptedException
   {
      boolean objectFound = false;
      for(AbstractObject object : session.getAllObjects())
      {
         if ((object instanceof Template && objectType == GenericObject.OBJECT_TEMPLATE) || (object instanceof TemplateGroup && objectType == GenericObject.OBJECT_TEMPLATEGROUP))
         {
            if (object.getObjectName().equals(objectName))
            {
               objectFound = true;
               break;
            }
         }
      }
      if (!objectFound)
      {
         NXCObjectCreationData cd = new NXCObjectCreationData(objectType, objectName, GenericObject.TEMPLATEROOT);
         cd.setPrimaryName(objectName);
         session.createObjectSync(cd);
      }
   }

   /**
    * Checks if a folder hierarchy exists within the given directory. If it doesn't exist, it creates the hierarchy and assigns the
    * necessary custom attributes.
    * 
    * @param session
    * @param containerName
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void checkOrCreateContainerHierarchy(NXCSession session, String containerName) throws IOException, NXCException, InterruptedException
   {
      boolean containerFound = false;

      for(AbstractObject object : session.getAllObjects())
      {
         if (object instanceof Container && ((Container)object).getObjectName().equals(containerName))
         {
            containerFound = true;
            break;
         }
      }
      if (!containerFound)
      {
         TestHelper.createContainer(session, CONTAINER_D);
         TestHelper.createContainer(session, CONTAINER_E);
         TestHelper.createContainer(session, CONTAINER_F);
         TestHelper.createContainer(session, CONTAINER_G);
         TestHelper.createContainer(session, CONTAINER_H);
         TestHelper.createContainer(session, CONTAINER_I);
         TestHelper.createContainer(session, CONTAINER_J);

         Container containerD = (Container)session.findObjectByName(CONTAINER_D);
         Container containerE = (Container)session.findObjectByName(CONTAINER_E);
         Container containerF = (Container)session.findObjectByName(CONTAINER_F);
         Container containerG = (Container)session.findObjectByName(CONTAINER_G);
         Container containerH = (Container)session.findObjectByName(CONTAINER_H);
         Container containerI = (Container)session.findObjectByName(CONTAINER_I);
         Container containerJ = (Container)session.findObjectByName(CONTAINER_J);

         TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_D, 1, containerD.getObjectId()); // sets the inheritable flag for the D
                                                                                              // container
         session.bindObject(containerD.getObjectId(), containerE.getObjectId());
         session.bindObject(containerD.getObjectId(), containerF.getObjectId());

         TestHelper.changeCustomAttributes(session, CA_NAME_2, CA_VALUE_E, 1, containerE.getObjectId()); // sets the inheritable flag for the E
                                                                                              // container
         session.bindObject(containerE.getObjectId(), containerG.getObjectId());
         session.bindObject(containerE.getObjectId(), containerH.getObjectId());

         TestHelper.changeCustomAttributes(session, CA_NAME_2, CA_VALUE_F, 1, containerF.getObjectId()); // sets the inheritable flag for the F
                                                                                              // container
         session.bindObject(containerF.getObjectId(), containerH.getObjectId());
         session.bindObject(containerF.getObjectId(), containerI.getObjectId());

         TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_I, 1, containerI.getObjectId()); // redefines the value of a custom
                                                                                              // attribute for I container
         session.bindObject(containerI.getObjectId(), containerJ.getObjectId());
         Thread.sleep(MILLIS_300);
      }
   } 
   // 1.Creates containers:
   // A (custom attribute: 1 A,Inheritable),
   // B,
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3.Removes inheritance flag from A.
   @Test
   public void testCaInheritance() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      
      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);
      
      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));
      
      Thread.sleep(MILLIS_300);

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId()); // sets the inheritable flag for the A
                                                                                           // container
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 0, containerA.getObjectId()); // remove the flag, A container no more
                                                                                           // inheritable
      Thread.sleep(MILLIS_300);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   // A (custom attribute: 1 A,Inheritable),
   // B (custom attribute: 1 B),
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3.Removes inheritance flag from A.
   @Test
   public void testCaInheritance2() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session,CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId()); // sets the inheritable flag for the A
                                                                                           // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_B, 0, containerB.getObjectId()); // redefines the value of a custom
                                                                                           // attribute for B container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerB.getCustomAttributeValue(CA_NAME_1)));
      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 0, containerA.getObjectId()); // remove the flag, A container no more
                                                                                           // inheritable
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNotNull(containerB.getCustomAttributeValue("1"));
      assertNotNull(containerC.getCustomAttributeValue("1"));

      session.disconnect();
   }

   // 1.Creates containers:
   // A (custom attribute: 1 A,Inheritable),
   // B,
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3.Removes custom attribute from A.
   @Test
   public void testCaInheritance3() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      assertEquals(containerA.getCustomAttributeValue(CA_NAME_1), containerB.getCustomAttributeValue(CA_NAME_1));
      assertEquals(containerA.getCustomAttributeValue(CA_NAME_1), containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, null, null, 0, containerA.getObjectId()); // remove CA from container A
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   // A (custom attribute: 1 A,Inheritable),
   // B (custom attribute: 1 B),
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3.Removes custom attribute from A.
   @Test
   public void testCaInheritance4() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_B, 1, containerB.getObjectId()); // redefines the value of a custom
                                                                                           // attribute for B container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerB.getCustomAttributeValue(CA_NAME_1)));
      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      TestHelper.changeCustomAttributes(session, null, null, 0, containerA.getObjectId()); // remove CA from container A
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNotNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNotNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   // A (custom attribute: 1 A,Inheritable),
   // B,
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3.Unbinds A from B A B->C.
   @Test
   public void testCaInheritance5() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      assertEquals(containerA.getCustomAttributeValue(CA_NAME_1), containerB.getCustomAttributeValue(CA_NAME_1));
      assertEquals(containerA.getCustomAttributeValue(CA_NAME_1), containerC.getCustomAttributeValue(CA_NAME_1));

      session.unbindObject(containerA.getObjectId(), containerB.getObjectId());
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   // A (custom attribute: 1 A,Inheritable),
   // B (custom attribute: 1 B),
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3.Unbinds A from B A B->C.
   @Test
   public void testCaInheritance6() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_B, 1, containerB.getObjectId()); // redefines the value of a custom
                                                                                           // attribute for B container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerB.getCustomAttributeValue(CA_NAME_1)));
      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.unbindObject(containerA.getObjectId(), containerB.getObjectId());
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNotNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNotNull(containerC.getCustomAttributeValue(CA_NAME_1));

     session.disconnect();
   }

   // 1.Creates containers:
   // A(custom attribute: 1 A,Inheritable),
   // B,
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3.Deletes container.
   @Test
   public void testCaInheritance7() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      assertEquals(containerA.getCustomAttributeValue(CA_NAME_1), containerB.getCustomAttributeValue(CA_NAME_1));
      assertEquals(containerA.getCustomAttributeValue(CA_NAME_1), containerC.getCustomAttributeValue(CA_NAME_1));

      session.deleteObject(containerA.getObjectId());
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   // A(custom attribute: 1 A,Inheritable),
   // B(custom attribute: 1 B),
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3.Deletes container.
   @Test
   public void testCaInheritance8() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_B, 1, containerB.getObjectId()); // redefines the value of a custom
                                                                                           // attribute for B container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerB.getCustomAttributeValue(CA_NAME_1)));
      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.deleteObject(containerA.getObjectId());
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNotNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNotNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }
   
   // 1.Creates containers:
   // A(custom attribute: 1 A,Inheritable),
   // B,
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3. Edits custom attribute value from A to H
   @Test
   public void testCaInheritance9() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      assertTrue(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerB.getCustomAttributeValue(CA_NAME_1)));
      assertTrue(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, "H", 1, containerA.getObjectId());// changes CA value
                                                                                                     // for the A container
      Thread.sleep(MILLIS_300);
      
      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerB.getCustomAttributeValue(CA_NAME_1)));
      assertTrue(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.disconnect();
   }

   // 1.Creates containers:
   // A(custom attribute: 1 A,Inheritable),
   // B(custom attribute: 1 B),
   // C.
   // 2.Creates a folder hierarchy A B->C.
   // 3.Creates a folder hierarchy A->B->C.
   @Test
   public void testCaInheritance10() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_B, 1, containerB.getObjectId());// redefines the value of a custom
                                                                                          // attribute for B container
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());
      assertEquals(containerB.getCustomAttributeValue(CA_NAME_1), containerC.getCustomAttributeValue(CA_NAME_1));

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));
      assertEquals(containerB.getCustomAttributeValue(CA_NAME_1), containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   // A(custom attribute: 1 A,Inheritable),
   // B(custom attribute: 1 B),
   // C(custom attribute: 1 C).
   // 2.Creates a folder hierarchy A B->C.
   // 3.Creates a folder hierarchy A->B->C.
   @Test
   public void testCaInheritance11() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_B, 1, containerB.getObjectId()); // redefines the value of a custom
                                                                                           // attribute for B container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, "C", 0, containerC.getObjectId()); // redefines the value of a custom attribute for
                                                                                    // C container
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());
      assertFalse(containerB.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.disconnect();
   }

   // 1.Creates containers:
   // A(custom attribute: 1 A,Inheritable),
   // B(custom attribute: 1 B),
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3. Edit folder hierarchy to C<-A->B.
   @Test
   public void testCaInheritance12() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_B, 1, containerB.getObjectId()); // redefines the value of a custom
                                                                                           // attribute for B container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerB.getCustomAttributeValue(CA_NAME_1)));
      assertTrue(containerB.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.bindObject(containerA.getObjectId(), containerC.getObjectId());
      session.unbindObject(containerB.getObjectId(), containerC.getObjectId());
      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));
      assertFalse(containerB.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.disconnect();
   }

   // 1.Creates containers:
   // A(custom attribute: 1 A,Inheritable),
   // B(custom attribute: 1 B, Inheritable),
   // C.
   // 2.Creates a folder hierarchy A->B->C.
   // 3.Edit folder hierarchy to A->C->B.
   @Test
   public void testCaInheritance13() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_B, 1, containerB.getObjectId()); // redefines the value of a custom
                                                                                           // attribute for B container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isInherited());

      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerB.getCustomAttributeValue(CA_NAME_1)));
      assertTrue(containerB.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.bindObject(containerA.getObjectId(), containerC.getObjectId());
      session.unbindObject(containerB.getObjectId(), containerC.getObjectId());
      session.bindObject(containerC.getObjectId(), containerB.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));
      assertFalse(containerB.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.disconnect();
   }

   // 1.Creates containers:
   // A(custom attribute: 1 A,Inheritable),
   // B(custom attribute: 2 B, Inheritable),
   // C,
   // V.
   //                                 V
   //                                 ^
   //                                 |
   // 2.Creates a folder hierarchy A->B->C.
   //
   //                               V
   //                               ^
   //                               |
   // 3.Edit folder hierarchy to A->C->B.
   @Test
   public void testCaInheritance14() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);
      TestHelper.createContainer(session, CONTAINER_V);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);
      Container containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerV.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId()); // sets the inheritable flag for the A
                                                                                           // container
      TestHelper.changeCustomAttributes(session, CA_NAME_2, CA_VALUE_B, 1, containerB.getObjectId()); // sets the inheritable flag for the A
                                                                                           // container
      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerV.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerB.getCustomAttributeValue(CA_NAME_1)));
      assertTrue(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerV.getCustomAttributeValue(CA_NAME_1)));
      assertTrue(containerB.getCustomAttributeValue(CA_NAME_2).equals(containerC.getCustomAttributeValue(CA_NAME_2)));
      assertTrue(containerB.getCustomAttributeValue(CA_NAME_2).equals(containerV.getCustomAttributeValue(CA_NAME_2)));

      session.unbindObject(containerB.getObjectId(), containerV.getObjectId());
      session.bindObject(containerA.getObjectId(), containerV.getObjectId());
      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertNull(containerV.getCustomAttribute(CA_NAME_2));

      session.disconnect();
   }

   // 1.Creates containers:
   // A(custom attribute: 1 A,Inheritable),
   // B(custom attribute: 1 B),
   // C,
   // V(custom attribute: 1 V)
   // 2.Creates a folder hierarchy V<-A->B->C.
   // 3.Edit folder hierarchy to C<-V<-A->B.
   @Test
   public void testCaInheritance15() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);
      TestHelper.createContainer(session, CONTAINER_V);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);
      Container containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerV.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId()); // sets the inheritable flag for the A
                                                                                           // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_B, 1, containerB.getObjectId()); // redefines the value of a custom
                                                                                           // attribute for B container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_V, 1, containerV.getObjectId()); // redefines the value of a custom
                                                                                           // attribute for V container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerA.getObjectId(), containerV.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerB.getCustomAttributeValue(CA_NAME_1)));
      assertFalse(containerA.getCustomAttributeValue(CA_NAME_1).equals(containerV.getCustomAttributeValue(CA_NAME_1)));
      assertTrue(containerB.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.unbindObject(containerB.getObjectId(), containerC.getObjectId());
      session.bindObject(containerV.getObjectId(), containerC.getObjectId());
      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerV.getCustomAttributeValue(CA_NAME_1).equals(containerC.getCustomAttributeValue(CA_NAME_1)));

      session.disconnect();
   }

   // 1.Creates objects:
   // TEMPLATE_GROUP(custom attribute: 1 TG, Inheritable),
   // TEMPLATE_A,
   // TEMPLATE_B(custom attribute: 1 T, Inheritable)
   // TEST_NODE

   // 2.Creates a hierarchy TEMPLATE_A<-TEMPLATE_GROUP->TEMPLATE_B
   //                               |
   //                               +->NODE
   //
   // 3.Edit folder hierarchy to TEMPLATE_A<-TEMPLATE_GROUP->TEMPLATE_B
   //                                  |                         |
   //                                  +---------->NODE<---------+
   @Test
   public void testCaInheritance16() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      checkObjectCreatedOrCreate(session, TEMPLATE_GROUP, GenericObject.OBJECT_TEMPLATEGROUP);
      checkObjectCreatedOrCreate(session, TEMPLATE_A, GenericObject.OBJECT_TEMPLATE);
      checkObjectCreatedOrCreate(session, TEMPLATE_B, GenericObject.OBJECT_TEMPLATE);
      TestHelper.checkNodeCreatedOrCreate(session, TEST_NODE);

      AbstractObject testTemplateGroup = session.findObjectByName(TEMPLATE_GROUP);
      AbstractObject testTemplateA = session.findObjectByName(TEMPLATE_A);
      AbstractObject testTemplateB = session.findObjectByName(TEMPLATE_B);
      AbstractObject testNode = session.findObjectByName(TEST_NODE);

      assertNull(testTemplateGroup.getCustomAttributeValue(CA_NAME_1));
      assertNull(testTemplateA.getCustomAttributeValue(CA_NAME_1));
      assertNull(testTemplateB.getCustomAttributeValue(CA_NAME_1));
      assertNull(testNode.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_TG, 1, testTemplateGroup.getObjectId()); // sets the inheritable flag for
                                                                                                   // the template group
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_T, 1, testTemplateB.getObjectId()); // sets the inheritable flag for the
                                                                                              // templateB

      session.bindObject(testTemplateGroup.getObjectId(), testTemplateA.getObjectId());
      session.bindObject(testTemplateGroup.getObjectId(), testTemplateB.getObjectId());
      session.bindObject(testTemplateA.getObjectId(), testNode.getObjectId());

      Thread.sleep(MILLIS_300);

      testTemplateGroup = session.findObjectByName(TEMPLATE_GROUP);
      testTemplateA = session.findObjectByName(TEMPLATE_A);
      testTemplateB = session.findObjectByName(TEMPLATE_B);
      testNode = session.findObjectByName(TEST_NODE);

      assertTrue(testTemplateGroup.getCustomAttributeValue(CA_NAME_1).equals(testTemplateA.getCustomAttributeValue(CA_NAME_1)));
      assertTrue(testTemplateGroup.getCustomAttributeValue(CA_NAME_1).equals(testNode.getCustomAttributeValue(CA_NAME_1)));
      assertFalse(testTemplateGroup.getCustomAttributeValue(CA_NAME_1).equals(testTemplateB.getCustomAttributeValue(CA_NAME_1)));
      assertTrue(testTemplateA.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(testTemplateA.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(testTemplateB.getCustomAttribute(CA_NAME_1).isRedefined());
      assertTrue(testNode.getCustomAttribute(CA_NAME_1).isInherited());

      session.bindObject(testTemplateB.getObjectId(), testNode.getObjectId());
      Thread.sleep(MILLIS_300);

      testNode = session.findObjectByName(TEST_NODE);
      assertTrue(testNode.getCustomAttribute(CA_NAME_1).isConflict());

      //to run next test
      session.deleteObject(testTemplateGroup.getObjectId());
      session.deleteObject(testTemplateA.getObjectId());
      session.deleteObject(testTemplateB.getObjectId());
      session.deleteObject(testNode.getObjectId());

      session.disconnect();
   }

   // Verifies that upon restart, the container hierarchy is preserved and custom attributes are correctly inherited.
   //
   //       E (CA: 2E) <- D (CA: 1D) -> F (CA: 2F)
   //        /                             \
   //        G <- --------> H <-------- --> I (CA: 1I)
   //                                         \
   //                                          ----> J
   @Test
   public void testCaInheritance17() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      checkOrCreateContainerHierarchy(session, CONTAINER_D);

      Container containerD = (Container)session.findObjectByName(CONTAINER_D);
      assertTrue(containerD.getCustomAttributeValue(CA_NAME_1).equals(CA_VALUE_D));
      assertTrue(containerD.getCustomAttribute(CA_NAME_1).isInheritable());

      Container containerE = (Container)session.findObjectByName(CONTAINER_E);
      assertTrue(containerE.getCustomAttributeValue(CA_NAME_1).equals(CA_VALUE_D));
      assertTrue(containerE.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerE.getCustomAttributeValue(CA_NAME_2).equals(CA_VALUE_E));
      assertTrue(containerE.getCustomAttribute(CA_NAME_2).isInheritable());

      Container containerF = (Container)session.findObjectByName(CONTAINER_F);
      assertTrue(containerF.getCustomAttributeValue(CA_NAME_1).equals(CA_VALUE_D));
      assertTrue(containerF.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerF.getCustomAttributeValue(CA_NAME_2).equals(CA_VALUE_F));
      assertTrue(containerF.getCustomAttribute(CA_NAME_2).isInheritable());

      Container containerG = (Container)session.findObjectByName(CONTAINER_G);
      assertTrue(containerG.getCustomAttributeValue(CA_NAME_1).equals(CA_VALUE_D));
      assertTrue(containerG.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerG.getCustomAttributeValue(CA_NAME_2).equals(CA_VALUE_E));
      assertTrue(containerG.getCustomAttribute(CA_NAME_2).isInheritable());

      Container containerH = (Container)session.findObjectByName(CONTAINER_H);
      assertTrue(containerH.getCustomAttributeValue(CA_NAME_1).equals(CA_VALUE_D));
      assertTrue(containerH.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerH.getCustomAttribute(CA_NAME_2).isInherited());
      assertTrue(containerH.getCustomAttribute(CA_NAME_2).isConflict());

      Container containerI = (Container)session.findObjectByName(CONTAINER_I);
      assertTrue(containerI.getCustomAttributeValue(CA_NAME_1).equals(CA_VALUE_I));
      assertTrue(containerI.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerI.getCustomAttribute(CA_NAME_1).isRedefined());
      assertTrue(containerI.getCustomAttributeValue(CA_NAME_2).equals(CA_VALUE_F));
      assertTrue(containerI.getCustomAttribute(CA_NAME_2).isInherited());

      Container containerJ = (Container)session.findObjectByName(CONTAINER_J);
      assertTrue(containerJ.getCustomAttributeValue(CA_NAME_1).equals(CA_VALUE_I));
      assertTrue(containerJ.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerJ.getCustomAttributeValue(CA_NAME_2).equals(CA_VALUE_F));
      assertTrue(containerJ.getCustomAttribute(CA_NAME_2).isInherited());

      session.disconnect();
   }
  
}
