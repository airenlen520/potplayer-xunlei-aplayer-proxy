# potplayer-xunlei-aplayer-proxy

安装
将 APlayer.dll 放入迅雷安装目录下的 program\player\ 文件夹，最终路径为：

text
<迅雷安装目录>\program\player\APlayer.dll
替换前请先备份原版 APlayer.dll。

编译
在 MinGW-w64 / g++ 环境下执行：

bash
g++ -shared -o APlayer.dll proxy7.cpp exports.def -Wl,--kill-at -static-libgcc -static-libstdc++ -O2
日志
日志文件位于：

text
<迅雷安装目录>\program\player\APlayerProxy.log
也可简写为 player\APlayerProxy.log。

默认识别的 PotPlayer 路径
默认情况下，APlayer.dll 仅能识别以下四个位置中的 PotPlayer 主程序 PotPlayerMini64.exe：

text
C:\Program Files\DAUM\PotPlayer\PotPlayerMini64.exe
C:\Program Files (x86)\DAUM\PotPlayer\PotPlayerMini64.exe
D:\Program Files\DAUM\PotPlayer\PotPlayerMini64.exe
D:\Program Files (x86)\DAUM\PotPlayer\PotPlayerMini64.exe
如果 PotPlayer 安装在其他位置，需要修改源码中的路径并重新编译。

兼容性
已测试迅雷版本：25.1.10.16.15。

原理说明
理论上，只要播放器支持 HTTP 链接播放，就可以适配不同播放器，因此本方案可能不仅限于 PotPlayer。

我观察到，迅雷的边下边播由 DownloadSDKServer.exe 提供视频元数据支持，例如：

text
http://127.0.0.1:9080/...
本代理 DLL 的核心逻辑是提取该 URL，并将其转交给 PotPlayer 直接播放。由于缺少迅雷原版 APlayer 的相关数据可能会触发报错，因此代理中填充了一些模拟/占位数据，以保证程序能够正常运行。

注意事项
替换前请备份原版 APlayer.dll。

本项目不包含迅雷原版 APlayer.dll。
仅供学习研究，请遵守相关软件许可与法律法规。
AI辅助分析的最终产物

