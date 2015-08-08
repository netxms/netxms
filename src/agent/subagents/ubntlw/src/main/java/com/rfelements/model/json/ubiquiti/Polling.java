package com.rfelements.model.json.ubiquiti;
/**
 * @author Pichanič Ján
 */
public class Polling {

	private Boolean enabled;

	private Integer quality;

	private Integer capacity;

	private Integer priority;

	private Integer noack;

	private Integer airsync_mode;

	private Integer airsync_connections;

	private Integer airsync_down_util;

	private Integer airsync_up_util;

	private Integer airselect;

	private Integer airselect_interval;

	public Boolean getEnabled() {
		return enabled;
	}

	public void setEnabled(Boolean enabled) {
		this.enabled = enabled;
	}

	public Integer getQuality() {
		return quality;
	}

	public void setQuality(Integer quality) {
		this.quality = quality;
	}

	public Integer getCapacity() {
		return capacity;
	}

	public void setCapacity(Integer capacity) {
		this.capacity = capacity;
	}

	public Integer getPriority() {
		return priority;
	}

	public void setPriority(Integer priority) {
		this.priority = priority;
	}

	public Integer getNoack() {
		return noack;
	}

	public void setNoack(Integer noack) {
		this.noack = noack;
	}

	public Integer getAirsync_mode() {
		return airsync_mode;
	}

	public void setAirsync_mode(Integer airsync_mode) {
		this.airsync_mode = airsync_mode;
	}

	public Integer getAirsync_connections() {
		return airsync_connections;
	}

	public void setAirsync_connections(Integer airsync_connections) {
		this.airsync_connections = airsync_connections;
	}

	public Integer getAirsync_down_util() {
		return airsync_down_util;
	}

	public void setAirsync_down_util(Integer airsync_down_util) {
		this.airsync_down_util = airsync_down_util;
	}

	public Integer getAirsync_up_util() {
		return airsync_up_util;
	}

	public void setAirsync_up_util(Integer airsync_up_util) {
		this.airsync_up_util = airsync_up_util;
	}

	public Integer getAirselect() {
		return airselect;
	}

	public void setAirselect(Integer airselect) {
		this.airselect = airselect;
	}

	public Integer getAirselect_interval() {
		return airselect_interval;
	}

	public void setAirselect_interval(Integer airselect_interval) {
		this.airselect_interval = airselect_interval;
	}

}
