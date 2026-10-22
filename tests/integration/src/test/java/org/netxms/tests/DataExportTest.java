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
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import org.junit.jupiter.api.Test;
import org.netxms.client.BackgroundTaskInfo;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.DciTier;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataExportMetric;
import org.netxms.client.datacollection.DataExportRequest;
import org.netxms.client.datacollection.DataExportResult;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;

/**
 * Tests for export of tagged DCI history
 */
public class DataExportTest extends AbstractSessionTest
{
   private static final String DCI_ENERGY = "DataExportTest-IT-Energy";
   private static final String DCI_POWER = "DataExportTest-Feed-Power";
   private static final int PUSHED_VALUES = 3;

   /**
    * Read ZIP entry as UTF-8 text
    */
   private static String readEntry(ZipFile zip, String name) throws Exception
   {
      ZipEntry entry = zip.getEntry(name);
      assertNotNull(entry, "missing archive member " + name);
      return new String(zip.getInputStream(entry).readAllBytes(), StandardCharsets.UTF_8);
   }

   /**
    * Parse CSV text into rows (test data contains no quoted separators)
    */
   private static List<String[]> parseCsv(String text)
   {
      List<String[]> rows = new ArrayList<>();
      for(String line : text.split("\n"))
      {
         if (!line.isEmpty())
            rows.add(line.split(",", -1));
      }
      return rows;
   }

   /**
    * Run export, wait for completion, and download archive
    */
   private static DataExportResult runExport(NXCSession session, DataExportRequest request) throws Exception
   {
      long taskId = session.startDataExport(request);
      for(int i = 0; i < 120; i++)
      {
         BackgroundTaskInfo info = session.getBackgroundTaskInfo(taskId);
         if (info.isFinished())
         {
            assertFalse(info.isFailed(), "export failed: " + info.getFailureReason());
            break;
         }
         Thread.sleep(500);
      }
      DataExportResult result = session.downloadDataExportFile(taskId, false, 60000);
      if (request.isKeepMapping())
      {
         DataExportResult mapping = session.downloadDataExportFile(taskId, true, 60000);
         String text = new String(java.nio.file.Files.readAllBytes(mapping.getFile().toPath()), StandardCharsets.UTF_8);
         assertTrue(text.startsWith("pseudonym,type,id,name\n"));
         assertTrue(text.contains(",metric,"));
         assertTrue(text.contains(",object,"));
         mapping.getFile().delete();
      }

      // Files are removed once every produced file has been downloaded
      try
      {
         session.downloadDataExportFile(taskId, false, 10000);
         fail("second download must fail");
      }
      catch(NXCException e)
      {
         assertEquals(RCC.INVALID_TASK_ID, e.getErrorCode());
      }
      return result;
   }

   @Test
   public void testDataExport() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node node = TestHelper.findManagementServer(session);
      assertNotNull(node);

      TestHelper.findAndDeleteDci(session, node, DCI_ENERGY);
      TestHelper.findAndDeleteDci(session, node, DCI_POWER);

      DataCollectionConfiguration dcc = session.openDataCollectionConfiguration(node.getObjectId());
      DataCollectionItem energy = new DataCollectionItem(dcc, 0);
      energy.setOrigin(DataOrigin.PUSH);
      energy.setDataType(DataType.FLOAT);
      energy.setName("DataExportTest.Energy");
      energy.setDescription(DCI_ENERGY);
      energy.setSystemTag("eed.eit");
      long energyId = dcc.modifyObject(energy);

      DataCollectionItem power = new DataCollectionItem(dcc, 0);
      power.setOrigin(DataOrigin.PUSH);
      power.setDataType(DataType.FLOAT);
      power.setName("DataExportTest.Power");
      power.setDescription(DCI_POWER);
      power.setSystemTag("power.in");
      long powerId = dcc.modifyObject(power);
      dcc.close();

