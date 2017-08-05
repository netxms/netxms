package com.rfelements;

import java.util.regex.Pattern;
import org.netxms.agent.Action;
import org.netxms.agent.ListParameter;
import org.netxms.agent.Plugin;
import org.netxms.agent.PushParameter;
import org.netxms.agent.SubAgent;
import org.netxms.agent.TableParameter;
import org.netxms.bridge.Config;
import org.netxms.bridge.LogLevel;
import org.netxms.bridge.Platform;
import com.rfelements.exception.CollectorException;
import com.rfelements.model.json.ligowave.Ligowave;
import com.rfelements.model.json.ubiquiti.Ubiquiti;
import com.rfelements.provider.DataProvider;
import com.rfelements.provider.DataProviderImpl;

/**
 * @author Pichanič Ján
 */
abstract class AbstractCollector extends Plugin {

    private static final String VERSION = "2.1.1";

    private static final String IP_ADDRESS_PATTERN = "^([0" + "1]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
            "([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
            "([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
            "([01]?\\d\\d?|2[0-4]\\d|25[0-5])$";

    private static Pattern pattern;

    protected DataProvider dataProvider;

    protected Config config;

    protected String basePath;

    protected Protocol protocol;

    private DeviceType type;

    public AbstractCollector(Config config, DeviceType type) {
        super(config);
        this.type = type;
        if (pattern == null) {
            pattern = Pattern.compile(IP_ADDRESS_PATTERN);
        }
        if (this.dataProvider == null) {
            this.dataProvider = DataProviderImpl.getInstance();
            this.dataProvider.setConfig(config);
        }
    }

    @Override
    public String getVersion() {
        return VERSION;
    }

    protected Ligowave getDlbObject(String param) throws CollectorException {
        String ip = parseDeviceIdentifierParameter(param);
        return dataProvider.getLigowaveObject(protocol, basePath, ip, type);
    }

    protected Ubiquiti getUbntObject(String param) throws CollectorException {
        String ip = parseDeviceIdentifierParameter(param);
        return this.dataProvider.getUbiquitiObject(protocol, basePath, ip, type);
    }

    protected String parseDeviceIdentifierParameter(String param) throws CollectorException {
        String ip = SubAgent.getParameterArg(param, 1);
        if (!pattern.matcher(ip).matches()) {
           Platform.writeLog(LogLevel.ERROR,
                    "[" + this.getClass().getName() + "] [parseDeviceIdentifierParameter] IP passed as parameter does not match IP address pattern");
            throw new CollectorException("Parameter does not match IP address pattern !");
        }
        return ip;
    }

    @Override
    public void shutdown() {
        dataProvider.onShutdown(type);
        super.shutdown();
    }

    @Override
    public PushParameter[] getPushParameters() {
        return new PushParameter[0];
    }

    @Override
    public ListParameter[] getListParameters() {
        return new ListParameter[0];
    }

    @Override
    public TableParameter[] getTableParameters() {
        return new TableParameter[0];
    }

    @Override
    public Action[] getActions() {
        return new Action[0];
    }
}
