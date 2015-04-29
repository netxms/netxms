package com.radensolutions.reporting.custom;

import com.radensolutions.reporting.service.ServerSettings;
import com.radensolutions.reporting.service.impl.FileSystemReportManager;
import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRValueParameter;
import net.sf.jasperreports.engine.JasperReportsContext;
import net.sf.jasperreports.engine.query.AbstractQueryExecuterFactory;
import net.sf.jasperreports.engine.query.JRQueryExecuter;
import org.springframework.beans.BeansException;
import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationContextAware;
import org.springframework.stereotype.Component;

import java.util.Map;

@Component
public class NXCLQueryExecutorFactory extends AbstractQueryExecuterFactory implements ApplicationContextAware {

    // TODO: temporary hack
    private static ApplicationContext applicationContext;

    public NXCLQueryExecutorFactory() {
    }

    @Override
    public Object[] getBuiltinParameters() {
        return new Object[0];
    }

    @Override
    public JRQueryExecuter createQueryExecuter(JasperReportsContext jasperReportsContext, JRDataset dataset, Map<String, ? extends JRValueParameter> parameters) throws JRException {
        System.out.println(getClass().getClassLoader());
        System.out.println("### " + parameters.get(FileSystemReportManager.SUBREPORT_DIR_KEY));
        NXCLQueryExecutor queryExecutor = new NXCLQueryExecutor(jasperReportsContext, dataset, parameters);
        queryExecutor.setSettings(applicationContext.getBean(ServerSettings.class));
        return queryExecutor;
    }

    @Override
    public boolean supportsQueryParameterType(String className) {
        return true;
    }

    @Override
    public void setApplicationContext(ApplicationContext applicationContext) throws BeansException {
        NXCLQueryExecutorFactory.applicationContext = applicationContext;
    }
}
