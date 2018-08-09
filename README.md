# GraphHook

为了兼容32位程序注入

## 远程线程注入

注入：DllInjector32.exe --remote-thread --inject-dll="TH10Hook32.dll" --process-name="th10.exe"

卸载：DllInjector32.exe --remote-thread --uninject-dll="TH10Hook32.dll" --process-name="th10.exe"

## 消息钩子注入

DllInjector32.exe --windows-hook --hook-dll="TH10Hook32.dll" --class-name="BASE"

## 问题

DLL注入时，风神录还是会小几率炸掉，原因未明。
