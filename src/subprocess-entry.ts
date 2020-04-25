import { ScreenCapture, ScreenCaptureConfig } from "./screen-capture";
import { ScreenCaptureImpl } from "./screen-capture-impl";

let SCREEN_CAPTURE: ScreenCapture | null = null;

/** Send a message to the parent process. */
const sendMessage = (msg: any) => {
  if (process.send) {
    process.send(msg);
  }
};

/** Handle the config message. */
const handleConfig = async (config: ScreenCaptureConfig) => {
  SCREEN_CAPTURE = new ScreenCaptureImpl(config);
  SCREEN_CAPTURE.onError(handleError);
};

/** Handle the start message. */
const handleStart = async () => {
  if (!SCREEN_CAPTURE) {
    handleError("Screen Capture has not been created");
    return;
  }

  SCREEN_CAPTURE.start().then(
    () => sendMessage({ type: "started" }),
    e => {
      handleError(e.message);
      process.exit(-1);
    }
  );
};

/** Handles the stop message. Note this will cause the process to exit. */
const handleStop = async () => {
  if (!SCREEN_CAPTURE) {
    handleError("Screen Capture has not been created");
    return;
  }

  SCREEN_CAPTURE.stop().then(
    output => {
      sendMessage({ type: "stopped", output });
      process.exit(0);
    },
    () => {
      process.exit(-1);
    }
  );
};

/** Handles any errors and broadcasts to the parent. */
const handleError = async (error: any) => {
  if (process.send) {
    process.send({ type: "error", error });
  }
};

/** Install the basic IPC. */
process.on("message", message => {
  if (message.type === "config") {
    handleConfig(message.config);
  } else if (message.type === "start") {
    handleStart();
  } else if (SCREEN_CAPTURE) {
    handleStop();
  }
});
