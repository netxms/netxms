/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.reporting.services;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.math.BigDecimal;
import java.net.URL;
import java.net.URLClassLoader;
import java.sql.Connection;
import java.sql.SQLException;
import java.sql.Statement;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collection;
import java.util.Date;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.ListResourceBundle;
import java.util.Locale;
import java.util.Map;
import java.util.ResourceBundle;
import java.util.UUID;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.jar.Manifest;
import org.netxms.client.SessionNotification;
import org.netxms.client.reporting.ReportRenderFormat;
import org.netxms.client.reporting.ReportResult;
import org.netxms.client.reporting.ReportingJobConfiguration;
import org.netxms.reporting.ReportClassLoader;
import org.netxms.reporting.Server;
import org.netxms.reporting.ServerException;
import org.netxms.reporting.extensions.ExecutionHook;
import org.netxms.reporting.model.ReportDefinition;
import org.netxms.reporting.model.ReportParameter;
import org.netxms.reporting.tools.DateParameterParser;
import org.netxms.reporting.tools.ThreadLocalReportInfo;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import net.sf.jasperreports.engine.DefaultJasperReportsContext;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRExpression;
import net.sf.jasperreports.engine.JRParameter;
import net.sf.jasperreports.engine.JRPropertiesMap;
import net.sf.jasperreports.engine.JasperCompileManager;
import net.sf.jasperreports.engine.JasperFillManager;
import net.sf.jasperreports.engine.JasperReport;
import net.sf.jasperreports.engine.export.JRPdfExporter;
import net.sf.jasperreports.engine.export.JRXlsExporter;
import net.sf.jasperreports.engine.query.QueryExecuterFactory;
import net.sf.jasperreports.engine.util.JRLoader;
import net.sf.jasperreports.export.SimpleExporterInput;
import net.sf.jasperreports.export.SimpleOutputStreamExporterOutput;
import net.sf.jasperreports.export.SimpleXlsReportConfiguration;

/**
 * Report manager
 */
public class ReportManager
{
   private static final String IDATA_VIEW_KEY = "IDATA_VIEW";
   private static final String SUBREPORT_DIR_KEY = "SUBREPORT_DIR";
   private static final String USER_ID_KEY = "SYS_USER_ID";
   private static final String DEFINITIONS_DIRECTORY = "definitions";
   private static final String FILE_SUFFIX_DEFINITION = ".jrxml";
   private static final String FILE_SUFFIX_COMPILED = ".jasper";
   private static final String FILE_SUFFIX_FILLED = ".jrprint";
   private static final String FILE_SUFFIX_METADATA = ".meta";
   private static final String MAIN_REPORT_COMPILED = "main" + FILE_SUFFIX_COMPILED;

   private static final Logger logger = LoggerFactory.getLogger(ReportManager.class);

   private Server server;
   private String workspace;
   private Map<UUID, String> reportMap;

   /**
    * Create new report manager.
    * 
    * @param server owning server
    * @param communicationManager communication manager
    * @param notificationManager notification manager
    */
   public ReportManager(Server server)
   {
      this.server = server;
      workspace = server.getConfigurationProperty("nxreportd.workspace", "");
      reportMap = new HashMap<UUID, String>();
   }

   /**
    * Get list of available reports
    * 
    * @return list of available reports
    */
   public List<UUID> listReports()
   {
      synchronized(reportMap)
      {
         ArrayList<UUID> li = new ArrayList<UUID>(reportMap.keySet());
         return li;
      }
   }

