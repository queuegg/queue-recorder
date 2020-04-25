import { fork, ChildProcess } from "child_process";
import path from "path";

import { ScreenCapture, ScreenCaptureConfig } from "./screen-capture";

class PromiseWithResolvers {
  public resolve: any;
  public promise: any;
  public reject: any;

  constructor() {
    this.resolve = undefined;
    this.reject = undefined;
    this.promise = new Promise((resolve, reject) => {
      this.resolve = resolve;
      this.reject = reject;
    });
  }
}

/**
 * This class implements the ScreenCapture interface, but instead
 * of running in process will spawn a subprocess to handle the
 * recording. This is useful if the main process is already using
 * resources needed for screen capture (like DesktopDuplication).
 */
export class ScreenCaptureSubprocess implements ScreenCapture {
  private child: ChildProcess;
  private errorCallbacks: Array<(error: any) => void>;
  private startedPromise: PromiseWithResolvers | null = null;
  private stoppedPromise: PromiseWithResolvers | null = null;

  constructor(config: ScreenCaptureConfig) {
    this.errorCallbacks = [];
    this.child = fork(path.join(__dirname, "subprocess-entry"));
    this.child.on("message", this.handleChildMessage);
    this.child.on("error", error =>
      this.handleError(`Subprocess error: ${error.message}`)
    );
    this.child.send({ type: "config", config });
  }

  public start() {
    this.startedPromise = new PromiseWithResolvers();
    this.child.send({ type: "start" });
    return this.startedPromise.promise;
  }

  public async stop() {
    this.stoppedPromise = new PromiseWithResolvers();
    this.child.send({ type: "stop" });
    return this.stoppedPromise.promise;
  }

  public onError(callback: (error: any) => void) {
    this.errorCallbacks.push(callback);
  }

  private handleError = (errorMessage: any) => {
    for (const callback of this.errorCallbacks) {
      callback(errorMessage);
    }

    // Errors with outstanding start/stop promises must reject those
    // promises.
    if (this.startedPromise !== null) {
      this.startedPromise.reject(errorMessage);
    }

    if (this.stoppedPromise !== null) {
      this.stoppedPromise.reject(errorMessage);
    }
  };

  private handleChildMessage = (message: any) => {
    if (message.type === "error") {
      this.handleError(message.error);
    }

    if (message.type === "started" && this.startedPromise) {
      this.startedPromise.resolve();
      this.startedPromise = null;
    }

    if (message.type === "stopped" && this.stoppedPromise) {
      this.stoppedPromise.resolve(message.output);
      this.stoppedPromise = null;
    }
  };
}