      try
      {
         for(int i = 0; i < PUSHED_VALUES; i++)
         {
            session.pushDciData(node.getObjectId(), energyId, String.valueOf(1000.5 + i));
            session.pushDciData(node.getObjectId(), powerId, String.valueOf(42.25 + i));
            Thread.sleep(50);
         }

         Date timeFrom = new Date(System.currentTimeMillis() - 3600000L);
         Date timeTo = new Date(System.currentTimeMillis() + 60000L);

         // Preview of selection must list both tagged DCIs with their tags and the tier that will be read
         DataExportRequest preview = new DataExportRequest();
         preview.setRootObjectId(node.getObjectId());
         preview.setTagPatterns(Arrays.asList("eed.*", "power.*"));
         preview.setTimeFrom(timeFrom);
         preview.setTimeTo(timeTo);
         preview.setTier(DciTier.RAW);
         Map<Long, DataExportMetric> previewed = new HashMap<>();
         for(DataExportMetric m : session.getDataExportMetrics(preview))
            previewed.put(m.getDciId(), m);
         assertTrue(previewed.containsKey(energyId), "preview must contain energy DCI");
         assertTrue(previewed.containsKey(powerId), "preview must contain power DCI");
         assertEquals("eed.eit", previewed.get(energyId).getTag());
         assertEquals("power.in", previewed.get(powerId).getTag());
         assertEquals(node.getObjectId(), previewed.get(energyId).getOwnerId());
         assertEquals(DCI_ENERGY, previewed.get(energyId).getDescription());
         assertEquals(DataOrigin.PUSH, previewed.get(energyId).getOrigin());
         assertEquals(DciTier.RAW, previewed.get(energyId).getTier());

         // Pushed values reach the database on DB writer flush; retry export until all rows are present
         DataExportRequest request = new DataExportRequest();
         request.setRootObjectId(node.getObjectId());
         request.setTagPatterns(Arrays.asList("eed.*", "power.*"));
         request.setTimeFrom(timeFrom);
         request.setTimeTo(timeTo);
         request.setTier(DciTier.RAW);

         DataExportResult result = null;
         for(int attempt = 0; attempt < 12; attempt++)
         {
            if (result != null)
               result.getFile().delete();
            result = runExport(session, request);
            if (result.getSampleCount() >= PUSHED_VALUES * 2)
               break;
            Thread.sleep(10000);
         }
         assertEquals(2, result.getMetricCount());
         assertTrue(result.getSampleCount() >= PUSHED_VALUES * 2, "expected pushed samples in export, got " + result.getSampleCount());

         String energyMetricId;
         try (ZipFile zip = new ZipFile(result.getFile()))
         {
            String manifest = readEntry(zip, "manifest.json");
            assertTrue(manifest.contains("\"formatVersion\": 1"));
            assertTrue(manifest.contains("\"anonymized\": false"));
            assertTrue(manifest.contains("\"metricCount\": 2"));

            List<String[]> metrics = parseCsv(readEntry(zip, "metrics.csv"));
            assertEquals(3, metrics.size());
            assertEquals("metric_id", metrics.get(0)[0]);
            assertEquals("tag", metrics.get(0)[1]);
            Map<String, String[]> byTag = new HashMap<>();
            for(int i = 1; i < metrics.size(); i++)
               byTag.put(metrics.get(i)[1], metrics.get(i));
            assertTrue(byTag.containsKey("eed.eit"));
            assertTrue(byTag.containsKey("power.in"));
            String[] energyRow = byTag.get("eed.eit");
            energyMetricId = energyRow[0];
            assertEquals(36, energyMetricId.length());   // DCI GUID
            assertEquals(String.valueOf(node.getObjectId()), energyRow[2]);
            assertEquals(node.getObjectName(), energyRow[3]);
            assertEquals(DCI_ENERGY, energyRow[9]);
            assertEquals("float", energyRow[12]);
            assertEquals("push", energyRow[13]);
            assertEquals("raw", energyRow[15]);

            List<String[]> data = parseCsv(readEntry(zip, "data.csv"));
            assertEquals("metric_id,timestamp,value,min,max,count,quality,lower_bound,upper_bound,completeness,method_id", String.join(",", data.get(0)));
            int energyRows = 0;
            String previousTimestamp = "";
            for(int i = 1; i < data.size(); i++)
            {
               String[] row = data.get(i);
               if (row[0].equals(energyMetricId))
               {
                  energyRows++;
                  assertTrue(row[1].compareTo(previousTimestamp) >= 0, "samples must be ordered oldest first");
                  previousTimestamp = row[1];
                  assertTrue(row[1].endsWith("Z"));
                  assertTrue(Double.parseDouble(row[2]) >= 1000.5);
                  assertEquals("", row[6]);   // default quality is not written
               }
            }
            assertTrue(energyRows >= PUSHED_VALUES);
         }
         result.getFile().delete();

         // Anonymized export with mapping file and explicit DCI selection
         request = new DataExportRequest();
         request.setDciList(Arrays.asList(new DataExportRequest.DciReference(node.getObjectId(), energyId)));
         request.setTimeFrom(timeFrom);
         request.setTimeTo(timeTo);
         request.setTier(DciTier.RAW);
         request.setAnonymize(true);
         request.setKeepMapping(true);
         result = runExport(session, request);
         assertEquals(1, result.getMetricCount());

         String anonymizedId;
         try (ZipFile zip = new ZipFile(result.getFile()))
         {
            String manifest = readEntry(zip, "manifest.json");
            assertTrue(manifest.contains("\"anonymized\": true"));
            assertTrue(manifest.contains("\"explicitSelection\": true"));
            assertTrue(manifest.contains("\"serverId\": null"), "anonymized manifest must not contain server ID");

            String metricsText = readEntry(zip, "metrics.csv");
            assertFalse(metricsText.contains(node.getObjectName()), "anonymized catalog must not contain object name");
            assertFalse(metricsText.contains(DCI_ENERGY), "anonymized catalog must not contain DCI description");
            List<String[]> metrics = parseCsv(metricsText);
            assertEquals(2, metrics.size());
            anonymizedId = metrics.get(1)[0];
            assertEquals(36, anonymizedId.length());
            assertNotEquals(energyMetricId, anonymizedId);
            assertEquals("eed.eit", metrics.get(1)[1]);
            assertEquals("", metrics.get(1)[2], "anonymized catalog must not contain owner ID");
            assertTrue(metrics.get(1)[3].startsWith("node-"), "owner name must be class-based pseudonym");
         }
         result.getFile().delete();

         // Pseudonym must be stable across exports
         result = runExport(session, request);
         try (ZipFile zip = new ZipFile(result.getFile()))
         {
            List<String[]> metrics = parseCsv(readEntry(zip, "metrics.csv"));
            assertEquals(anonymizedId, metrics.get(1)[0]);
         }
         result.getFile().delete();
      }
      finally
      {
         dcc = session.openDataCollectionConfiguration(node.getObjectId());
         dcc.deleteObject(energyId);
         dcc.deleteObject(powerId);
         dcc.close();
         session.disconnect();
      }
   }
}
