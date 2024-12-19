## Cooltify

Lean, cool looking osd for displaying one-line notifications. Useful for critical alerts.
```console
$ echo -n "🪫 battery low at 2%" | cooltify
```
Requires wlr-layer-shell protocol i.e. wlroots based compositors. 

![image](https://github.com/user-attachments/assets/0a17e024-72e9-496b-8710-e95f38c8d685)

This is very early staged. There's better handling of timeout to be added as currently, the timeout is dependant on the refresh rate of the screen. In addition to some config file or flags specifying some options.

Part of the [MARBS desktop environment](https://marbs.kuchta.dev/).
