{
  "targets": [
    {
      "target_name": "screen-capture-native",
      "sources": [
        "src/native/nvenc/NvEncoder.cpp",
        "src/native/nvenc/NvEncoderD3D11.cpp",
        "src/native/main.cpp",
        "src/native/pipeline.cpp",
        "src/native/stages/*.cpp",
        "src/native/stages/common/*.cpp",
        "src/native/amf/public/common/**/*.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "src/native/amf"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "defines": ["NAPI_CPP_EXCEPTIONS"]
    }
  ]
}