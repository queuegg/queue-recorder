export interface ScreenCapture {
  start: () => Promise<void>; // Resolved when the pipeline has initialized.
  stop: () => Promise<string>; // Resolved when the final video is ready.
  onError: (callback: (err: string) => void) => void;
}

export interface WindowVideoSource {
  type: "window";
  windowTitle?: string; // If omitted use the currently focused window.
}

export interface DesktopVideoSource {
  type: "desktop";
  screenId?: number; // If ommitted use the default desktop.
}

export type VideoSource = WindowVideoSource | DesktopVideoSource;

export interface VideoCaptureConfig {
  // FPS of the output video (frames will be inserted if the capture rate can not keep up).
  frameRate?: number;
  // Default source is Desktop default capture.
  source?: VideoSource;
  // Whether to capture the cursor. Default = false
  captureCursor?: boolean;
}

export interface AudioSource {
  type: "render" | "capture";
}

export interface AudioCaptureConfig {
  sources?: AudioSource[];
}

export interface OutputConfig {
  fileName: string;
}

export interface ScreenCaptureConfig {
  // Define how the output should be sent out.
  output: OutputConfig;
  video?: false | VideoCaptureConfig;
  audio?: false | AudioCaptureConfig;
}
