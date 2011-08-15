package org.netxms.report;

import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JasperCompileManager;
import net.sf.jasperreports.engine.JasperFillManager;
import net.sf.jasperreports.engine.JasperReport;

import java.io.FileInputStream;
import java.io.IOException;
import java.sql.Connection;
import java.sql.DriverManager;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

public class Generator {

    private final Map<String, Object> parameters = new HashMap<String, Object>(0);
    private String configFileName;
    private String sourceFileName;
    private String destinationFileName;

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
        System.out.println("Usage: <config> <source> <destination> [param1=INT:value] [param2=LIST:1,2,3] [param3=string_value]");
    }

    @SuppressWarnings({"ProhibitedExceptionDeclared"})
    private void run(final String[] args) throws Exception {
        parseCommandLine(args);
        loadConfig();

        Class.forName(config.getProperty(CONFIG_JDBCDRIVER));
        Connection connection = null;
        try {
            //noinspection CallToDriverManagerGetConnection
            connection = DriverManager.getConnection(config.getProperty(CONFIG_JDBCURL), config.getProperty(CONFIG_DBLOGIN), config.getProperty(CONFIG_DBPASSWORD));
            generate(connection);
        } finally {
            if (connection != null) {
                connection.close();
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

        for (final String arg : Arrays.copyOfRange(args, 3, args.length)) {
            final String[] keyValue = arg.split("=");
            if (keyValue.length == 2) {
                final String[] typeValue = keyValue[1].split(":");
                if (typeValue.length == 2) {
                    final String type = typeValue[0];
                    final String key = keyValue[0];
                    final String rawValue = typeValue[1];

                    if ("INT".equals(type)) {
                        parameters.put(key, Integer.valueOf(rawValue));
                    } else if ("LIST".equals(type)) {
                        final String[] listElements = rawValue.split(",");
                        parameters.put(key, Arrays.asList(listElements));
                    } else {
                        parameters.put(key, rawValue);
                    }
                }
            }
        }
    }

    private void generate(final Connection connection) throws JRException {
        final JasperReport report = JasperCompileManager.compileReport(sourceFileName);
        JasperFillManager.fillReportToFile(report, destinationFileName, parameters, connection);
    }
}