   /**
    * Get report definition by GUID.
    *
    * @param reportId report GUID
    * @param locale locale to use
    * @return report definition or null if not found
    */
   public ReportDefinition getReportDefinition(UUID reportId, Locale locale)
   {
      ReportDefinition definition = null;

      final JasperReport jasperReport = loadReport(reportId);
      if (jasperReport != null)
      {
         final File reportDirectory = getReportDirectory(reportId);
         ResourceBundle labels = loadReportTranslation(reportDirectory, locale);

         definition = new ReportDefinition();
         definition.setName(jasperReport.getName());
         final int numberOfColumns = getPropertyFromMap(jasperReport.getPropertiesMap(), "numberOfColumns", 1);
         definition.setNumberOfColumns(numberOfColumns);

         final JRParameter[] parameters = jasperReport.getParameters();
         int index = 0;
         for(JRParameter jrParameter : parameters)
         {
            if (!jrParameter.isSystemDefined() && jrParameter.isForPrompting())
            {
               final JRPropertiesMap propertiesMap = jrParameter.getPropertiesMap();
               final String type = jrParameter.getValueClass().getName();
               String logicalType = getPropertyFromMap(propertiesMap, "logicalType", type);
               index = getPropertyFromMap(propertiesMap, "index", index);
               final JRExpression defaultValue = jrParameter.getDefaultValueExpression();
               String dependsOn = getPropertyFromMap(propertiesMap, "dependsOn", "");
               int span = getPropertyFromMap(propertiesMap, "span", 1);
               if (span < 1)
               {
                  span = 1;
               }

               final ReportParameter parameter = new ReportParameter();
               final String name = jrParameter.getName();
               parameter.setName(name);
               parameter.setType(logicalType);
               parameter.setIndex(index);
               parameter.setDescription(getTranslation(labels, name));
               parameter.setDefaultValue(defaultValue == null ? null : defaultValue.getText());
               parameter.setDependsOn(dependsOn);
               parameter.setSpan(span);
               definition.putParameter(parameter);
               index++;
            }
         }
      }
      return definition;
   }

   /**
    * Load report with given GUID.
    * 
    * @param uuid report GUID
    * @return report object or null
    */
   private JasperReport loadReport(UUID uuid)
   {
      JasperReport jasperReport = null;
      final File reportDirectory = getReportDirectory(uuid);
      final File reportFile = new File(reportDirectory, MAIN_REPORT_COMPILED);
      try
      {
         jasperReport = (JasperReport)JRLoader.loadObject(reportFile);
      }
      catch(JRException e)
      {
         logger.error("Cannot load compiled report from " + reportFile, e);
      }
      return jasperReport;
   }

   /**
    * Deploy reports from jar or zip packages
    */
   public void deployAll()
   {
      File definitionsDirectory = getDefinitionsDirectory();
      logger.info("Deploying *.jar/*.zip in " + definitionsDirectory.getAbsolutePath());
      String[] files = definitionsDirectory.list(new FilenameFilter() {
         @Override
         public boolean accept(File dir, String name)
         {
            String normalizedName = name.toLowerCase();
            return normalizedName.endsWith(".jar") || normalizedName.endsWith(".zip");
         }
      });
      if (files != null)
      {
         for(String archiveName : files)
            deploy(archiveName);
      }
      else
      {
         logger.info("No files found");
      }
   }

   /**
    * Deploy specific archive
    *
    * @param archiveName archive name
    */
   public void deploy(String archiveName)
   {
      File definitionsDirectory = getDefinitionsDirectory();
      logger.debug("Deploying " + archiveName);
      try
      {
         String deployedName = archiveName.split("\\.(?=[^\\.]+$)")[0];
         File destination = new File(definitionsDirectory, deployedName);
         deleteFolder(destination);
         UUID bundleId = unpackJar(destination, new File(definitionsDirectory, archiveName));
         compileReport(destination);
         synchronized(reportMap)
         {
            reportMap.put(bundleId, deployedName);
         }
         logger.info("Report " + bundleId + " deployed as \"" + deployedName + "\" in " + destination.getAbsolutePath());
         validateResults(bundleId);
      }
      catch(Exception e)
      {
         logger.error("Error while deploying package " + archiveName);
      }
   }

   /**
    * Delete given folder recursively
    *
    * @param folder folder to delete
    */
   private static void deleteFolder(File folder)
   {
      File[] files = folder.listFiles();
      if (files != null)
      {
         for(File f : files)
         {
            if (f.isDirectory())
            {
               deleteFolder(f);
            }
            else
            {
               f.delete();
            }
         }
      }
      folder.delete();
   }

