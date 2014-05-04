package com.radensolutions.reporting.infrastructure.impl;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FilenameFilter;
import java.math.BigDecimal;
import java.net.URL;
import java.net.URLClassLoader;
import java.sql.Connection;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Date;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.ListResourceBundle;
import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;
import java.util.ResourceBundle;
import java.util.UUID;

import javax.sql.DataSource;

import net.sf.jasperreports.engine.JRAbstractExporter;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRExporterParameter;
import net.sf.jasperreports.engine.JRExpression;
import net.sf.jasperreports.engine.JRParameter;
import net.sf.jasperreports.engine.JRPropertiesMap;
import net.sf.jasperreports.engine.JasperCompileManager;
import net.sf.jasperreports.engine.JasperFillManager;
import net.sf.jasperreports.engine.JasperPrint;
import net.sf.jasperreports.engine.JasperReport;
import net.sf.jasperreports.engine.export.JRPdfExporter;
import net.sf.jasperreports.engine.export.JRXlsExporter;
import net.sf.jasperreports.engine.export.JRXlsExporterParameter;
import net.sf.jasperreports.engine.util.JRLoader;

import org.netxms.api.client.SessionNotification;
import org.netxms.api.client.reporting.ReportRenderFormat;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;

import com.radensolutions.reporting.application.PrepareParameterDate;
import com.radensolutions.reporting.application.ReportingServerFactory;
import com.radensolutions.reporting.application.Session;
import com.radensolutions.reporting.domain.Notification;
import com.radensolutions.reporting.domain.ReportDefinition;
import com.radensolutions.reporting.domain.ReportParameter;
import com.radensolutions.reporting.domain.ReportResult;
import com.radensolutions.reporting.infrastructure.ReportManager;
import com.radensolutions.reporting.service.NotificationService;
import com.radensolutions.reporting.service.ReportResultService;

public class FileSystemReportManager implements ReportManager
{
	public static final String SUBREPORT_DIR_KEY = "SUBREPORT_DIR";
	public static final String DEFINITIONS_DIRECTORY = "definitions";
	public static final String FILE_SUFIX_DEFINITION = ".jrxml";
	public static final String FILE_SUFIX_COMPILED = ".jasper";
	public static final String FILE_SUFIX_FILLED = ".jrprint";
	public static final String MAIN_REPORT_DEFINITION = "main" + FILE_SUFIX_DEFINITION;
	public static final String MAIN_REPORT_COMPILED = "main" + FILE_SUFIX_COMPILED;
	Logger log = LoggerFactory.getLogger(FileSystemReportManager.class);
	
	private String workspace;
	@Autowired
	@Qualifier("reportingDataSource")
	private DataSource dataSource;
	@Autowired
	private ReportResultService reportResultService;
	@Autowired
	private NotificationService notificationService;

	public void setWorkspace(String workspace)
	{
		this.workspace = workspace;
	}

	@Override
	public List<UUID> listReports()
	{
		List<UUID> reports = new ArrayList<UUID>(0);
		final File[] files = getDefinitionsDirectory().listFiles();
		if (files != null)
		{
			for (File file : files)
			{
				if (file.isDirectory())
				{
					try
					{
						reports.add(UUID.fromString(file.getName()));
					}
					catch (IllegalArgumentException e)
					{
						// ignore
					}
				}
			}
		}
		return reports;
	}

