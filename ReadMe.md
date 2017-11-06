# Constant Process Priority
Constantly updates one or more processes' priority level.

## My purpose
I've made it so Unreal Engine's ShaderCompileWorker.exe several instances would have a High priority all the time, but you may find another use to it. It's very basic but does what I want it to.

## How to use
Processes' names must be a comma-separated list without spaces (except the spaces in the name of the process).

I've created a shortcut to it with command-line arguments and pinned it to Start. Once opened, you can set your settings and minimize it. It'll minimize to tray.

Everytime you configure it, be it through command-line or by manually setting it up and pushing **Update Settings**, it stores the settings in a json file that it automatically loads on start (unless there are command-line arguments). That makes it possible for you to edit the settings outside the app or keep several settings files. The loaded one will always be **ConstantProcessPrioritySettings.json**.

### Command-Line usage
```ConstantProcessPriority.exe -set ShaderCompileWorker.exe,Steam.exe 2```

Priority level is set by a number. Possible values are:

 - 0 = Below Normal priority
 - 1 = Normal priority
 - 2 = Above Normal priority
 - 3 = High priority