   /**
    * Unpack zip/jar to given destination.
    *
    * @param destination destination directory
    * @param archive source archive
    * @return Build-Id attribute from manifest
    * @throws IOException if archive cannot be unpacked
    * @throws ServerException if there are errors in archive
    */
   private static UUID unpackJar(File destination, File archive) throws IOException, ServerException
   {
      JarFile jarFile = new JarFile(archive);
      try
      {
         Manifest manifest = jarFile.getManifest();
         Enumeration<JarEntry> entries = jarFile.entries();
         while(entries.hasMoreElements())
         {
            JarEntry jarEntry = entries.nextElement();
            if (jarEntry.isDirectory())
            {
               File destDir = new File(destination, jarEntry.getName());
               if (!destDir.isDirectory() && !destDir.mkdirs())
               {
                  throw new ServerException("Cannot create directory " + destDir.getAbsolutePath());
               }
            }
         }

         entries = jarFile.entries();
         while(entries.hasMoreElements())
         {
            JarEntry jarEntry = entries.nextElement();
            File destinationFile = new File(destination, jarEntry.getName());

            InputStream inputStream = null;
            FileOutputStream outputStream = null;
            if (jarEntry.isDirectory())
            {
               continue;
            }
            try
            {
               inputStream = jarFile.getInputStream(jarEntry);
               outputStream = new FileOutputStream(destinationFile);

               byte[] buffer = new byte[1024];
               int len;
               assert inputStream != null;
               while((len = inputStream.read(buffer)) > 0)
               {
                  outputStream.write(buffer, 0, len);
               }
            }
            finally
            {
               if (inputStream != null)
               {
                  inputStream.close();
               }
               if (outputStream != null)
               {
                  outputStream.close();
               }
            }
         }

         String buildId = manifest.getMainAttributes().getValue("Build-Id");
         if (buildId == null)
            throw new ServerException("Missing Build-Id attribute in package manifest");
         return UUID.fromString(buildId);
      }
      finally
      {
         jarFile.close();
      }
   }

   /**
    * Compile report in given directory
    *
    * @param reportDirectory report directory
    */
   private static void compileReport(File reportDirectory)
   {
      final String[] list = reportDirectory.list(new FilenameFilter() {
         @Override
         public boolean accept(File dir, String name)
         {
            return name.endsWith(FILE_SUFFIX_DEFINITION);
         }
      });
      for(String fileName : list)
      {
         final File file = new File(reportDirectory, fileName);
         try
         {
            final String sourceFileName = file.getAbsolutePath();
            final String destinationFileName = sourceFileName.substring(0, sourceFileName.length() - FILE_SUFFIX_DEFINITION.length())
                  + FILE_SUFFIX_COMPILED;
            JasperCompileManager.compileReportToFile(sourceFileName, destinationFileName);
         }
         catch(JRException e)
         {
            logger.error("Cannot compile report " + file.getAbsoluteFile(), e);
         }
      }
   }

   /**
    * Get directory with report definitions
    *
    * @return directory with report definitions
    */
   public File getDefinitionsDirectory()
   {
      return new File(workspace, DEFINITIONS_DIRECTORY);
   }

   /**
    * Get directory for report with given GUID.
    *
    * @param reportId report GUID
    * @return report directory or null
    */
   private File getReportDirectory(UUID reportId)
   {
      String path;
      synchronized(reportMap)
      {
         path = reportMap.get(reportId);
      }
      return (path != null) ? new File(getDefinitionsDirectory(), path) : null;
   }

   /**
    * Get output directory for report with given GUID
    * 
    * @param reportId report GUID
    * @return output directory for report
    */
   private File getOutputDirectory(UUID reportId)
   {
      final File output = new File(workspace, "output");
      if (!output.exists())
      {
         output.mkdirs();
      }
      final File file = new File(output, reportId.toString());
      if (!file.exists())
      {
         file.mkdirs();
      }
      return file;
   }

