import { ScreenCapture, ScreenCaptureConfig } from "./screen-capture";
import { ScreenCaptureImpl } from "./screen-capture-impl";
import { ScreenCaptureSubprocess } from "./screen-capture-subprocess";
import { postProcessDirectory } from "./post-processing";

/** Creates a screen capture (optionally in a subprocess). */
const createScreenCapture = (
  config: ScreenCaptureConfig,
  subprocess: boolean = false
): ScreenCapture => {
  return subprocess
    ? new ScreenCaptureSubprocess(config)
    : new ScreenCaptureImpl(config);
};

export { createScreenCapture, postProcessDirectory };
