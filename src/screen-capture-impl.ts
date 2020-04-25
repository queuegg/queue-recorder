const ScreenCaptureNative = require("../build/Release/screen-capture-native");

import { ScreenCapture, ScreenCaptureConfig } from "./screen-capture";
import { doPostProcessing } from "./post-processing";

/** Simple interface for the native Pipeline class. */
export interface Pipeline {
  initialize: () => void;
  start: () => void;
  pause: () => void;
  resume: () => void;
  stop: () => void;
  pollErrors: () => string[];
  supportsStage: (str: string) => boolean;
}

/** Possible pipeline types. */
export enum PipelineType {
  VIDEO,
  AUDIO
}

/**
 * Some of the resources (specifically files) used by the pipeline
 * may not be released immediatly. To compensate for this we introduce
 * a small delay between finishing processing and doing the
 * post-processing.
 */
const POST_PROCESSING_DELAY = 1000;

/**
 * Valid state transitions:
 *
 * UNSTARTED -> CAPTURING (start)
 * CAPTURING -> STARTED (pause)
 * STARTED -> CAPTURING (resume)
 * STARTED,CAPTURING -> STOPPED (stop)
 */
export enum CaptureState {
  UNSTARTED,
  STARTED,
  CAPTURING,
  STOPPED
}

/** The main screen capture */
export class ScreenCaptureImpl implements ScreenCapture {
  private config: ScreenCaptureConfig;
  private pipelines: Pipeline[];
  private state: CaptureState;
  private outputFiles: string[];
  private errorCallbacks: Array<(err: string) => void>;
  constructor(config: ScreenCaptureConfig) {
    this.config = config;
    this.state = CaptureState.UNSTARTED;
    this.pipelines = [];
    this.errorCallbacks = [];
    this.outputFiles = [];
  }
  public async start() {
    this.ensureState([CaptureState.UNSTARTED]);
    if (this.config.video !== false) {
      const fileName = `${this.config.output.fileName}.h264`;
      this.outputFiles.push(fileName);
      this.createPipeline(PipelineType.VIDEO, {
        video: { ...this.config.video },
        output: { fileName }
      });
    }
    if (this.config.audio !== false) {
      const sources =
        this.config.audio && this.config.audio.sources
          ? this.config.audio.sources
          : [{ type: "render" }];
      sources.forEach(source => {
        const fileName = `${this.config.output.fileName}.${source.type}.wav`;
        this.outputFiles.push(fileName);
        this.createPipeline(PipelineType.AUDIO, {
          audio: { source },
          output: { fileName }
        });
      });
    }
    try {
      this.pipelines.forEach(p => p.initialize());
      this.pipelines.forEach(p => p.start());
    } catch (e) {
      // Publish to any listeners, but also rethrow
      this.handleError(e.toString());
      // Clean up any pipelines which have already acquired resources.
      this.pipelines.forEach(p => p.stop());
      // Throw so higher callers know that we had a problem.
      return Promise.reject(e);
    }
    this.state = CaptureState.CAPTURING;
  }

  /** Pauses a started screen capture. */
  public pause() {
    this.ensureState([CaptureState.CAPTURING]);
    this.pollErrors();
    this.pipelines.forEach(p => p.pause());
    this.state = CaptureState.STARTED;
  }

  /** Resumes a paused screen capture. */
  public resume() {
    this.ensureState([CaptureState.STARTED]);
    this.pollErrors();
    this.pipelines.forEach(p => p.resume());
    this.state = CaptureState.CAPTURING;
  }

  /** Stops a screen capture. */
  public async stop() {
    this.ensureState([CaptureState.STARTED, CaptureState.CAPTURING]);
    this.pollErrors();
    this.state = CaptureState.STOPPED;
    this.pipelines.forEach(p => p.stop());
    await new Promise(resolve => setTimeout(resolve, POST_PROCESSING_DELAY));
    try {
      await this.doPostProcessing();
      return this.config.output.fileName;
    } catch (e) {
      this.handleError(e.toString());
      throw e;
    }
  }

  /** Installs an error handler. */
  public onError(callback: (err: string) => void) {
    this.errorCallbacks.push(callback);
  }

  /** Creates either an audio or video pipeline. */
  private createPipeline(pipelineType: PipelineType, config: any) {
    const VIDEO_STAGES = [
      // Note that the capture stage is specified below, based on the config.
      ["NVENC", "AMF"],
      "FILE_WRITER"
    ];
    const AUDIO_STAGES = ["WASAPI", "WAV_WRITER"];
    const pipeline = new ScreenCaptureNative.Pipeline(config);
    let stages: Array<string | string[]> = [];
    switch (pipelineType) {
      case PipelineType.AUDIO:
        stages = AUDIO_STAGES;
        break;
      case PipelineType.VIDEO:
        stages = VIDEO_STAGES;
        // Determine which stage we should use based on the source.
        stages.unshift(
          config &&
            config.video &&
            config.video.source &&
            config.video.source.type == "window"
            ? "GDI_CAPTURE"
            : "DESKTOP_DUPLICATION"
        );
        break;
    }
    stages.forEach(stage => {
      if (Array.isArray(stage)) {
        let added = false;
        for (const s of stage) {
          if (pipeline.supportsStage(s)) {
            pipeline.addStage(s);
            added = true;
            break;
          }
        }
        if (!added) {
          throw new Error("Could not find suitable stage from " + stage);
        }
      } else {
        pipeline.addStage(stage);
      }
    });
    this.pipelines.push(pipeline);
  }

  /** Run the captured files through ffmpeg to do muxing/minor transcoding. */
  private async doPostProcessing() {
    return doPostProcessing(this.config.output.fileName, this.outputFiles);
  }

  /** Ensure the state is what we expect it to be. */
  private ensureState(states: CaptureState[]) {
    if (states.indexOf(this.state) === -1) {
      throw new Error(
        `Expected capture state ${this.state} to be in ${states}`
      );
    }
  }

  /**
   * Checks for any errors.
   * This will check all of the pipelines for any errors in the processing
   * thread. Errors in the main threads should be communicated syncronously.
   */
  private pollErrors() {
    this.pipelines.forEach(pipeline => {
      const errors = pipeline.pollErrors();
      if (errors && errors.length) {
        for (const error of errors) {
          this.handleError(error);
        }
      }
    });
  }

  /**
   * @param error Error string.
   */
  private handleError(error: string) {
    for (const callback of this.errorCallbacks) {
      callback(error);
    }
  }
}