   /**
    * Execute report.
    *
    * @param userId user ID
    * @param jobId job GUID
    * @param jobConfiguration reporting job configuration
    * @param idataView name of database view for idata tables access or null if not provided
    * @param locale locale for translation
    * @return true on success and false otherwise
    */
   public void execute(int userId, UUID jobId, ReportingJobConfiguration jobConfiguration, String idataView, Locale locale)
   {
      final JasperReport report = loadReport(jobConfiguration.reportId);
      if (report == null)
      {
         logger.error("Cannot load report with UUID=" + jobConfiguration.reportId);
         return;
      }

      if ((idataView == null) || idataView.isEmpty())
      {
         logger.error("Error executing report " + jobConfiguration.reportId + " " + report.getName() + ": DCI data view not provided");
         saveResult(new ReportResult(jobId, jobConfiguration.reportId, new Date(), userId, false));
         return;
      }

      final File reportDirectory = getReportDirectory(jobConfiguration.reportId);
      final ResourceBundle translations = loadReportTranslation(reportDirectory, locale);

      // fill report parameters
      final HashMap<String, Object> localParameters = new HashMap<String, Object>(jobConfiguration.executionParameters);
      localParameters.put(JRParameter.REPORT_LOCALE, locale);
      localParameters.put(JRParameter.REPORT_RESOURCE_BUNDLE, translations);
      String subrepoDirectory = reportDirectory.getPath() + File.separatorChar;
      localParameters.put(SUBREPORT_DIR_KEY, subrepoDirectory);
      localParameters.put(USER_ID_KEY, userId);
      localParameters.put(IDATA_VIEW_KEY, idataView);

      URLClassLoader reportClassLoader = new URLClassLoader(new URL[] {}, getClass().getClassLoader());
      localParameters.put(JRParameter.REPORT_CLASS_LOADER, reportClassLoader);

      prepareParameters(jobConfiguration.executionParameters, report, localParameters);
      logger.debug("Report parameters: " + localParameters);

      ThreadLocalReportInfo.setReportLocation(subrepoDirectory);
      ThreadLocalReportInfo.setServer(server);

      Connection dbConnection = null;
      final String outputFile = new File(getOutputDirectory(jobConfiguration.reportId), jobId.toString() + FILE_SUFFIX_FILLED).getPath();
      try
      {
         dbConnection = server.createDatabaseConnection();

         executeHook("PreparationHook", subrepoDirectory, localParameters, dbConnection);

         DefaultJasperReportsContext reportsContext = DefaultJasperReportsContext.getInstance();
         reportsContext.setProperty(QueryExecuterFactory.QUERY_EXECUTER_FACTORY_PREFIX + "nxcl", "org.netxms.reporting.nxcl.NXCLQueryExecutorFactory");
         final JasperFillManager manager = JasperFillManager.getInstance(reportsContext);
         manager.fillToFile(report, outputFile, localParameters, dbConnection);

         saveResult(new ReportResult(jobId, jobConfiguration.reportId, new Date(), userId, true));
         sendMailNotifications(jobConfiguration.reportId, report.getName(), jobId, jobConfiguration.renderFormat, jobConfiguration.emailRecipients);

         executeHook("CleanupHook", subrepoDirectory, localParameters, dbConnection);
      }
      catch(Throwable e)
      {
         logger.error("Error executing report " + jobConfiguration.reportId + " " + report.getName(), e);
         try
         {
            saveResult(new ReportResult(jobId, jobConfiguration.reportId, new Date(), userId, false));
         }
         catch(Throwable t)
         {
            logger.error("Unexpected exception while saving failure result", t);
         }
      }
      finally
      {
         if (dbConnection != null)
         {
            dropDataView(dbConnection, idataView);
            try
            {
               dbConnection.close();
            }
            catch(SQLException e)
            {
               logger.error("Unexpected error while closing database connection", e);
            }
         }
      }
      server.getCommunicationManager().sendNotification(SessionNotification.RS_RESULTS_MODIFIED, 0);
      logger.info("Report execution completed (reportId=" + jobConfiguration.reportId + ", jobId=" + jobId + ")");
   }

