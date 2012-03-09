package org.vaadin.jonatan.nativefsevents;

import java.util.HashMap;
import java.util.Map.Entry;

public class NativeFSEvents {
	private final String path;
	private long monitorId;
	private NativeFSEventListener listener;

	private static native long monitor(String path);
	private static native void unmonitor(long monitorId);

	private static HashMap<String, Long> pathToId = new HashMap<String, Long>();
	private static HashMap<Long, NativeFSEventListener> idToListener = new HashMap<Long, NativeFSEventListener>();
	
	static {
		System.loadLibrary("nativefsevents");
	}
	
	public NativeFSEvents(String path, NativeFSEventListener listener) {
		this.path = path;
		this.listener = listener;
	}
	
	public void startMonitoring() {
		monitorId = monitor(path);
		pathToId.put(path, monitorId);
		idToListener.put(monitorId, listener);
	}
	
	public void stopMonitoring() {
		unmonitor(monitorId);
		String path = null;
		for (String p : pathToId.keySet()) {
			if (pathToId.get(p) == monitorId) {
				path = p;
				break;
			}
		}
		pathToId.remove(path);
		idToListener.remove(monitorId);
	}
	
	public static void eventCallback(String path) {
		if (path == null) {
			return;
		}
		for (Entry<String, Long> entry : pathToId.entrySet()) {
			if (path.startsWith(entry.getKey()) && idToListener.get(entry.getValue()) != null) {
				idToListener.get(entry.getValue()).pathModified(path);
			}
		}
	}
	
	public interface NativeFSEventListener {
		public void pathModified(String path);
	}
}
