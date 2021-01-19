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
import org.hibernate.HibernateException;
import org.hibernate.Session;
import org.hibernate.Transaction;
import org.hibernate.jdbc.Work;
import org.netxms.client.SessionNotification;
import org.netxms.client.reporting.ReportRenderFormat;
import org.netxms.client.reporting.ReportingJobConfiguration;
import org.netxms.reporting.Server;
import org.netxms.reporting.model.ReportDefinition;
import org.netxms.reporting.model.ReportParameter;
import org.netxms.reporting.model.ReportResult;
import org.netxms.reporting.tools.DateParameterParser;
import org.netxms.reporting.tools.ThreadLocalReportInfo;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import net.sf.jasperreports.engine.DefaultJasperReportsContext;
import net.sf.jasperreports.engine.JRAbstractExporter;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRExporterParameter;
import net.sf.jasperreports.engine.JRExpression;
import net.sf.jasperreports.engine.JRParameter;
import net.sf.jasperreports.engine.JRPropertiesMap;
import net.sf.jasperreports.engine.JasperCompileManager;
import net.sf.jasperreports.engine.JasperFillManager;
import net.sf.jasperreports.engine.JasperReport;
import net.sf.jasperreports.engine.export.JRPdfExporter;
import net.sf.jasperreports.engine.export.JRXlsExporter;
import net.sf.jasperreports.engine.export.JRXlsExporterParameter;
import net.sf.jasperreports.engine.query.QueryExecuterFactory;
import net.sf.jasperreports.engine.util.JRLoader;

@SuppressWarnings("deprecation")
public class ReportManager
{
   private static final String IDATA_VIEW_KEY = "IDATA_VIEW";
   private static final String SUBREPORT_DIR_KEY = "SUBREPORT_DIR";
   private static final String USER_ID_KEY = "SYS_USER_ID";
   private static final String DEFINITIONS_DIRECTORY = "definitions";
   private static final String FILE_SUFIX_DEFINITION = ".jrxml";
   private static final String FILE_SUFIX_COMPILED = ".jasper";
   private static final String MAIN_REPORT_COMPILED = "main" + FILE_SUFIX_COMPILED;
   private static final String FILE_SUFIX_FILLED = ".jrprint";

   private static final Logger logger = LoggerFactory.getLogger(ReportManager.class);