   /**
    * Execute hook code.
    *
    * @param hookName hook name
    * @param reportLocation location of report's files
    * @param parameters report execution parameters
    * @param dbConnection database connection
    * @throws Exception on any unrecoverable error
    */
   @SuppressWarnings("unchecked")
   private void executeHook(String hookName, String reportLocation, HashMap<String, Object> parameters, Connection dbConnection) throws Exception
   {
      ReportClassLoader classLoader = null;
      try
      {
         URL[] urls = { new URL("file:" + reportLocation) };
         classLoader = new ReportClassLoader(urls, getClass().getClassLoader());
         Class<ExecutionHook> hookClass = (Class<ExecutionHook>)classLoader.loadClass("report." + hookName);
         ExecutionHook hook = hookClass.getConstructor().newInstance();
         logger.info("Running report execution hook " + hookName);
         hook.run(parameters, dbConnection);
      }
      catch(ClassNotFoundException e)
      {
         // ignore
      }
      finally
      {
         try
         {
            if (classLoader != null)
               classLoader.close();
         }
         catch(Exception e)
         {
            logger.warn("Unexpected exception while closing report hook class loader", e);
         }
      }
   }

   /**
    * Drop data view created by server
    *
    * @param viewName view name
    */
   private void dropDataView(Connection dbConnection, String viewName)
   {
      if (viewName == null)
         return;

      logger.debug("Drop data view " + viewName);
      try
      {
         Statement stmt = dbConnection.createStatement();
         stmt.execute("DROP VIEW " + viewName);
         stmt.close();
      }
      catch(Exception e)
      {
         logger.error("Cannot drop data view " + viewName, e);
         return;
      }
   }

   /**
    * Prepare report parameters
    *
    * @param parameters input parameters
    * @param report report object
    * @param localParameters local report parameters
    */
   private static void prepareParameters(Map<String, String> parameters, JasperReport report, HashMap<String, Object> localParameters)
   {
      final JRParameter[] jrParameters = report.getParameters();
      for(JRParameter jrParameter : jrParameters)
      {
         if (!jrParameter.isForPrompting() || jrParameter.isSystemDefined())
         {
            continue;
         }

         final String jrName = jrParameter.getName();
         final String input = parameters.get(jrName);
         final Class<?> valueClass = jrParameter.getValueClass();

         String logicalType = jrParameter.getPropertiesMap().getProperty("logicalType");

         if ("START_DATE".equalsIgnoreCase(logicalType))
         {
            Date date = DateParameterParser.getDateTime(input, false);
            localParameters.put(jrName, date.getTime() / 1000);
         }
         else if ("END_DATE".equalsIgnoreCase(logicalType))
         {
            Date date = DateParameterParser.getDateTime(input, true);
            localParameters.put(jrName, date.getTime() / 1000);
         }
         else if ("SEVERITY_LIST".equalsIgnoreCase(logicalType))
         {
            localParameters.put(jrName, convertCommaSeparatedToIntList(input));
         }
         else if ("OBJECT_ID_LIST".equalsIgnoreCase(logicalType))
         {
            localParameters.put(jrName, convertCommaSeparatedToIntList(input));
         }
         else if ("EVENT_CODE".equalsIgnoreCase(logicalType))
         {
            if ("0".equals(input))
            {
               localParameters.put(jrName, new ArrayList<Integer>(0));
            }
            else
            { // not "<any>"
               localParameters.put(jrName, convertCommaSeparatedToIntList(input));
            }
         }
         else if (Boolean.class.equals(valueClass))
         {
            localParameters.put(jrName, Boolean.parseBoolean(input));
         }
         else if (Date.class.equals(valueClass))
         {
            long timestamp = 0;
            try
            {
               timestamp = Long.parseLong(input);
            }
            catch(NumberFormatException e)
            {
               // ignore?
            }
            localParameters.put(jrName, new Date(timestamp));
         }
         else if (Double.class.equals(valueClass))
         {
            double value = 0.0;
            try
            {
               value = Double.parseDouble(input);
            }
            catch(NumberFormatException e)
            {
               // ignore?
            }
            localParameters.put(jrName, value);
         }
         else if (Float.class.equals(valueClass))
         {
            float value = 0.0f;
            try
            {
               value = Float.parseFloat(input);
            }
            catch(NumberFormatException e)
            {
               // ignore?
            }
            localParameters.put(jrName, value);
         }
         else if (Integer.class.equals(valueClass))
         {
            int value = 0;
            try
            {
               value = Integer.parseInt(input);
            }
            catch(NumberFormatException e)
            {
               // ignore?
            }
            localParameters.put(jrName, value);
         }
         else if (Long.class.equals(valueClass))
         {
            long value = 0L;
            try
            {
               value = Long.parseLong(input);
            }
            catch(NumberFormatException e)
            {
               // ignore?
            }
            localParameters.put(jrName, value);
         }
         else if (Short.class.equals(valueClass))
         {
            short value = 0;
            try
            {
               value = Short.parseShort(input);
            }
            catch(NumberFormatException e)
            {
               // ignore?
            }
            localParameters.put(jrName, value);
         }
         else if (BigDecimal.class.equals(valueClass))
         {
            BigDecimal value = BigDecimal.valueOf(0);
            try
            {
               value = new BigDecimal(input);
            }
            catch(NumberFormatException e)
            {
               // ignore?
            }
            localParameters.put(jrName, value);
         }
         else if (Collection.class.equals(valueClass) || List.class.equals(valueClass))
         {
            final List<String> list = Arrays.asList(input.trim().split("\\t"));
            localParameters.put(jrName, list);
         }
      }
   }

