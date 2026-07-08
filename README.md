# OpenMix Room

**Open-source Waves NX alternative — virtual monitoring over headphones.**

OpenMix Room 是一款虚拟混音监听插件，通过 HRTF 空间化渲染、Crossfeed 串扰处理、房间脉冲响应卷积和耳机频率响应校准，在普通耳机上还原音箱混音房的听感。支持 VST3 / AU 格式（macOS）。

---

## 当前状态

**Phase 2+ — 全功能 DSP 链路 + Headphone Lab 风格 UI**

信号链：`Input → Headphone Cal EQ → Crossfeed → Room IR → Dry/Wet Mix → Output`

### 已实现功能

| 模块 | 说明 |
|------|------|
| **HRTF 双耳渲染** | SOFA 文件加载，±30° 虚拟扬声器定位 |
| **Crossfeed 串扰** | Bauer / Meier / Chu Moy / HRTF 四种算法，可调截止频率 |
| **房间 IR 卷积** | Small / Medium / Large 三种合成房间脉冲响应 |
| **耳机校准 EQ** | 11 款耳机 AutoEq oratory1990 PEQ 数据（Harman over-ear 2018），每款 10 段参数 EQ + 预衰减 |
| **FR 曲线图** | 实时显示当前耳机校准的频率响应曲线 |
| **Bypass 对比** | 一键旁通方便 A/B 对比 |
| **干湿混合** | Room Mix、Crossfeed、Total Mix 独立控制 |

### 支持的耳机校准 Profile

使用 [AutoEq](https://github.com/jaakkopasanen/AutoEq) / [oratory1990](https://www.reddit.com/r/oratory1990) 测量数据：

- Beyerdynamic DT 770 Pro / DT 880 / DT 990 Pro
- Sennheiser HD 600 / HD 650
- Audio-Technica ATH-M50x / ATH-M20x
- AKG K701 / K702
- Sony MDR-7506
- Shure SRH840

---

## 路线图

| Phase | 内容 | 状态 |
|-------|------|------|
| 1 | 项目骨架、音频直通、基础 GUI | ✅ 完成 |
| 2 | HRTF 加载、Crossfeed、Room IR、耳机校准 | ✅ 完成 |
| 3 | UI 打磨、参数自动化、Windows 支持 | ⬜ 待开发 |
| 4 | 头部追踪 (OSC / 摄像头) | ⬜ 待开发 |

---

## 构建

### 环境要求

- **JUCE 8** — git submodule
- **CMake 3.22+**
- **Xcode Command Line Tools**（macOS）
- **C++17**
- **macOS 10.15+**

### 步骤

```bash
git clone --recurse-submodules https://github.com/MOHE007/openmixROOM.git
cd openmixROOM
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target OpenMixRoom_VST3
cmake --build . --target OpenMixRoom_AU
```

产物位置：`build/OpenMixRoom_artefacts/`

---

## 项目结构

```
openmixROOM/
├── CMakeLists.txt
├── README.md
├── juce/                         # JUCE 8 (submodule)
├── src/
│   ├── PluginProcessor.h / .cpp
│   ├── PluginEditor.h / .cpp
│   ├── dsp/
│   │   ├── SofaLoader.h / .cpp          # SOFA HRIR 加载
│   │   ├── CrossfeedProcessor.h / .cpp   # 串扰算法
│   │   ├── RoomProcessor.h / .cpp        # 房间 IR 卷积
│   │   └── HeadphoneCalibration.h / .cpp # 耳机校准 PEQ
│   ├── ui/
│   │   └── FrequencyResponseGraph.h / .cpp
│   └── utils/
└── resources/
```

---

## 依赖

- [JUCE 8](https://github.com/juce-framework/JUCE)
- [AutoEq](https://github.com/jaakkopasanen/AutoEq) — oratory1990 耳机测量数据

---

## License

待定
