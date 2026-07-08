---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: bf570042b9bab5ca445e13766da77718_b7b69fc37a7411f1b95a525400d9a7a1
    ReservedCode1: aYt04fQ7f3ExeYdmmjr//EzZQRc44kgXZ4OxlXerdpWZ0/Qa9CiFIQDtziDJKwUDGDSdVaug1eusTt5QyrEPY8w6JS1dYbMbHOUnFBR/QYBMRp35KjG+eTvxeIDdl0QYhF8jzKXp7o4EpV3j/nKo0UDKg2/Y75y3Q3DEEeE5d9ZRglvn8c6yFZ8oUng=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: bf570042b9bab5ca445e13766da77718_b7b69fc37a7411f1b95a525400d9a7a1
    ReservedCode2: aYt04fQ7f3ExeYdmmjr//EzZQRc44kgXZ4OxlXerdpWZ0/Qa9CiFIQDtziDJKwUDGDSdVaug1eusTt5QyrEPY8w6JS1dYbMbHOUnFBR/QYBMRp35KjG+eTvxeIDdl0QYhF8jzKXp7o4EpV3j/nKo0UDKg2/Y75y3Q3DEEeE5d9ZRglvn8c6yFZ8oUng=
---

# OpenMix Room 项目计划书

**日期：** 2026-07-08
**版本：** v1.0
**项目：** OpenMix Room — 开源虚拟混音监听插件

---

## 一、项目概述

模拟 Waves NX 的虚拟监听体验，通过 HRTF 空间化、Crossfeed 串扰和房间 IR 卷积，在普通耳机上还原音箱混音房的听感。目标平台：VST3 / AU（macOS），后续扩展 Windows。

---

##二、里程碑时间线

| 阶段 | 内容 | 预计工期 | 开始 | 截止 | 状态 |
|------|------|---------|------|------|------|
| Phase 1 | 项目骨架 + 音频直通 + GUI | 1 天 | 07-08 | 07-08 | ✅ |
| Phase 2 | HRTF 加载 + Crossfeed + 干湿混合 | 3-5 天 | 07-09 | 07-14 | ⬜ |
| Phase 3 | 房间 IR 卷积 + 早期反射 | 5-7 天 | 07-15 | 07-22 | ⬜ |
| Phase 4 | 头部追踪 (OSC/摄像头) | 5-7 天 | 07-23 | 07-30 | ⬜ |

---

## 三、Phase 2 详细任务拆解（07-09 ~ 07-14）

### 3.1 HRTF SOFA 文件加载
- [ ] 集成 libmysofa 依赖
- [ ] 实现 SOFA 文件选择器（内置/自定义）
- [ ] 解析 SOFA 文件中的 HRIR 数据（左耳+右耳）
- [ ] 内置一套通用 HRTF 数据集（如 MIT KEMAR / SADIE II）

### 3.2 Crossfeed 矩阵
- [ ] 实现标准 Crossfeed 算法（Bauer / Meier / Chu Moy）
- [ ] 支持可调 Cutoff Freq（100-2000Hz）
- [ ] 干/湿信号混合比控制
- [ ] GUI：旋钮 + 算法选择下拉菜单

### 3.3 干湿混合
- [ ] Mix 参数从 int 重构为 0.0-1.0 float
- [ ] 干信号直通 + 湿信号（Crossfeed 输出）混合
- [ ] 旁路切换（Bypass）

---

## 四、Phase 3 详细任务拆解（07-15 ~ 07-22）

### 4.1 房间 IR 卷积
- [ ] 实现实时卷积引擎（Partitioned Convolution）
- [ ] 加载用户提供的房间 IR WAV 文件
- [ ] 内置 Studio / Living Room / Hall 三种预设

### 4.2 早期反射模拟
- [ ] 实现早期反射延迟线网络
- [ ] 可调 Room Size / Damping / Pre-Delay

### 4.3 GUI 页面扩展
- [ ] Tab 页：Main / Room / Tracking
- [ ] 信号路径图动画更新

---

## 五、Phase 4 详细任务拆解（07-23 ~ 07-30）

### 5.1 头部追踪
- [ ] OSC 协议监听（接收手机陀螺仪数据）
- [ ] 可选：macOS 摄像头面部追踪（AVFoundation）
- [ ] 实时 HRTF 方位角更新

### 5.2 Windows 构建（如时间允许）
- [ ] CMake 配置 Windows 目标
- [ ] 预编译 libmysofa (mingw-w64)
- [ ] 测试 VST3 在 Windows DAW 中的兼容性

---

## 六、技术依赖清单

| 库 | 用途 | 引入阶段 |
|----|------|---------|
| JUCE 8 | 音频插件框架 | Phase 1 ✅ |
| libmysofa | SOFA HRTF 解析 | Phase 2 |
| 卷积引擎 (自研) | 房间 IR | Phase 3 |
| OSC 协议 | 头部追踪 | Phase 4 |

---

## 七、风险与对策

| 风险 | 可能性 | 影响 | 对策 |
|------|--------|------|------|
| HRTF 数据集版权不明 | 中 | 高 | 优先使用 MIT KEMAR / SADIE II 等开放数据集 |
| 实时卷积 CPU 占用高 | 高 | 中 | Partitioned Convolution + 低延迟模式可选 |
| 摄像头追踪跨平台困难 | 高 | 低 | macOS 优先，Windows 用 OSC 替代 |

---

## 八、版本发布计划

| 版本 | 内容 | 预计 |
|------|------|------|
| v0.2.0 | Phase 2 完成 | 07-14 |
| v0.3.0 | Phase 3 完成 | 07-22 |
| v0.4.0 | Phase 4 完成 | 07-30 |
| v1.0.0 | 全平台测试 + Bug 修复 | 08-07 |

---

##九、每日 Todo（2026-07-09 周三）

- [ ] 创建 libmysofa 集成分支
- [ ] 拉取预编译 libmysofa 或从源码构建
- [ ] 下载 MIT KEMAR SOFA 文件
- [ ] 实现 SOFA loader 类
- [ ] 单元测试：验证 HRIR 数据解析
*（内容由AI生成，仅供参考）*
