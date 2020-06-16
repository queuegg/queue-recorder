# Queue-recorder

A high performance recorder, specifically targeting recording of video games.

## Requirements

Since this module is mostly written in C++ you must have the NPM tools to build native extensions. This can be handled with the command `npm install --global --production windows-build-tools`.

## Building

There are two main components that must be built:

1. Compile typescript to javascript. This can be done with `npm run build:javascript`
2. Compile native extensions. This can be done with `npm run build:native`

Both of these will be run whenever you run `npm run build`

## Running
Included is the `src/samples` directory are some examples of how to use the recorder. These can either be compiled or run directly with ts-node.

### Options
Full documentation for the options can be found in `src/screen-capture.ts`, but here are the options you may care the most about:

- `output`: Must be an object with the following fields:
    - `filename`: The filename where we should write the output. Should end in .mp4
- `video`: Can contain either a video config with the following keys or be set to "false" to indicate that you do not want to capture video.
    - `frameRate`: Optional number of frames to capture per second. Default is 30.
    - `captureCursor`: Whether to capture the cursor. Default is false.
    - `source`: Describe the source to capture from
        - `type`: Can be either 'window' or 'desktop'.
        - `screenId`: If the type is desktop, this specifies which destkop to capture. Numbers increment from 0.
        - `windowTitle`: If you want to capture a specific window, you must specify the title here. If this is omitted it will capture the focused window.
- `audio`: How to capture audio. It can either specify `sources` or be set to false to capture no audio.
    - `sources`: A list of audio sources to capture. All of these will be mixed into a single audio stream.
        - `type`: The audio source type. Must be either "render" (what is coming out of the speakres) or "capture" (what is recorded by the microphone)