	@Override
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
			for (JRParameter jrParameter : parameters)
			{
				if (!jrParameter.isSystemDefined() && jrParameter.isForPrompting())
				{
					final JRPropertiesMap propertiesMap = jrParameter.getPropertiesMap();
					final String type = jrParameter.getValueClass().getName();
					String logicalType = getPropertyFromMap(propertiesMap, "logicalType", type);
					index = getPropertyFromMap(propertiesMap, "index", index);
					final JRExpression defaultValue = jrParameter.getDefaultValueExpression();
					String dependsOn = getPropertyFromMap(propertiesMap, "dependsOn", "");
					int span = getPropertyFromMap(propertiesMap, "span", index);
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
					definition.setParameter(parameter);
					index++;
				}
			}
		}
		return definition;
	}

	private JasperReport loadReport(UUID uuid)
	{
		JasperReport jasperReport = null;
		final File reportDirectory = getReportDirectory(uuid);
		final File reportFile = new File(reportDirectory, MAIN_REPORT_COMPILED);
		try
		{
			jasperReport = (JasperReport) JRLoader.loadObject(reportFile);
		} 
		catch (JRException e)
		{
			log.error("Can't load compiled report", e.getMessage());
		}
		return jasperReport;
	}

	@Override
	public void compileDeployedReports()
	{
		final List<UUID> list = listReports();
		for (UUID reportId : list)
		{
			final File reportDirectory = getReportDirectory(reportId);
			compileReport(reportDirectory);
		}
	}

	private void compileReport(File reportDirectory)
	{
		final String[] list = reportDirectory.list(new FilenameFilter()
		{
			@Override
			public boolean accept(File dir, String name)
			{
				return name.endsWith(FILE_SUFIX_DEFINITION);
			}
		});
		for (String fileName : list)
		{
			final File file = new File(reportDirectory, fileName);
			try
			{
				final String sourceFileName = file.getAbsolutePath();
				final String destinationFileName = sourceFileName.substring(0, sourceFileName.length() - FILE_SUFIX_DEFINITION.length()) + FILE_SUFIX_COMPILED;
				JasperCompileManager.compileReportToFile(sourceFileName, destinationFileName);
			}
			catch (JRException e)
			{
				log.error("Can't compile report " + file.getAbsoluteFile(), e);
			}
		}
	}

	private File getDefinitionsDirectory()
	{
		final File file = new File(workspace, DEFINITIONS_DIRECTORY);
		if (!file.exists())
		{
			file.mkdirs();
		}
		return file;
	}

	private File getReportDirectory(UUID reportId)
	{
		final File file = new File(getDefinitionsDirectory(), reportId.toString());
		if (!file.exists())
		{
			file.mkdirs();
		}
		return file;
	}

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

	@Override
	public boolean execute(int userId, UUID reportId, UUID jobId, final Map<String, Object> parameters, Locale locale)
	{
		boolean ret = false;

		Connection connection = null;
		try
		{
			connection = dataSource.getConnection();
			ret = realExecute(userId, reportId, jobId, parameters, locale, ret, connection);
		} catch (SQLException e)
		{
			log.error("Can't get report connection", e);
		} finally
		{
			if (connection != null)
			{
				try
				{
					connection.close();
				} catch (SQLException e)
				{
					// ignore
				}
			}
		}
		return ret;
	}

	private boolean realExecute(int userId, UUID reportId, UUID jobId, Map<String, Object> parameters, Locale locale, boolean ret, Connection connection)
	{
		final JasperReport report = loadReport(reportId);
		if (connection != null && report != null)
		{
			final File reportDirectory = getReportDirectory(reportId);

			final ResourceBundle translations = loadReportTranslation(reportDirectory, locale);
			final Enumeration<String> keys = translations.getKeys();

			// fill report parameters
			final HashMap<String, Object> localParameters = new HashMap<String, Object>(parameters);
			localParameters.put(JRParameter.REPORT_LOCALE, locale);
			localParameters.put(JRParameter.REPORT_RESOURCE_BUNDLE, translations);
			localParameters.put(SUBREPORT_DIR_KEY, reportDirectory.getPath() + File.separatorChar);

			PrepareParameterDate prepareDate = new PrepareParameterDate();
			for (Entry<String, Object> p : localParameters.entrySet())
			{
				if (p.getKey().equalsIgnoreCase("TIME_FROM") || p.getKey().equalsIgnoreCase("TIME_TO"))
				{
					Date date = prepareDate.getDateTime((String) p.getValue());
					if (date != null)
					{
						parameters.put(p.getKey(), String.valueOf(date.getTime() / 1000));
						p.setValue(date.getTime() / 1000);
					}
				}
			}
			
			prepareParameters(parameters, report, localParameters);

			System.out.println(localParameters);

			final File outputDirectory = getOutputDirectory(reportId);
			final String outputFile = new File(outputDirectory, jobId.toString() + ".jrprint").getPath();
			try
			{				
				JasperFillManager.fillReportToFile(report, outputFile, localParameters, connection);
				final JasperPrint print = JasperFillManager.fillReport(report, localParameters, connection);
				reportResultService.save(new ReportResult(new Date(), reportId, jobId, userId));
				
				// JasperViewer.viewReport(print);
				ret = true;
				
				Session session = ReportingServerFactory.getApp().getSession(ReportingServerFactory.getApp().getConnector());
				session.sendNotify(SessionNotification.RS_RESULTS_MODIFIED, 0);
				List<Notification> notify = notificationService.load(jobId);
				session.sendMailNotification(notify);
				
			} catch (JRException e)
			{
				log.error("Can't execute report", e);
			}
		}
		return ret;
	}

	private void prepareParameters(Map<String, Object> parameters, JasperReport report, HashMap<String, Object> localParameters)
	{
		final JRParameter[] jrParameters = report.getParameters();
		for (JRParameter jrParameter : jrParameters)
		{
			if (!jrParameter.isForPrompting() || jrParameter.isSystemDefined())
			{
				continue;
			}

			final String jrName = jrParameter.getName();
			final String input = (String) parameters.get(jrName);
			final Class<?> valueClass = jrParameter.getValueClass();

			if (Boolean.class.equals(valueClass))
			{
				localParameters.put(jrName, Boolean.parseBoolean(input));
			}
			else if (Date.class.equals(valueClass))
			{
				localParameters.put(jrName, new Date(Long.parseLong(input)));
			}
			else if (Double.class.equals(valueClass))
			{
				localParameters.put(jrName, Double.parseDouble(input));
			}
			else if (Float.class.equals(valueClass))
			{
				localParameters.put(jrName, Float.parseFloat(input));
			}
			else if (Integer.class.equals(valueClass))
			{
				localParameters.put(jrName, Integer.parseInt(input));
			}
			else if (Long.class.equals(valueClass))
			{
				localParameters.put(jrName, Long.parseLong(input));
			}
			else if (Short.class.equals(valueClass))
			{
				localParameters.put(jrName, Short.parseShort(input));
			}
			else if (BigDecimal.class.equals(valueClass))
			{
				localParameters.put(jrName, new BigDecimal(input));
			}
			else if (Collection.class.equals(valueClass) || List.class.equals(valueClass))
			{
				final List<String> list = Arrays.asList(input.trim().split("\\t"));
				localParameters.put(jrName, list);
			}
		}
	}

	@Override
	public List<ReportResult> listResults(UUID reportId, int userId)
	{
		return reportResultService.list(reportId, userId);
	}

	@Override
	public boolean deleteResult(UUID reportId, UUID jobId)
	{
		reportResultService.delete(jobId);
		final File reportDirectory = getOutputDirectory(reportId);
		final File file = new File(reportDirectory, jobId.toString() + FILE_SUFIX_FILLED);
		return file.delete();
	}

	@Override
	public byte[] renderResult(UUID reportId, UUID jobId, ReportRenderFormat format)
	{
		byte[] result = null;

		final File outputDirectory = getOutputDirectory(reportId);
		final File dataFile = new File(outputDirectory, jobId.toString() + ".jrprint");

		JRAbstractExporter exporter = null;
		switch (format)
		{
			case PDF:
				exporter = new JRPdfExporter();
				break;
			case XLS:
				exporter = new JRXlsExporter();
				final JasperReport jasperReport = loadReport(reportId);
				if (jasperReport != null)
				{
					exporter.setParameter(JRXlsExporterParameter.SHEET_NAMES, new String[] {prepareXlsSheetName(jasperReport.getName())});
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
		}

		exporter.setParameter(JRExporterParameter.INPUT_FILE, dataFile);
		final ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
		exporter.setParameter(JRExporterParameter.OUTPUT_STREAM, outputStream);

		try
		{
			exporter.exportReport();
			result = outputStream.toByteArray();
		} catch (JRException e)
		{
			log.error("Failed to render report", e);
		}

		return result;
	}
	
	/**
	 * Prepare Excel sheet name. Excel sheet name doesn't contain special chars: / \ ? * ] [
	 * Maximum sheet name length is 31 and sheet names must not begin or end with ' (apostrophe)
	 * 
	 * @param sheetName
	 * @return
	 */
	private String prepareXlsSheetName(String sheetName)
	{
		int length = Math.min(31, sheetName.length());
		StringBuilder stringBuilder = new StringBuilder(sheetName.substring(0, length));
		for (int i=0; i<length; i++)
		{
			char ch = stringBuilder.charAt(i);
			switch (ch)
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
					if (i==0 || i==length-1)	
						stringBuilder.setCharAt(i, ' ');
					break;
				default:
					break;
			}
		}
		return stringBuilder.toString();
	}

	private String getTranslation(ResourceBundle bundle, String name)
	{
		if (bundle.containsKey(name))
		{
			return bundle.getString(name);
		}
		return name;
	}

	private int getPropertyFromMap(JRPropertiesMap map, String key, int defaultValue)
	{
		if (map.containsProperty(key))
		{
			try
			{
				return Integer.parseInt(map.getProperty(key));
			} catch (NumberFormatException e)
			{
				return defaultValue;
			}
		}
		return defaultValue;
	}

	private String getPropertyFromMap(JRPropertiesMap map, String key, String defaultValue)
	{
		if (map.containsProperty(key))
		{
			return map.getProperty(key);
		}
		return defaultValue;
	}

	private ResourceBundle loadReportTranslation(File directory, Locale locale)
	{
		ResourceBundle labels = null;
		try
		{
			final URLClassLoader classLoader = URLClassLoader.newInstance(new URL[] { directory.toURI().toURL() });
			labels = ResourceBundle.getBundle("i18n", locale, classLoader);
		} catch (Exception e)
		{
			log.error("Can't load language bundle for report", e);
			// on error create empty bundle
			labels = new ListResourceBundle()
			{
				@Override
				protected Object[][] getContents()
				{
					return new Object[0][];
				}
			};
		}
		return labels;
	}
}