   /**
    * Convert string with comma-separated list of integers to list of integers.
    * 
    * @param input input string
    * @return resulting list
    */
   private static List<Integer> convertCommaSeparatedToIntList(String input)
   {
      if (input == null)
         return null;

      String[] strings = input.split(",");
      List<Integer> ret = new ArrayList<Integer>(strings.length);
      for(String s : strings)
      {
         try
         {
            ret.add(Integer.parseInt(s));
         }
         catch(NumberFormatException e)
         {
            // TODO: handle
            logger.error("Invalid ID in comma separated list: " + input);
         }
      }
      return ret;
   }

   /**
    * Save execution result object
    *
    * @param result execution result object
    */
   public void saveResult(ReportResult result)
   {
      try
      {
         File outputFile = new File(getOutputDirectory(result.getReportId()), result.getJobId().toString() + FILE_SUFFIX_METADATA);
         result.saveAsXml(outputFile);
      }
      catch(Exception e)
      {
         logger.error("Error saving report execution result", e);
      }
   }

   /**
    * List available results for given report and user.
    *
    * @param reportId report ID
    * @param userId user ID
    * @return list of available results
    */
   public List<ReportResult> listResults(UUID reportId, int userId)
   {
      File[] files = getOutputDirectory(reportId).listFiles(new FilenameFilter() {
         @Override
         public boolean accept(File dir, String name)
         {
            String normalizedName = name.toLowerCase();
            return normalizedName.endsWith(FILE_SUFFIX_METADATA);
         }
      });

      List<ReportResult> results = new ArrayList<ReportResult>(files.length);
      for(File f : files)
      {
         try
         {
            results.add(ReportResult.loadFromFile(f));
         }
         catch(Exception e)
         {
            logger.error("Error reading report execution metadata from file " + f, e);
         }
      }

      return results;
   }

   /**
    * Delete report result
    *
    * @param reportId report ID
    * @param jobId job ID
    * @return true on success
    */
   public boolean deleteResult(UUID reportId, UUID jobId)
   {
      File reportDirectory = getOutputDirectory(reportId);
      boolean success = true;

      File file = new File(reportDirectory, jobId.toString() + FILE_SUFFIX_FILLED);
      if (file.exists())
         success = file.delete();

      file = new File(reportDirectory, jobId.toString() + FILE_SUFFIX_METADATA);
      if (file.exists())
         success = file.delete() && success;

      return success;
   }

