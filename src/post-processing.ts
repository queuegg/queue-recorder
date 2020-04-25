import fs from "fs";
import path from "path";

import ffmpegWrapper from "./ffmpeg-wrapper";

/**
 * Look for any temporary files that we may have created and run
 * post processing on them (this can happen if a recording is
 * interrupted).
 */
export const postProcessDirectory = async (dir: string): Promise<void> => {
  const fileNames = fs
    .readdirSync(dir)
    .filter(x => x.endsWith(".h264") || x.endsWith(".wav"));
  const prefixes = fileNames.reduce((a, fileName) => {
    const prefix = fileName.split(".")[0];
    if (a.indexOf(prefix) === -1) {
      return [...a, prefix];
    } else {
      return a;
    }
  }, [] as string[]);

  for (const prefix of prefixes) {
    try {
      await doPostProcessing(
        path.join(dir, `${prefix}.mp4`),
        fileNames
          .filter(x => x.startsWith(prefix))
          .map(fileName => {
            return path.join(dir, fileName);
          })
      );
    } catch (e) {
      // A single bad file should not stop us from processing the rest.
      console.log(e);
    }
  }
};

/** Post process temporary recording files through ffmpeg. */
export const doPostProcessing = async (
  ouptutFile: string,
  inputFiles: string[]
) => {
  await ffmpegWrapper.process(ouptutFile, inputFiles);
  // Clean up temp files.
  inputFiles.forEach(fileName => {
    if (fileName) {
      fs.unlinkSync(fileName);
    }
  });
};
