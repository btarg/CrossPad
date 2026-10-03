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
        ],
        [
          "OS=='win'",
          {
            "libraries": [ "Shell32.lib" ]
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
