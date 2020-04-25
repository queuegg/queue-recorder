import { createScreenCapture } from ".";

import { ScreenCaptureConfig } from "./screen-capture";

const CONFIG: ScreenCaptureConfig = {
  video: {
    source: {
      type: "window"
    },
    captureCursor: true
    // source: {
    //   type: "window",
    //   windowTitle: "File Explorer"
    // }
  },
  audio: false,
  output: {
    fileName: "test.mp4"
  }
};

const main = async () => {
  const capture = createScreenCapture(CONFIG, true);
  await capture.start();
  setTimeout(
    () => capture.stop().catch(error => console.log("Got error", error)),
    5000
  );
};

main();
