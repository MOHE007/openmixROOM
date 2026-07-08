# OpenMix Room

**Open-source Waves NX alternative — virtual monitoring over headphones.**

OpenMix Room 是一款虚拟混音监听插件，通过 HRTF 空间化渲染、Crossfeed 串扰处理和房间脉冲响应卷积，在普通耳机上还原音箱混音房的听感。支持 VST3 / AU 格式（macOS），Windows 支持计划中。

---

## 当前状态

**Phase 1 — 音频直通 + GUI 框架（已完成）**  
插件可在 DAW 中加载，音频直通输出，深色主题 GUI 含信号路径图和 Mix 旋钮。

---

## 路线图

| Phase | 内容 | 状态 |
|-------|------|------|
| 1 | 项目骨架、音频直通、基础 GUI | ✅ 完成 |
| 2 | HRTF 加载 (SOFA)、Crossfeed 矩阵、干湿混合 | ⬜ 待开发 |
| 3 | 房间 IR 卷积、早期反射 | ⬜ 待开发 |
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
```

构建 VST3 + AU：

```bash
cmake --build . --target OpenMixRoom_AU
```

产物位置：`build/OpenMixRoom_artefacts/`

---

## 项目结构

```
openmixROOM/
├── CMakeLists.txt
├── README.md
├── juce/               # JUCE 8 (submodule)
├── src/
│   ├── PluginProcessor.h / .cpp
│   ├── PluginEditor.h / .cpp
│   ├── dsp/
│   ├── ui/
│   └── utils/
└── resources/
```

---

## 依赖

- [JUCE 8](https://github.com/juce-framework/JUCE)

---

## License

待定
