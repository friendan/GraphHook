# GraphHook

为了兼容32位程序注入

## 远程线程注入

注入：DllInjector32.exe --remote-thread --process-name="th10.exe" --inject-dll="GraphHook32.dll"

卸载：DllInjector32.exe --remote-thread --process-name="th10.exe" --uninject-dll="GraphHook32.dll"

## 消息钩子注入

DllInjector32.exe --windows-hook --class-name="BASE" --hook-dll="TH10Hook32.dll"
