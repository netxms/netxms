package com.rfelements;
import com.rfelements.model.json.ligowave.Ligowave;
import org.netxms.agent.Config;
import org.netxms.agent.Parameter;
import org.netxms.agent.ParameterType;
/**
 * @author Pichanič Ján
 */
public class LigowaveAp extends AbstractCollector {

	public LigowaveAp(Config config) {
		super(config, DeviceType.LIGOWAVE_AP);
		super.basicPath = "/ligowave-ap/";
		super.protocol = Protocol.HTTP;
		super.config = config;
	}

	@Override
	public String getName() {
		return "ligowave-ap";
	}

	@Override
	public String getVersion() {
		return VERSION;
	}

	@Override
	public void init(Config config) {
		//		super.config = config;
	}

	@Override
	public void shutdown() {
		super.shutdown();
	}

	@Override
	public Parameter[] getParameters() {
		return new Parameter[]{new Parameter() {

			@Override
			public String getName() {
				return "/ligowave-ap/wireless/channel_width(*)";
			}

			@Override
			public String getDescription() {
				return "Wireless channel width";
			}

			@Override
			public ParameterType getType() {
				return ParameterType.INT;
			}

			@Override
			public String getValue(String param) throws Exception {
				Ligowave dlb;
				dlb = getDlbObject(param);
				return dlb.getWirelessInformation().getRadios().getDev().getWifi0().getChannelWidth();
			}
		}, new Parameter() {

			@Override
			public String getName() {
				return "/ligowave-ap/wireless/frequency(*)";
			}

			@Override
			public String getDescription() {
				return "Wireless frequency";
			}

			@Override
			public ParameterType getType() {
				return ParameterType.INT;
			}

			@Override
			public String getValue(String param) throws Exception {
				Ligowave dlb;
				dlb = getDlbObject(param);
				return String.valueOf(dlb.getWirelessInformation().getRadios().getDev().getWifi0().getFrequency());
			}
		}, new Parameter() {

			@Override
			public String getName() {
				return "/ligowave-ap/wireless/peer_count(*)";
			}

			@Override
			public String getDescription() {
				return "Wireless peer count";
			}

			@Override
			public ParameterType getType() {
				return ParameterType.INT;
			}

			@Override
			public String getValue(String param) throws Exception {
				Ligowave dlb;
				dlb = getDlbObject(param);
				return String.valueOf(dlb.getWirelessInformation().getVaps()[0].getPeerCount());
			}
		}, new Parameter() {

			@Override
			public String getName() {
				return "/ligowave-ap/wireless/noise_floor(*)";
			}

			@Override
			public String getDescription() {
				return "Wireless noise floor";
			}

			@Override
			public ParameterType getType() {
				return ParameterType.INT;
			}

			@Override
			public String getValue(String param) throws Exception {
				Ligowave dlb;
				dlb = getDlbObject(param);
				Integer[] noise = dlb.getWirelessInformation().getRadios().getDev().getWifi0().getNoise();
				return String.valueOf(noise[0]);
			}
		}};
	}
}
