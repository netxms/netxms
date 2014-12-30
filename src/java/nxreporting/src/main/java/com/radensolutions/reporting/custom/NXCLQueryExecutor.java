package com.radensolutions.reporting.custom;

import com.radensolutions.reporting.service.ServerSettings;
import net.sf.jasperreports.engine.*;
import net.sf.jasperreports.engine.query.JRQueryExecuter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;

public class NXCLQueryExecutor implements JRQueryExecuter {

    private static final Logger log = LoggerFactory.getLogger(NXCLQueryExecutor.class);

    private ServerSettings settings;
    private JRDataset dataset;
    private Map<String, ? extends JRValueParameter> parametersMap;

    protected NXCLQueryExecutor(JasperReportsContext jasperReportsContext, JRDataset dataset, Map<String, ? extends JRValueParameter> parametersMap) {
        this.dataset = dataset;
        this.parametersMap = parametersMap;
//        super(jasperReportsContext, dataset, parametersMap);
//        parseQuery();
        JRParameter[] parameters = dataset.getParameters();
        System.out.println("--------------------------");
        for (JRParameter parameter : parameters) {
            if (!parameter.isSystemDefined()) {
            }
            System.out.print(parameter.isSystemDefined());
            System.out.print(" ");
            System.out.println(parameter.getName());
        }
        System.out.println("--------------------------");
    }

//    @Override
//    protected String getParameterReplacement(String parameterName) {
//        System.out.println("getParameterReplacement: " + parameterName);
//        Object value = getParameterValue(parameterName);
//        System.out.println(value);
//        return "test value";
//    }

    @Override
    public JRDataSource createDatasource() throws JRException {
//        String queryString = getQueryString();
//        String queryString = "blah";
        JRQueryChunk chunk = dataset.getQuery().getChunks()[0];
        String queryString = chunk.getText().trim();
        log.debug("query[0]: " + queryString);
        if (queryString.equalsIgnoreCase("epp")) {
            EppDataSource dataSource = new EppDataSource();
            dataSource.connect(settings.getNetxmsServer(), settings.getNetxmsLogin(), settings.getNetxmsPassword());
            return dataSource;
        } else if (queryString.equalsIgnoreCase("ip-inventory")) {
            IPInventoryDataSource dataSource = new IPInventoryDataSource(dataset, parametersMap);
            dataSource.connect(settings.getNetxmsServer(), settings.getNetxmsLogin(), settings.getNetxmsPassword());
            return dataSource;
        } else if (queryString.equalsIgnoreCase("perf-cpu")) {
            PerformanceDataSource dataSource = new PerformanceDataSource(PerformanceDataSource.Type.CPU, dataset, parametersMap);
            dataSource.connect(settings.getNetxmsServer(), settings.getNetxmsLogin(), settings.getNetxmsPassword());
            return dataSource;
        } else if (queryString.equalsIgnoreCase("perf-traffic")) {
            PerformanceDataSource dataSource = new PerformanceDataSource(PerformanceDataSource.Type.TRAFFIC, dataset, parametersMap);
            dataSource.connect(settings.getNetxmsServer(), settings.getNetxmsLogin(), settings.getNetxmsPassword());
            return dataSource;
        } else if (queryString.equalsIgnoreCase("perf-disk")) {
            PerformanceDataSource dataSource = new PerformanceDataSource(PerformanceDataSource.Type.DISK, dataset, parametersMap);
            dataSource.connect(settings.getNetxmsServer(), settings.getNetxmsLogin(), settings.getNetxmsPassword());
            return dataSource;
        } else if (queryString.equalsIgnoreCase("inventory-by-platform")) {
            InventoryDataSource dataSource = new InventoryDataSource(dataset, parametersMap);
            dataSource.connect(settings.getNetxmsServer(), settings.getNetxmsLogin(), settings.getNetxmsPassword());
            return dataSource;
        }
        return null;
    }

    @Override
    public void close() {
    }

    @Override
    public boolean cancelQuery() throws JRException {
        System.out.println("cancelQuery");
        return false;
    }

    public void setSettings(ServerSettings settings) {
        this.settings = settings;
    }
}
