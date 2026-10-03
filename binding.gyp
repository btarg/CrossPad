{
  "targets": [
    {
      "target_name": "virtual_x360",
      "sources": [ "src/addon.cc" ],
      "cflags_cc": [ "-std=c++17" ],
      "conditions": [
        [
          "OS!='win'",
          {
            "cflags_cc": [ "-fexceptions" ]
          }
        ]
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1
        }
      }
    }
  ]
}
