package com.rfelements.model.json.ubiquiti;
/**
 * @author Pichanič Ján
 */
public class Stats {

	private Integer rx_nwids;

	private Integer rx_crypts;

	private Integer rx_frags;

	private Integer rx_retries;

	private Integer missed_beacons;

	private Integer err_other;

	public Integer getRx_nwids() {
		return rx_nwids;
	}

	public void setRx_nwids(Integer rx_nwids) {
		this.rx_nwids = rx_nwids;
	}

	public Integer getRx_crypts() {
		return rx_crypts;
	}

	public void setRx_crypts(Integer rx_crypts) {
		this.rx_crypts = rx_crypts;
	}

	public Integer getRx_frags() {
		return rx_frags;
	}

	public void setRx_frags(Integer rx_frags) {
		this.rx_frags = rx_frags;
	}

	public Integer getRx_retries() {
		return rx_retries;
	}

	public void setRx_retries(Integer rx_retries) {
		this.rx_retries = rx_retries;
	}

	public Integer getMissed_beacons() {
		return missed_beacons;
	}

	public void setMissed_beacons(Integer missed_beacons) {
		this.missed_beacons = missed_beacons;
	}

	public Integer getErr_other() {
		return err_other;
	}

	public void setErr_other(Integer err_other) {
		this.err_other = err_other;
	}
}
