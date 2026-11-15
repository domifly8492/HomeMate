# HomeMate 智能家庭语音管家

HomeMate 是一个基于 ESP32-S3 的智能家庭语音管家项目，采用 ESP-IDF 开发框架，结合 FreeRTOS 多任务管理、LVGL 图形界面、Wi-Fi 网络通信及音频播放等技术，逐步实现家庭信息显示、天气查询、语音播报、备忘录和定时提醒等功能。

项目采用模块化设计，按照功能逐步开发，优先保证系统稳定性和代码可维护性。

---

## 一、项目基本信息

| 项目 | 配置 |
|---|---|
| 项目名称 | HomeMate |
| 项目类型 | 智能家庭语音管家 |
| 主控芯片 | ESP32-S3 N16R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| 开发框架 | ESP-IDF 5.5.6 |
| 开发工具 | VS Code |
| 操作系统 | FreeRTOS（ESP-IDF 内置） |
| 图形界面 | LVGL 9 |
| 显示屏 | 2.4 英寸 ST7789 SPI |
| 音频功放 | MAX98357A |
| 扬声器 | 4Ω / 3W |
| 存储扩展 | SPI MicroSD 模块（计划） |

## 二、项目功能规划

### V1.0：基础功能

- [ ] ST7789 显示屏驱动
- [ ] LVGL 9 图形界面
- [ ] 系统主界面设计
- [ ] Wi-Fi 连接
- [ ] 网络时间同步（SNTP）
- [ ] 天气信息获取与显示
- [ ] MAX98357A I2S 音频输出
- [ ] MicroSD 卡 WAV 音频播放
- [ ] NVS 本地备忘录
- [ ] 定时提醒功能

### V2.0：智能交互扩展

- [ ] 语音交互功能
- [ ] 在线语音合成（TTS）
- [ ] 微信小程序消息交互
- [ ] 云端数据通信
- [ ] 家庭设备状态管理

### V3.0：家庭感知扩展

- [ ] 毫米波人体存在检测
- [ ] 门磁状态检测
- [ ] 人员进出区域事件判断
- [ ] 家庭环境与设备联动

> 以上为开发规划，不代表所有功能已经实现。

## 三、硬件组成

| 模块 | 器件 | 用途 |
|---|---|---|
| 主控 | ESP32-S3 N16R8 | 系统控制与数据处理 |
| 显示 | ST7789 SPI 屏幕 | 图形界面显示 |
| 音频 | MAX98357A | I2S 数字音频功放 |
| 扬声器 | 4Ω / 3W | 音频输出 |
| 存储 | SPI MicroSD 模块 | 音频文件存储 |
| 网络 | ESP32-S3 Wi-Fi | 网络通信 |
| 人体检测 | HLK-LD2410C（计划） | 人体存在检测 |
| 门磁 | 磁性门磁模块（计划） | 门状态检测 |

注意：具体 GPIO 引脚分配以最终硬件接线和工程配置为准。

## 四、软件架构

项目采用 ESP-IDF + FreeRTOS 架构，按照功能划分为多个独立模块。

初步任务规划：

| 任务 | 主要职责 |
|---|---|
| UI Task | LVGL 界面刷新与交互 |
| Audio Task | 音频文件读取与播放 |
| Network Task | Wi-Fi、天气与网络通信 |
| Reminder Task | 定时提醒 |
| Storage Task | SD 卡及本地数据管理 |

### 设计原则

1. 各功能模块尽量独立，降低模块之间的耦合。
2. 使用 FreeRTOS 任务管理不同功能。
3. LVGL 界面操作集中在 UI Task 中，其他任务通过消息队列或事件通知 UI。
4. 音频播放与网络请求相互独立，避免阻塞界面。
5. 关键配置和本地数据使用 NVS 保存。
6. 逐步开发，每完成一个模块进行编译和功能测试。

## 五、开发环境

- Windows
- Visual Studio Code
- ESP-IDF 5.5.6
- CMake
- Ninja
- LVGL 9

## 六、工程目录

```text
HomeMate/
├── .devcontainer/
├── .vscode/
├── components/
│   ├── app_events/
│   └── board/
├── main/
│   ├── CMakeLists.txt
│   └── main.c
├── CMakeLists.txt
├── partitions.csv
├── .gitignore
└── README.md
```

后续将根据功能增加 display、audio、storage、network、weather 等模块。

## 七、开发进度

当前阶段：基础工程搭建。

- [x] 创建 ESP-IDF 工程
- [x] 建立基础目录结构
- [x] 建立 Git 本地版本管理
- [x] 创建 GitHub 私有仓库
- [x] 完成第一次代码提交与上传
- [ ] 完成基础工程编译验证
- [ ] 完成显示屏驱动
- [ ] 完成 LVGL 9 初始化

## 八、版本管理

项目使用 Git + GitHub 进行版本管理。

建议每完成一个独立功能，就进行一次提交：

- `Initial commit`：首次提交
- `Add display driver`：添加显示屏驱动
- `Add LVGL interface`：添加 LVGL 界面
- `Add WiFi module`：添加 Wi-Fi 模块
- `Add weather module`：添加天气模块

---

**HomeMate —— 让家庭设备更智能，让生活更简单。**