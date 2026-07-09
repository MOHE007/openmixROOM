# OpenMix Room

**Open-source virtual monitoring over headphones — FDN room reverb + headphone calibration.**

OpenMix Room 是一款虚拟混音监听插件，通过耳机频率响应校准、Crossfeed 串扰处理、FDN 板式混响和实时可视化，在普通耳机上还原音箱混音房的听感。支持 VST3 / AU 格式（macOS）。

> **v0.1.0-beta** — FDN Room 混响模块首次发布

---

## 当前状态

**Phase 3 — FDN Room 混响 + 可视化 + 参数打磨**

信号链：`Input → Headphone Cal EQ → Crossfeed → FDN Room Reverb → Dry/Wet Mix → Output`

### 已实现功能

| 模块 | 说明 |
|------|------|
| **耳机校准 EQ** | 11 款耳机 AutoEq oratory1990 PEQ 数据（Harman over-ear 2018），每款 10 段参数 EQ + 预衰减 + 增益控制 |
| **FR 曲线图** | 实时频率响应曲线，256 点对数频谱 (20Hz–20kHz) |
| **Crossfeed 串扰** | Bauer / Meier / Chu Moy / HRTF 四种算法，可调截止频率 |
| **FDN Room 混响** | 8 通道 Feedback Delay Network + 随机化 Hadamard 反馈矩阵，Small / Medium / Large / Extra Large 四种预调模式，可调 Room Mix 干湿比 |
| **能量衰减可视化** | 实时 Room 模块能量衰减曲线 (EDC)，橙色全频带衰减 + 蓝色高频阻尼曲线，dB 刻度 + RT60 标注 |
| **Bypass 对比** | 一键旁通方便 A/B 对比 + Room ON/OFF 独立开关 |
| **干湿混合** | Room Mix、Crossfeed Mix、Total Mix 独立控制 |

### 支持的耳机校准 Profile

使用 [AutoEq](https://github.com/jaakkopasanen/AutoEq) / [oratory1990](https://www.reddit.com/r/oratory1990) 测量数据：

- Beyerdynamic DT 770 Pro / DT 880 / DT 990 Pro
- Sennheiser HD 600 / HD 650
- Audio-Technica ATH-M50x / ATH-M20x
- AKG K701 / K702
- Sony MDR-7506
- Shure SRH840

---

## Room 模块技术细节

采用 FDN（Feedback Delay Network）架构，参考 ValhallaVintageVerb 设计理念：

- **8 路延迟线** — 质数长度 (997–2711 samples @ 44.1kHz)，避免公约数导致的金属共振峰
- **随机化 Hadamard-8 反馈矩阵** — 启动时行/列随机符号翻转，打破标准 Hadamard 全 +1 行 DC 放大，保持酉矩阵能量守恒
- **一阶低通阻尼** — 反馈环内单极 LP 滤波器，模拟空气吸收的频率相关衰减
- **4 级 Schroeder 全通扩散器** — 黄金分割增益 g=0.618，最大扩散最小染色
- **预调模式** — Small (0.6s) / Medium (1.8s) / Large (3.5s) / Extra Large (6.0s)

---

## 路线图

| Phase | 内容 | 状态 |
|-------|------|------|
| 1 | 项目骨架、音频直通、基础 GUI | ✅ 完成 |
| 2 | HRTF 加载、Crossfeed、Room IR、耳机校准 | ✅ 完成 |
| 3 | FDN Room 混响、能量衰减可视化、参数自动化 | 🚧 开发中 |
| 4 | 嵌套全通扩散器、早期反射分离、Size 交叉淡入淡出 | ⬜ 待开发 |
| 5 | Windows 支持、头部追踪 (OSC / 摄像头) | ⬜ 待开发 |

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
├── juce/                              # JUCE 8 (submodule)
├── src/
│   ├── PluginProcessor.h / .cpp
│   ├── PluginEditor.h / .cpp
│   ├── dsp/
│   │   ├── SofaLoader.h / .cpp          # SOFA HRIR 加载
│   │   ├── CrossfeedProcessor.h / .cpp   # 串扰算法
│   │   ├── RoomProcessor.h / .cpp        # FDN 板式混响
│   │   └── HeadphoneCalibration.h / .cpp # 耳机校准 PEQ
│   └── ui/
│       ├── FrequencyResponseGraph.h / .cpp  # FR 曲线图
│       └── RoomResponseGraph.h / .cpp       # 能量衰减曲线图
└── resources/
```

---

## 依赖

- [JUCE 8](https://github.com/juce-framework/JUCE)
- [AutoEq](https://github.com/jaakkopasanen/AutoEq) — oratory1990 耳机测量数据

---

## License

待定
