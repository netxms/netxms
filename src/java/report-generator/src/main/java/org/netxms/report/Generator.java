package org.netxms.report;

import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRParameter;
import net.sf.jasperreports.engine.JasperCompileManager;
import net.sf.jasperreports.engine.JasperFillManager;
import net.sf.jasperreports.engine.JasperReport;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.UnsupportedEncodingException;
import java.sql.Connection;
import java.sql.DriverManager;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

@SuppressWarnings({"ProhibitedExceptionThrown"})
public class Generator {

    private final Map<String, String> rawParameters = new HashMap<String, String>(0);
    private String configFileName;
    private String sourceFileName;
    private String destinationFileName;
    private String reportDefinition;

    private Properties config;
    private static final String CONFIG_JDBCURL = "JDBCUrl";
    private static final String CONFIG_DBLOGIN = "DBLogin";
    private static final String CONFIG_DBPASSWORD = "DBPassword";
    private static final String CONFIG_JDBCDRIVER = "JDBCDriver";

    public static void main(final String[] args) {
        int ret = 0;
        if (args.length >= 3) {
            try {
                new Generator().run(args);
            } catch (Exception e) {
                System.out.println("ERROR: " + e.getMessage());
                ret = 2;
            }
        } else {
            help();
            ret = 1;
        }

        //noinspection CallToSystemExit
        System.exit(ret);
    }

    private static void help() {
        System.out.println("Usage: <config> <source> <destination>");
    }

    @SuppressWarnings({"ProhibitedExceptionDeclared"})
    private void run(final String[] args) throws Exception {
        parseCommandLine(args);
        loadConfig();
        loadDefinitionAndParameters();
        final JasperReport report = prepare();

        final String dataSourceName;
        if (report.getPropertiesMap().containsProperty("netxms.datasource")) {
            dataSourceName = '-' + report.getProperty("netxms.datasource");
        } else {
            dataSourceName = "";
        }

        final String driverName = config.getProperty(CONFIG_JDBCDRIVER + dataSourceName);
        if (driverName == null) {
            throw new RuntimeException(CONFIG_JDBCDRIVER + dataSourceName + " not found in config file");
        }
        Class.forName(driverName);
        
        Connection connection = null;
        try {
            //noinspection CallToDriverManagerGetConnection
            final String url = config.getProperty(CONFIG_JDBCURL + dataSourceName);
            final String user = config.getProperty(CONFIG_DBLOGIN + dataSourceName);
            final String password = config.getProperty(CONFIG_DBPASSWORD + dataSourceName);

            if (url == null) {
                throw new RuntimeException(CONFIG_JDBCURL + dataSourceName + " not found in config file");
            }

            connection = DriverManager.getConnection(url, user, password);
            generate(connection, report);
        } finally {
            if (connection != null) {
                connection.close();
            }
        }
    }

    private void loadDefinitionAndParameters() throws IOException, UnsupportedEncodingException {
        Reader reader = null;
        try {
            reader = new InputStreamReader(new FileInputStream(sourceFileName), "UTF-8");
            final BufferedReader bufferedReader = new BufferedReader(reader);

            boolean inBody = true;
            final StringBuilder builder = new StringBuilder(4096);
            String line = bufferedReader.readLine();
            while (line != null) {
                if (inBody) {
                    if (line.startsWith("###")) {
                        inBody = false;
                    } else {
                        builder.append(line).append('\n');
                    }
                } else {
                    final int position = line.indexOf('=');
                    if (position != -1) {
                        final String key = line.substring(0, position);
                        final String value = line.substring(position + 1);
                        rawParameters.put(key, value);
                    }
                }
                line = bufferedReader.readLine();
            }
            reportDefinition = builder.toString();
        } finally {
            if (reader != null) {
                reader.close();
            }
        }
    }

    private void loadConfig() throws IOException {
        config = new Properties();
        FileInputStream inStream = null;
        try {
            inStream = new FileInputStream(configFileName);
            config.load(inStream);
        } finally {
            if (inStream != null) {
                inStream.close();
            }
        }
    }

    private void parseCommandLine(final String[] args) {
        configFileName = args[0];
        sourceFileName = args[1];
        destinationFileName = args[2];
    }

    private void generate(final Connection connection, final JasperReport report) throws JRException, UnsupportedEncodingException {
        final Map<String, Object> parameters = new HashMap<String, Object>(rawParameters.size());
        for (final JRParameter parameter : report.getParameters()) {
            final String name = parameter.getName();
            final String rawParameter = rawParameters.get(name);
            if (rawParameter != null) {
                final Class valueClass = parameter.getValueClass();
                if (Integer.class.equals(valueClass)) {
                    parameters.put(name, Integer.valueOf(rawParameter));
                } else {
                    parameters.put(name, rawParameter);
                }
            }
        }
        JasperFillManager.fillReportToFile(report, destinationFileName, parameters, connection);
    }

    private JasperReport prepare() throws JRException, UnsupportedEncodingException {
        return JasperCompileManager.compileReport(new ByteArrayInputStream(reportDefinition.getBytes("utf-8")));
    }

}