   private String workspace;
   private Server server;
   private ReportResultManager reportResultManager;
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
      workspace = server.getConfigurationProperty("workspace", "");
      reportMap = new HashMap<UUID, String>();
      reportResultManager = new ReportResultManager(server);
   }

   /**
    * Get list of available reports
    * 
    * @return list of available reports
    */
   public List<UUID> listReports()
   {
      ArrayList<UUID> li = new ArrayList<UUID>(reportMap.keySet());
      return li;
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
   public void deploy()
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
         {
            logger.debug("Deploying " + archiveName);
            try
            {
               String deployedName = archiveName.split("\\.(?=[^\\.]+$)")[0];
               File destination = new File(definitionsDirectory, deployedName);
               deleteFolder(destination);
               UUID bundleId = unpackJar(destination, new File(definitionsDirectory, archiveName));
               reportMap.put(bundleId, deployedName);
               compileReport(destination);
               logger.info("Report " + bundleId + " deployed as \"" + deployedName + "\" in " + destination.getAbsolutePath());
            }
            catch(IOException e)
            {
               e.printStackTrace();
            }
         }
      }
      else
      {
         logger.info("No files found");
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
    */
   private static UUID unpackJar(File destination, File archive) throws IOException
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
               if (!destDir.mkdirs())
               {
                  logger.error("Can't create directory " + destDir.getAbsolutePath());
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

         // TODO: handle possible exception
         return UUID.fromString(manifest.getMainAttributes().getValue("Build-Id"));
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
            return name.endsWith(FILE_SUFIX_DEFINITION);
         }
      });
      for(String fileName : list)
      {
         final File file = new File(reportDirectory, fileName);
         try
         {
            final String sourceFileName = file.getAbsolutePath();
            final String destinationFileName = sourceFileName.substring(0, sourceFileName.length() - FILE_SUFIX_DEFINITION.length())
                  + FILE_SUFIX_COMPILED;
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
   private File getDefinitionsDirectory()
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
      String path = reportMap.get(reportId);
      File file = null;
      if (path != null)
      {
         file = new File(getDefinitionsDirectory(), path);
      }
      return file;
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
   public boolean execute(int userId, UUID jobId, ReportingJobConfiguration jobConfiguration, String idataView, Locale locale)
   {
      final JasperReport report = loadReport(jobConfiguration.reportId);
      if (report == null)
      {
         logger.error("Cannot load report with UUID=" + jobConfiguration.reportId);
         return false;
      }

      boolean ret = false;
      final File reportDirectory = getReportDirectory(jobConfiguration.reportId);

      final ResourceBundle translations = loadReportTranslation(reportDirectory, locale);

      // fill report parameters
      final HashMap<String, Object> localParameters = new HashMap<String, Object>(jobConfiguration.executionParameters);
      localParameters.put(JRParameter.REPORT_LOCALE, locale);
      localParameters.put(JRParameter.REPORT_RESOURCE_BUNDLE, translations);
      String subrepoDirectory = reportDirectory.getPath() + File.separatorChar;
      localParameters.put(SUBREPORT_DIR_KEY, subrepoDirectory);
      localParameters.put(USER_ID_KEY, userId);
      if (idataView != null)
         localParameters.put(IDATA_VIEW_KEY, idataView);

      localParameters.put(JRParameter.REPORT_CLASS_LOADER, new URLClassLoader(new URL[] {}, getClass().getClassLoader()));

      prepareParameters(jobConfiguration.executionParameters, report, localParameters);
      logger.debug("Report parameters: " + localParameters);

      ThreadLocalReportInfo.setReportLocation(subrepoDirectory);
      ThreadLocalReportInfo.setServer(server);

      final File outputDirectory = getOutputDirectory(jobConfiguration.reportId);
      final String outputFile = new File(outputDirectory, jobId.toString() + ".jrprint").getPath();
      try
      {
         DefaultJasperReportsContext reportsContext = DefaultJasperReportsContext.getInstance();
         reportsContext.setProperty(QueryExecuterFactory.QUERY_EXECUTER_FACTORY_PREFIX + "nxcl", "org.netxms.reporting.nxcl.NXCLQueryExecutorFactory");
         final JasperFillManager manager = JasperFillManager.getInstance(reportsContext);
         Session session = server.getSessionFactory().getCurrentSession();
         Transaction transaction = session.beginTransaction();
         try
         {
            session.doWork(new Work() {
               @Override
               public void execute(Connection connection) throws SQLException
               {
                  try
                  {
                     manager.fillToFile(report, outputFile, localParameters, connection);
                  }
                  catch(JRException e)
                  {
                     throw new SQLException(e);
                  }
               }
            });
         }
         catch(HibernateException e)
         {
            transaction.rollback();
            dropDataView(idataView);
            throw e;
         }
         transaction.commit();
         reportResultManager.saveResult(new ReportResult(new Date(), jobConfiguration.reportId, jobId, userId));
         sendMailNotifications(jobConfiguration.reportId, report.getName(), jobId, jobConfiguration.renderFormat, jobConfiguration.emailRecipients);
         server.getCommunicationManager().sendNotification(SessionNotification.RS_RESULTS_MODIFIED, 0);
         dropDataView(idataView);
         ret = true;
      }
      catch(Exception e)
      {
         logger.error("Error executing report " + jobConfiguration.reportId + " " + report.getName(), e);
      }
      return ret;
   }

   /**
    * Drop data view created by server
    *
    * @param viewName view name
    */
   private void dropDataView(String viewName)
   {
      if (viewName == null)
         return;

      logger.debug("Drop data view " + viewName);
      Session session = server.getSessionFactory().getCurrentSession();
      Transaction transaction = session.beginTransaction();
      try
      {
         session.doWork(new Work() {
            @Override
            public void execute(Connection connection) throws SQLException
            {
               Statement stmt = connection.createStatement();
               stmt.execute("DROP VIEW " + viewName);
               stmt.close();
            }
         });
      }
      catch(HibernateException e)
      {
         logger.error("Cannot drop data view " + viewName, e);
         transaction.rollback();
         return;
      }
      transaction.commit();
   }

   private void prepareParameters(Map<String, String> parameters, JasperReport report, HashMap<String, Object> localParameters)
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
    * List available results for given report and user.
    *
    * @param reportId report ID
    * @param userId user ID
    * @return list of available results
    */
   public List<ReportResult> listResults(UUID reportId, int userId)
   {
      return reportResultManager.listResults(reportId, userId);
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
      reportResultManager.deleteReportResult(jobId);
      final File reportDirectory = getOutputDirectory(reportId);
      final File file = new File(reportDirectory, jobId.toString() + FILE_SUFIX_FILLED);
      return file.delete();
   }

   public File renderResult(UUID reportId, UUID jobId, ReportRenderFormat format)
   {
      final File outputDirectory = getOutputDirectory(reportId);
      final File dataFile = new File(outputDirectory, jobId.toString() + ".jrprint");

      File outputFile = new File(outputDirectory, jobId.toString() + "." + System.currentTimeMillis() + ".render");

      JRAbstractExporter exporter = null;
      switch(format)
      {
         case PDF:
            exporter = new JRPdfExporter();
            break;
         case XLS:
            exporter = new JRXlsExporter();
            final JasperReport jasperReport = loadReport(reportId);
            if (jasperReport != null)
            {
               exporter.setParameter(JRXlsExporterParameter.SHEET_NAMES,
                     new String[] { prepareXlsSheetName(jasperReport.getName()) });
            }
            exporter.setParameter(JRXlsExporterParameter.IS_IGNORE_CELL_BORDER, false);
            exporter.setParameter(JRXlsExporterParameter.IS_WHITE_PAGE_BACKGROUND, false);
            // Arrange cell spacing and remove paging gaps
            exporter.setParameter(JRXlsExporterParameter.IS_REMOVE_EMPTY_SPACE_BETWEEN_ROWS, true);
            exporter.setParameter(JRXlsExporterParameter.IS_COLLAPSE_ROW_SPAN, true);
            exporter.setParameter(JRXlsExporterParameter.IS_REMOVE_EMPTY_SPACE_BETWEEN_COLUMNS, true);
            exporter.setParameter(JRXlsExporterParameter.IS_ONE_PAGE_PER_SHEET, false);
            exporter.setParameter(JRXlsExporterParameter.IS_DETECT_CELL_TYPE, true);
            // Arrange graphics
            exporter.setParameter(JRXlsExporterParameter.IS_IMAGE_BORDER_FIX_ENABLED, true);
            exporter.setParameter(JRXlsExporterParameter.IS_FONT_SIZE_FIX_ENABLED, true);
            exporter.setParameter(JRXlsExporterParameter.IS_IGNORE_GRAPHICS, false);
            break;
         default:
            break;
      }

      exporter.setParameter(JRExporterParameter.INPUT_FILE, dataFile);

      try
      {
         exporter.setParameter(JRExporterParameter.OUTPUT_STREAM, new FileOutputStream(outputFile));
         exporter.exportReport();
      }
      catch(Exception e)
      {
         logger.error("Failed to render report", e);
         outputFile.delete();
         outputFile = null;
      }

      return outputFile;
   }

   /**
    * Prepare Excel sheet name. Excel sheet name doesn't contain special chars: / \ ? * ] [ Maximum sheet name length is 31 and
    * sheet names must not begin or end with ' (apostrophe)
    *
    * @param sheetName proposed sheet name
    * @return valid sheet name
    */
   private String prepareXlsSheetName(String sheetName)
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
