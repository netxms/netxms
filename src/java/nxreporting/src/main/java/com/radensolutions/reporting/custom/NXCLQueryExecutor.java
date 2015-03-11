package com.radensolutions.reporting.custom;

import com.radensolutions.reporting.ReportClassLoader;
import com.radensolutions.reporting.ThreadLocalReportInfo;
import com.radensolutions.reporting.service.ServerSettings;
import net.sf.jasperreports.engine.*;
import net.sf.jasperreports.engine.query.JRQueryExecuter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.net.MalformedURLException;
import java.net.URL;
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
//        JRParameter[] parameters = dataset.getParameters();
//        System.out.println("--------------------------");
//        for (JRParameter parameter : parameters) {
//            if (!parameter.isSystemDefined()) {
//            }
//            System.out.print(parameter.isSystemDefined());
//            System.out.print(" ");
//            System.out.println(parameter.getName());
//        }
//        System.out.println("--------------------------");
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
        String reportLocation = ThreadLocalReportInfo.getReportLocation();

        try {
            URL[] urls = {new URL("file:" + reportLocation)};
            ReportClassLoader classLoader = new ReportClassLoader(urls, getClass().getClassLoader());
            Class<NXCLDataSource> aClass = (Class<NXCLDataSource>) classLoader.loadClass("report.DataSource");
            NXCLDataSource dataSource = aClass.newInstance();
            JRQueryChunk chunk = dataset.getQuery().getChunks()[0];
            dataSource.setQuery(chunk.getText().trim());
            dataSource.connect(settings.getNetxmsServer(), settings.getNetxmsLogin(), settings.getNetxmsPassword());
            return dataSource;
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        } catch (MalformedURLException e) {
            e.printStackTrace();
        } catch (InstantiationException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
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
