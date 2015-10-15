package com.rfelements;
import com.rfelements.exception.CollectorException;
import com.rfelements.model.json.ligowave.Ligowave;
import com.rfelements.model.json.ubiquiti.Ubiquiti;
import com.rfelements.provider.DataProvider;
import com.rfelements.provider.DataProviderImpl;
import org.netxms.agent.*;

import java.util.regex.Pattern;
/**
 * @author Pichanič Ján
 */
public abstract class AbstractCollector extends Plugin {

	public static final String VERSION = "1.0";

	private static final String IP_ADDRESS_PATTERN = "^([0" + "1]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
			"([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
			"([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
			"([01]?\\d\\d?|2[0-4]\\d|25[0-5])$";

	private static Pattern pattern;

	protected DataProvider dataProvider;

	protected Config config;

	protected String basicPath;

	protected String protocol;

	private DeviceType type;

	public AbstractCollector(Config config, DeviceType type) {
		super(config);
		this.type = type;
		if (pattern == null)
			pattern = Pattern.compile(IP_ADDRESS_PATTERN);
		if (this.dataProvider == null) {
			this.dataProvider = DataProviderImpl.getInstance();
			this.dataProvider.setConfig(config);
		}
	}

	protected Ligowave getDlbObject(String param) throws CollectorException {
		String ip = parseDeviceIdentifierParameter(param);
		return this.dataProvider.getLigowaveObject(protocol, basicPath, ip, type);
	}

	protected Ubiquiti getUbntObject(String param) throws CollectorException {
		String ip = parseDeviceIdentifierParameter(param);
		return this.dataProvider.getUbiquitiObject(protocol, basicPath, ip, type);
	}

	protected String parseDeviceIdentifierParameter(String param) throws CollectorException {
		String[] p = param.split("\\(");
		String ip = p[1].substring(0, p[1].length() - 1).trim();
		if (ip.contains("\"")) {
			if (ip.charAt(0) == '"') {
				ip = ip.substring(1);
			}
			if (ip.charAt(ip.length() - 1) == '"') {
				ip = ip.substring(0, ip.length() - 2);
			}
		}
		if (!pattern.matcher(ip).matches()) {
			SubAgent.writeLog(SubAgent.LogLevel.ERROR,
					"[" + this.getClass().getName() + "] [parseDeviceIdentifierParameter] IP passed as parameter does not match IP address pattern");
			throw new CollectorException("Parameter does not match IP address pattern !");
		}
		return ip;
	}

	@Override
	public void shutdown() {
		this.dataProvider.onShutdown(this.type);
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
