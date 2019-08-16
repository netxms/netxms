package com.radensolutions.reporting.custom;

import java.io.IOException;
import java.util.Map;
import net.sf.jasperreports.engine.JRDataSource;
import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRValueParameter;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class NXCLDataSource implements JRDataSource {

    private static final Logger log = LoggerFactory.getLogger(NXCLDataSource.class);
    
    private static final int[] PROTOCOL_COMPONENTS = { ProtocolVersion.INDEX_FULL };

    protected JRDataset dataset;
    protected Map<String, ? extends JRValueParameter> parameters;

    protected NXCLDataSource(JRDataset dataset, Map<String, ? extends JRValueParameter> parameters) {
        this.dataset = dataset;
        this.parameters = parameters;
    }

    public void connect(String server, String login, String password) {
        log.debug("Connecting to NetXMS server at " + server + " as " + login);
        NXCSession session = new NXCSession(server);
        try {
            session.connect(PROTOCOL_COMPONENTS);
            session.login(login, password);
            session.syncEventTemplates();
            session.syncObjects();
            loadData(session);
        } catch (Exception e) {
            log.error("Cannot connect to NetXMS", e);
        }
    }

    protected Object getParameterValue(String name) {
        return parameters == null ? null : parameters.get(name).getValue();
    }

    public abstract void loadData(NXCSession session) throws IOException, NXCException;

    public abstract void setQuery(String query);
}
