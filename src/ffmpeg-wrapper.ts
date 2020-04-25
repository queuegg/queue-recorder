/**
 * This module contains some simple code to wrap FFMPEG (mainly using fluent
 * ffmpeg), as well as handling the cases where FFMPEG does not exist.
 */

import ffmpeg from "fluent-ffmpeg";
const ffmpegPath = require("@ffmpeg-installer/ffmpeg").path.replace(
  "app.asar",
  "app.asar.unpacked"
);

/**
 * Combine video and audio. Returns a promise that is resolved whet processing is finished.
 * @param output
 * @param videoInput
 * @param audioInput
 */
const process = (output: string, inputFiles: string[]): Promise<void> => {
  return new Promise((resolve, reject) => {
    ffmpeg.setFfmpegPath(ffmpegPath);
    let command = ffmpeg();
    for (const inputFile of inputFiles) {
      command = command.input(inputFile).videoCodec("copy");
    }

    const numAudioInputs = inputFiles.filter(x => x.endsWith(".wav")).length;
    if (numAudioInputs > 1) {
      command = command.complexFilter(`amix=inputs=${numAudioInputs}`);
    }

    command
      .output(output)
      .on("end", resolve)
      .on("error", reject)
      .run();
  });
};

export default { process };
