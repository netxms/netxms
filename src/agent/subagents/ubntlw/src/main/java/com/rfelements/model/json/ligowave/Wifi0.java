package com.rfelements.model.json.ligowave;
/**
 * @author Pichanič Ján
 */
public class Wifi0 {

	private String name;

	private Integer bitrate;

	private String channelWidth;

	private Integer antennaGain;

	private String protocol;

	private Integer[] noise;

	private Boolean txPowerLimited;

	private Integer channel;

	private String radioMode;

	private Integer frequency;

	private Integer txPower;

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public Integer getBitrate() {
		return bitrate;
	}

	public void setBitrate(Integer bitrate) {
		this.bitrate = bitrate;
	}

	public String getChannelWidth() {
		return channelWidth;
	}

	public void setChannelWidth(String channelWidth) {
		this.channelWidth = channelWidth;
	}

	public Integer getAntennaGain() {
		return antennaGain;
	}

	public void setAntennaGain(Integer antennaGain) {
		this.antennaGain = antennaGain;
	}

	public String getProtocol() {
		return protocol;
	}

	public void setProtocol(String protocol) {
		this.protocol = protocol;
	}

	public Integer[] getNoise() {
		return noise;
	}

	public void setNoise(Integer[] noise) {
		this.noise = noise;
	}

	public Boolean getTxPowerLimited() {
		return txPowerLimited;
	}

	public void setTxPowerLimited(Boolean txPowerLimited) {
		this.txPowerLimited = txPowerLimited;
	}

	public Integer getChannel() {
		return channel;
	}

	public void setChannel(Integer channel) {
		this.channel = channel;
	}

	public String getRadioMode() {
		return radioMode;
	}

	public void setRadioMode(String radioMode) {
		this.radioMode = radioMode;
	}

	public Integer getFrequency() {
		return frequency;
	}

	public void setFrequency(Integer frequency) {
		this.frequency = frequency;
	}

	public Integer getTxPower() {
		return txPower;
	}

	public void setTxPower(Integer txPower) {
		this.txPower = txPower;
	}
}