   /**
    * Validate report execution results.
    *
    * @param reportId report ID
    */
   private void validateResults(UUID reportId)
   {
      File[] files = getOutputDirectory(reportId).listFiles(new FilenameFilter() {
         @Override
         public boolean accept(File dir, String name)
         {
            String normalizedName = name.toLowerCase();
            return normalizedName.endsWith(FILE_SUFFIX_FILLED);
         }
      });
      for(File f : files)
      {
         File metaFile = new File(f.getAbsolutePath().replace(FILE_SUFFIX_FILLED, FILE_SUFFIX_METADATA));
         if (!metaFile.exists())
         {
            logger.warn("Missing metadata for report result file " + f.getAbsolutePath());
            try
            {
               UUID jobId = UUID.fromString(f.getName().substring(0, f.getName().indexOf('.')));
               saveResult(new ReportResult(jobId, reportId, new Date(f.lastModified()), 0, true));
            }
            catch(Throwable t)
            {
               logger.error("Cannot create missing metadata file", t);
            }
         }
      }
   }

   /**
    * Render report result
    *
    * @param reportId report ID
    * @param jobId job ID
    * @param format rendering format
    * @return file with rendered results on success and null on failure
    */
   public File renderResult(UUID reportId, UUID jobId, ReportRenderFormat format)
   {
      final File outputDirectory = getOutputDirectory(reportId);
      final File dataFile = new File(outputDirectory, jobId.toString() + FILE_SUFFIX_FILLED);
      final File outputFile = new File(outputDirectory, jobId.toString() + "." + System.currentTimeMillis() + ".render");

      try
      {
         switch(format)
         {
            case PDF:
               renderPDF(dataFile, outputFile);
               break;
            case XLS:
               renderXLS(dataFile, outputFile, loadReport(reportId));
               break;
            default:
               logger.error("Unsupported rendering format " + format);
               return null;
         }
         return outputFile;
      }
      catch(Throwable e)
      {
         logger.error("Failed to render report", e);
         outputFile.delete();
         return null;
      }
   }

   /**
    * Render report to PDF format.
    *
    * @param inputFile input file
    * @param outputFile output file
    * @throws Exception on error
    */
   private static void renderPDF(File inputFile, File outputFile) throws Exception
   {
      JRPdfExporter exporter = new JRPdfExporter();
      exporter.setExporterInput(new SimpleExporterInput(inputFile));
      exporter.setExporterOutput(new SimpleOutputStreamExporterOutput(new FileOutputStream(outputFile)));
      exporter.exportReport();
   }

   /**
    * Render report to XLS format.
    *
    * @param inputFile input file
    * @param outputFile output file
    * @param report report object
    * @throws Exception on error
    */
   private static void renderXLS(File inputFile, File outputFile, JasperReport report) throws Exception
   {
      SimpleXlsReportConfiguration configuration = new SimpleXlsReportConfiguration();
      if (report != null)
         configuration.setSheetNames(new String[] { prepareXlsSheetName(report.getName()) });
      configuration.setIgnoreCellBorder(false);
      configuration.setWhitePageBackground(false);
      // Arrange cell spacing and remove paging gaps
      configuration.setRemoveEmptySpaceBetweenRows(true);
      configuration.setRemoveEmptySpaceBetweenColumns(true);
      configuration.setCollapseRowSpan(true);
      configuration.setOnePagePerSheet(false);
      configuration.setDetectCellType(true);
      // Arrange graphics
      configuration.setImageBorderFixEnabled(true);
      configuration.setFontSizeFixEnabled(true);
      configuration.setIgnoreGraphics(false);

      JRXlsExporter exporter = new JRXlsExporter();
      exporter.setConfiguration(configuration);
      exporter.setExporterInput(new SimpleExporterInput(inputFile));
      exporter.setExporterOutput(new SimpleOutputStreamExporterOutput(new FileOutputStream(outputFile)));
      exporter.exportReport();
   }

