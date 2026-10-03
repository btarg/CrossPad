{
  "targets": [
    {
      "target_name": "virtual_x360",
      "sources": [ "src/addon.cc" ],
      "cflags_cc": [ "-std=c++17" ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1
        }
      }
    }
  ]
}
