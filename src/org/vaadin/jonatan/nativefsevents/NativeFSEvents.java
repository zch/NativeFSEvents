package org.vaadin.jonatan.nativefsevents;

import java.util.HashMap;
import java.util.Map.Entry;

public class NativeFSEvents {
	private final String path;
	private NativeFSEventListener listener;

	private static native void monitor(String path);
	private static native void unmonitor(String path);

	private static HashMap<String, NativeFSEventListener> pathToListener = new HashMap<String, NativeFSEventListener>();
	
	static {
		System.loadLibrary("nativefsevents");
	}
	
	public NativeFSEvents(String path, NativeFSEventListener listener) {
		this.path = path;
		this.listener = listener;
	}
	
	public void startMonitoring() {
		monitor(path);
		pathToListener.put(path, listener);
	}
	
	public void stopMonitoring() {
		unmonitor(path);
		pathToListener.remove(path);
	}
	
	public static void eventCallback(String path) {
		if (path == null) {
			return;
		}

		for (Entry<String, NativeFSEventListener> entry : pathToListener.entrySet()) {
			if (path.startsWith(entry.getKey())) {
				entry.getValue().pathModified(path);
			}
		}
	}
	
	public interface NativeFSEventListener {
		public void pathModified(String path);
	}
}
