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
 */package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Container;
import org.netxms.utilities.TestHelper;

/**
 * Testing inheritance of custom attributes. In this class creates conflicts for custom attributes and checks their appearance and
 * disappearance for correctness.
 */
public class CustomAttributeInheritanceConflict extends AbstractSessionTest
{
   private final static long MILLIS_300 = 300;

   private final static String CONTAINER_A = "A-container";
   private final static String CONTAINER_B = "B-container";
   private final static String CONTAINER_C = "C-container";
   private final static String CONTAINER_V = "V-container";
   private final static String CONTAINER_X = "X-container";
   String[] containers = { CONTAINER_A, CONTAINER_B, CONTAINER_C, CONTAINER_V, CONTAINER_X };

   private final static String CA_NAME_1 = "1";
   
   private final static String CA_VALUE_A = "A";
   private final static String CA_VALUE_B = "B";
   private final static String CA_VALUE_V = "V";
   
   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   V(custom attribute: 1 A, Inheritable),
   //   B,
   //   C.
   //                                  С
   //                                  ^
   //                                  |
   // 2.Creates a folder hierarchy A-> B <-V.
   //
   // 3.Deletes custom attribute from A and V containers
   @Test
   public void testCaInheritanceConflict1() throws Exception
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

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerV.getObjectId());// sets the inheritable flag for the V
                                                                                          // container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerV.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isConflict());

      TestHelper.changeCustomAttributes(session, null, null, 0, containerA.getObjectId()); // remove CA from container A
      TestHelper.changeCustomAttributes(session, null, null, 0, containerV.getObjectId()); // remove CA from container V
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }
   
   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   V(custom attribute: 1 A, Inheritable),
   //   B,
   //   C.
   //                                  С
   //                                  ^
   //                                  |
   // 2.Creates a folder hierarchy A-> B <-V.
   //
   // 3.Unbinds A and V from B 
   @Test
   public void testCaInheritanceConflict2() throws Exception
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

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerV.getObjectId());// sets the inheritable flag for the V
                                                                                          // container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerV.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isConflict());

      session.unbindObject(containerA.getObjectId(), containerB.getObjectId());
      session.unbindObject(containerV.getObjectId(), containerB.getObjectId());
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   V(custom attribute: 1 A, Inheritable),
   //   B,
   //   C.
   //                                  С
   //                                  ^
   //                                  |
   // 2.Creates a folder hierarchy A-> B <-V.
   //
   // 3.Removes inheritable flags from A and V containers
   @Test
   public void testCaInheritanceConflict3() throws Exception
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

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId());// sets the inheritable flag for the A
                                                                                          // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerV.getObjectId());// sets the inheritable flag for the V
                                                                                          // container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerV.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());
      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isConflict());

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 0, containerA.getObjectId());// remove the flag, A container no more
                                                                                          // inheritable
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 0, containerV.getObjectId());// remove the flag, V container no more
                                                                                          // inheritable
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   V(custom attribute: 1 V, Inheritable),
   //   B,
   //   C.
   //                                  С
   //                                  ^
   //                                  |
   // 2.Creates a folder hierarchy A-> B <-V.
   //
   // 3.Deletes custom attribute from A and V containers
   @Test
   public void testCaInheritanceConflict4() throws Exception
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
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_V, 1, containerV.getObjectId()); // sets the inheritable flag for the V
                                                                                           // container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerV.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isConflict());

      TestHelper.changeCustomAttributes(session, null, null, 0, containerA.getObjectId()); // remove CA from container A
      TestHelper.changeCustomAttributes(session, null, null, 0, containerV.getObjectId()); // remove CA from container V
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   V(custom attribute: 1 V, Inheritable),
   //   B,
   //   C.
   //                                  С
   //                                  ^
   //                                  |
   // 2.Creates a folder hierarchy A-> B <-V.
   //
   // 3.Unbinds A and V from B 
   @Test
   public void testCaInheritanceConflict5() throws Exception
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
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_V, 1, containerV.getObjectId()); // sets the inheritable flag for the V
                                                                                           // container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerV.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isConflict());

      session.unbindObject(containerA.getObjectId(), containerB.getObjectId());
      session.unbindObject(containerV.getObjectId(), containerB.getObjectId());
      session.unbindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   V(custom attribute: 1 A, Inheritable),
   //   B,
   //   C.
   //                                  С
   //                                  ^
   //                                  |
   // 2.Creates a folder hierarchy A-> B <-V.
   //
   // 3.Removes inheritable flags from A and V containers
   @Test
   public void testCaInheritanceConflict6() throws Exception
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
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_V, 1, containerV.getObjectId()); // sets the inheritable flag for the V
                                                                                           // container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerV.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isConflict());

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 0, containerA.getObjectId()); // remove the flag, A container no more
                                                                                           // inheritable
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_V, 0, containerV.getObjectId()); // remove the flag, V container no more
                                                                                           // inheritable
      Thread.sleep(MILLIS_300);

      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));

      session.disconnect();
   }

   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   V(custom attribute: 1 A, Inheritable),
   //   B,
   //   C.
   //                                  С
   //                                  ^
   //                                  |
   // 2.Creates a folder hierarchy A-> B <-V.
   // 3.Removes inheritable flag from A
   // 4.Returns inheritable flag to A
   @Test
   public void testCaInheritanceConflict7() throws Exception
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
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerV.getObjectId()); // sets the inheritable flag for the V
                                                                                           // container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerV.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isConflict());

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 0, containerA.getObjectId()); // remove the flag, A container no more
                                                                                           // inheritable
      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertFalse(containerB.getCustomAttribute(CA_NAME_1).isConflict());
      assertFalse(containerC.getCustomAttribute(CA_NAME_1).isConflict());

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId()); // sets the inheritable flag for the A
                                                                                           // container
      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isConflict()); // ZDESJ PADAJET
      assertFalse(containerC.getCustomAttribute(CA_NAME_1).isConflict());

      session.disconnect();
   }

   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   V(custom attribute: 1 V, Inheritable),
   //   B,
   //   C.
   //                                  С
   //                                  ^
   //                                  |
   // 2.Creates a folder hierarchy A-> B <-V.
   // 3.Removes inheritable flag from A
   // 4.Returns inheritable flag to A
   @Test
   public void testCaInheritanceConflict8() throws Exception
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
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_V, 1, containerV.getObjectId()); // sets the inheritable flag for the V
                                                                                           // container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerV.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInherited());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isInheritable());
      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isConflict());

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 0, containerA.getObjectId()); // remove the flag, A container no more
                                                                                           // inheritable
      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertFalse(containerB.getCustomAttribute(CA_NAME_1).isConflict());
      assertFalse(containerC.getCustomAttribute(CA_NAME_1).isConflict());

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId()); // sets the inheritable flag for the A
                                                                                           // container
      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerB.getCustomAttribute(CA_NAME_1).isConflict());
      assertFalse(containerC.getCustomAttribute(CA_NAME_1).isConflict());

      session.disconnect();
   }

   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   B,
   //   C.
   //                                   
   //                                 -> C <-
   //                               /         \
   // 2.Creates a folder hierarchy A --------> B 
   @Test
   public void testCaInheritanceConflict9() throws Exception
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

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId()); // sets the inheritable flag for the A
                                                                                           // container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerA.getObjectId(), containerC.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertFalse(containerC.getCustomAttribute(CA_NAME_1).isConflict());

      session.disconnect();
   }
   
   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   B(custom attribute: 1 A),
   //   C.
   //                                   
   //                                 -> C <-
   //                               /         \
   // 2.Creates a folder hierarchy A --------> B 
   @Test
   public void testCaInheritanceConflict10() throws Exception
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

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId()); // sets the inheritable flag for the A
                                                                                           // container
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_B, 1, containerB.getObjectId()); // redefines the value of a custom
                                                                                           // attribute for B container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerA.getObjectId(), containerC.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);

      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isConflict());

      session.disconnect();
   }

   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   B,
   //   C,
   //   V.                           
   //                               ->B---> C <-
   //                              |           |
   // 2.Creates a folder hierarchy A --------> V 
   @Test
   public void testCaInheritanceConflict11() throws Exception
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

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerA.getObjectId(), containerV.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());
      session.bindObject(containerV.getObjectId(), containerC.getObjectId());
      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertFalse(containerC.getCustomAttribute(CA_NAME_1).isConflict());

      session.disconnect();
   }

   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   B(custom attribute: 1 B),
   //   C,
   //   V(custom attribute: 1 V).                           
   //                               ->B---> C <-
   //                              |           |
   // 2.Creates a folder hierarchy A --------> V 
   @Test
   public void testCaInheritanceConflict12() throws Exception // WORKS
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
      session.bindObject(containerV.getObjectId(), containerC.getObjectId());
      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);

      assertTrue(containerC.getCustomAttribute(CA_NAME_1).isConflict());

      session.disconnect();
   }
   
   // 1.Creates containers:
   //   A(custom attribute: 1 A,Inheritable),
   //   B,
   //   C,
   //   V(custom attribute: 1 V),
   //   X.
   //                                   --> X <--
   //                                  /         \
   //                               ->B           C <-
   //                              |                 |
   // 2.Creates a folder hierarchy A --------------> V 
   @Test
   public void testCaInheritanceConflict13() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);
      TestHelper.createContainer(session, CONTAINER_V);
      TestHelper.createContainer(session, CONTAINER_X);

      Container containerA = (Container)session.findObjectByName(CONTAINER_A);
      Container containerB = (Container)session.findObjectByName(CONTAINER_B);
      Container containerC = (Container)session.findObjectByName(CONTAINER_C);
      Container containerV = (Container)session.findObjectByName(CONTAINER_V);
      Container containerX = (Container)session.findObjectByName(CONTAINER_X);

      assertNull(containerA.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerB.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerC.getCustomAttributeValue(CA_NAME_1));
      assertNull(containerV.getCustomAttributeValue(CA_NAME_1));

      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_A, 1, containerA.getObjectId()); // sets the inheritable flag
                                                                                                      // for the A 
      
      TestHelper.changeCustomAttributes(session, CA_NAME_1, CA_VALUE_V, 1, containerV.getObjectId()); // sets the inheritable flag
                                                                                                      // for the V container

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerV.getObjectId(), containerC.getObjectId());
      session.bindObject(containerB.getObjectId(), containerX.getObjectId());
      session.bindObject(containerC.getObjectId(), containerX.getObjectId());

      Thread.sleep(MILLIS_300);

      containerA = (Container)session.findObjectByName(CONTAINER_A);
      containerB = (Container)session.findObjectByName(CONTAINER_B);
      containerC = (Container)session.findObjectByName(CONTAINER_C);
      containerV = (Container)session.findObjectByName(CONTAINER_V);
      containerX = (Container)session.findObjectByName(CONTAINER_X);

      assertTrue(containerX.getCustomAttribute(CA_NAME_1).isConflict());

      session.disconnect();
   }

}
