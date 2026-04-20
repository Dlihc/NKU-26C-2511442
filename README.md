# NKU-26C-MGS-tribute

一个使用 **Qt 6.11.0 + C++17** 编写的潜行关卡课程项目，灵感来自经典潜行游戏，但关卡、美术与代码均为课程大作业用途的原创实现。

## 已实现内容
- 网格地图与墙体阻挡
- 玩家移动与到达目标点过关
- 守卫巡逻路径
- 守卫前方扇形视野
- 墙体遮挡视线
- 纸箱躲藏机制
- 状态栏提示、步数统计
- 像素风图片资源与简单音效资源整合

## 操作
- `WASD` 或方向键：移动
- `Space`：在纸箱位置切换躲藏/离开躲藏
- `1-5`：开始菜单中选择关卡
- `Enter`：开始当前所选关卡
- `Esc`：返回选关菜单
- `R`：重开关卡

## 构建（Qt 6.11.0）
```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="你的Qt6路径"
cmake --build . --config Release
```

Windows 例子：
```bash
cmake .. -DCMAKE_PREFIX_PATH="D:/Qt/6.11.0/mingw_64"
cmake --build . --config Release
```

## 目录结构
- `include/`：头文件
- `src/`：C++ 源码
- `assets/`：图片与音频资源
- `resources.qrc`：Qt 资源文件

## 后续可扩展方向
1. 加入趴下、贴墙、投掷空弹壳诱敌。
2. 增加多关卡 JSON / TXT 地图读取。
3. 加入开始菜单、失败界面、结算界面。
4. 把守卫 AI 扩展为巡逻、搜索、回归三态有限状态机。
5. 用 sprite sheet 替换当前占位资源，提升演示效果。