   /**
    * Prepare Excel sheet name. Excel sheet name doesn't contain special chars: / \ ? * ] [ Maximum sheet name length is 31 and
    * sheet names must not begin or end with ' (apostrophe)
    *
    * @param sheetName proposed sheet name
    * @return valid sheet name
    */
   private static String prepareXlsSheetName(String sheetName)
   {
      int length = Math.min(31, sheetName.length());
      StringBuilder stringBuilder = new StringBuilder(sheetName.substring(0, length));
      for(int i = 0; i < length; i++)
      {
         char ch = stringBuilder.charAt(i);
         switch(ch)
         {
            case '/':
            case '\\':
            case '?':
            case '*':
            case ']':
            case '[':
               stringBuilder.setCharAt(i, ' ');
               break;
            case '\'':
               if (i == 0 || i == length - 1)
                  stringBuilder.setCharAt(i, ' ');
               break;
            default:
               break;
         }
      }
      return stringBuilder.toString();
   }

   /**
    * Get translation for given string
    *
    * @param bundle bundle to use
    * @param name string name
    * @return translated string or same string if translation not found
    */
   private static String getTranslation(ResourceBundle bundle, String name)
   {
      if (bundle.containsKey(name))
      {
         return bundle.getString(name);
      }
      return name;
   }

   /**
    * Get integer property from map.
    *
    * @param map map to use
    * @param key key name
    * @param defaultValue default value
    * @return property value or default value
    */
   private static int getPropertyFromMap(JRPropertiesMap map, String key, int defaultValue)
   {
      if (map.containsProperty(key))
      {
         try
         {
            return Integer.parseInt(map.getProperty(key));
         }
         catch(NumberFormatException e)
         {
            return defaultValue;
         }
      }
      return defaultValue;
   }

   /**
    * Get string property from map.
    *
    * @param map map to use
    * @param key key name
    * @param defaultValue default value
    * @return property value or default value
    */
   private static String getPropertyFromMap(JRPropertiesMap map, String key, String defaultValue)
   {
      if (map.containsProperty(key))
      {
         return map.getProperty(key);
      }
      return defaultValue;
   }

   /**
    * Load translation from report directory
    *
    * @param directory report directory
    * @param locale locale to use
    * @return bundle with localized strings or empty bundle if not found
    */
   private static ResourceBundle loadReportTranslation(File directory, Locale locale)
   {
      ResourceBundle labels = null;
      try
      {
         final URLClassLoader classLoader = URLClassLoader.newInstance(new URL[] { directory.toURI().toURL() });
         labels = ResourceBundle.getBundle("i18n", locale, classLoader);
      }
      catch(Exception e)
      {
         logger.error("Cannot load language bundle for report", e);
         // on error create empty bundle
         labels = new ListResourceBundle() {
            @Override
            protected Object[][] getContents()
            {
               return new Object[0][];
            }
         };
      }
      return labels;
   }

   /**
    * Send an email notification
    *
    * @param reportId report ID
    * @param reportName report nane
    * @param jobId job ID
    * @param renderFormat requested render format
    * @param recipients list of mail recipients
    */
   private void sendMailNotifications(UUID reportId, String reportName, UUID jobId, ReportRenderFormat renderFormat, List<String> recipients)
   {
      if (recipients.isEmpty())
      {
         logger.info("Notification recipients list is empty");
         return;
      }

      DateFormat df = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss");
      Date today = Calendar.getInstance().getTime();
      String reportDate = df.format(today);

      String text = String.format("Report \"%s\" successfully generated on %s and can be accessed using management console.", reportName, reportDate);

      String fileName = null;
      File renderResult = null;
      if (renderFormat != ReportRenderFormat.NONE)
      {
         renderResult = renderResult(reportId, jobId, renderFormat);
         String time = new SimpleDateFormat("dd-MM-yyyy").format(new Date());
         fileName = String.format("%s %s.%s", reportName, time, renderFormat == ReportRenderFormat.PDF ? "pdf" : "xls");
         text += "\n\nPlease find attached copy of the report.";
      }
      text += "\n\nThis message is generated automatically by NetXMS.";

      for(String r : recipients)
         server.getSmtpSender().sendMail(r, "New report is available", text, fileName, renderResult);
      if (renderResult != null)
         renderResult.delete();
   }
}
