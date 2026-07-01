# 使用说明

这些是外部 RLE 加载方案需要替换或新增的文件。

- `rlepattern.h/.cpp`：新增文件。
- `gamewidget.h/.cpp`：替换项目里的同名文件，增加外部图案表。
- `mainwindow.h/.cpp`：替换项目里的同名文件，启动时扫描 `pattern` 文件夹。
- `CMakeLists.txt`：替换项目里的同名文件，构建后自动复制 `pattern` 文件夹。
- `pattern/*.rle`：新增外部图案文件。

程序启动后会扫描运行目录下的 `pattern` 文件夹，把 `.rle` 和 `.txt` 文件加入预设下拉框。
