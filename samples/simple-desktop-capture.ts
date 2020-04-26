/**
 * This shows a minimal example, recording the entire desktop for
 * 5 seconds and then writing the results to a MP4 file.
 */

import { createScreenCapture } from "../src";
import { ScreenCaptureConfig } from "../src/screen-capture";

const CONFIG: ScreenCaptureConfig = {
  video: {
    source: {
      type: "desktop",
    },
  },
  output: {
    fileName: "output/test.mp4",
  },
};

const main = async () => {
  const capture = createScreenCapture(CONFIG);
  await capture.start();
  setTimeout(() => capture.stop(), 5000);
};

main();
