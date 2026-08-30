---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: bf570042b9bab5ca445e13766da77718_f73d87f4a3e111f193c6525400f8a581
    ReservedCode1: ryfqHJqyR/gk6aKSmk3ui1bNtNyOjizEGFRIn/vVV7LekYtFE22e1v/Jg9elxLPnkIuAdg3+sAOmmKLgwmiRRbt3FtTjSXPo2uA79zPshrzfmnfx0xFunsAPTVK0eBOrS5BJcXYY9Yw9KKB+mZhkUEsLLjJx1SQxecB4z1rOMYHrjOFcXrnAzOXBg0A=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: bf570042b9bab5ca445e13766da77718_f73d87f4a3e111f193c6525400f8a581
    ReservedCode2: ryfqHJqyR/gk6aKSmk3ui1bNtNyOjizEGFRIn/vVV7LekYtFE22e1v/Jg9elxLPnkIuAdg3+sAOmmKLgwmiRRbt3FtTjSXPo2uA79zPshrzfmnfx0xFunsAPTVK0eBOrS5BJcXYY9Yw9KKB+mZhkUEsLLjJx1SQxecB4z1rOMYHrjOFcXrnAzOXBg0A=
---

# OpenMix Room 后续更新规划（企业级项目流程）

> 版本: v1.1
> 日期: 2026-08-30
> 范围: 头部追踪（电脑软件 + 手机扫码连局域网，手机端采用 Web App 免安装方案）、工程基线治理、Phase 5 Windows 支持
> 方法: 标准 SDLC（需求冻结 → 技术预研 → 迭代开发 → 测试 → 发布 → 反馈闭环）

---

## 一、方案核实结论：手机扫码连局域网的头部追踪

### 1.1 结论

**方案成立，且是行业已验证的主流路径。** 头部追踪 = 电脑端插件内嵌接收模块 + 手机浏览器 Web App（扫码即出网址、免安装免上架）+ 二维码完成局域网发现与配对。

### 1.2 行业先例（已核实）

| 先例 | 模式 | 与我们的关系 |
|------|------|-------------|
| Opentrack + FreePIE IMU（开源） | 手机陀螺仪 → UDP → 电脑端，手动输入 IP | 验证「手机 IMU + 局域网 UDP」技术路径成熟 |
| HeadTrack（Steam 2026-02 上架） | PC 软件 + 手机 App + 扫码下载/连接，陀螺仪低延迟、摄像头 6DoF | 验证「电脑软件 + 手机 App + 扫码」产品形态被市场接受 |
| FaceTrackNoIR OSC Tracker Plugin | OSC over UDP 接收手机姿态 | 验证 OSC 协议作为姿态传输标准可行 |

### 1.3 技术要点

- **传感器方案**：手机 IMU（陀螺仪 + 加速度计 + 磁力计）融合出 yaw/pitch/roll（3DoF），足以驱动虚拟监听声场旋转；摄像头可扩展 6DoF（Phase 5 可选）。
- **传输协议**：优先 WebSocket（浏览器 Web App 天然支持，双向低延迟）；OSC/UDP 作为兼容备选（如对接现成 OSC App）。
- **二维码配对（Web 方案）**：电脑端插件内嵌轻量 Web 服务（HTTP/HTTPS + WebSocket），生成二维码，内容就是局域网网址（如 `https://192.168.x.x:8443`）；手机扫码直接打开网页 → 网页通过 `DeviceOrientation API` 读取手机陀螺仪姿态 → WebSocket 实时回传电脑端。**不用建站、不用上架 App、手机零安装**，与 dsh 部署后出现访问网址的形态一致。
- **iOS 关键限制**：iOS 13+ 的 `DeviceOrientation` 强制要求 HTTPS 安全上下文（secure context），纯 `http://192.168.x.x` 会被 Safari 拒绝读取陀螺仪。对策：电脑端用 `mkcert` 生成局域网自签证书起 HTTPS/WSS，手机首次访问信任证书一次即可。

### 1.4 风险清单

| 风险 | 等级 | 对策 |
|------|------|------|
| 路由器 AP 隔离 / 跨网段导致手机连不上电脑 | 高 | 二维码中同时给出备选连接说明；文档明确「同一局域网」要求；检测并提示 |
| iOS 对非 HTTPS 页面禁用陀螺仪（DeviceOrientation） | 高 | 电脑端用 mkcert 自签证书起 HTTPS/WSS；首次访问提示信任证书 |
| 防火墙拦截端口 | 中 | 电脑端首次监听时提示放行；文档注明端口 |
| 手机锁屏 / 切后台导致网页暂停 | 中 | 提示保持屏幕常亮（最低亮度）；WebSocket 断线自动重连 |
| 传感器漂移与抖动 | 中 | 姿态融合 + 平滑滤波（互补/卡尔曼/指数平滑）；提供 center 重置 |
| 延迟抖动导致声场不稳 | 中 | 带时间戳与序列号；接收端插值平滑 |
| 网页会话被其他标签页抢占 | 低 | 单会话约束；页面可见性监听（visibilitychange） |

### 1.5 Git 分支与存档策略（已执行）

| 项 | 状态 |
|----|------|
| 仓库 | github.com/MOHE007/openmixROOM（main 含 v0.5.0-beta DMG） |
| 开发分支 | `feature/head-tracking` 已从 main 创建并推送远程（origin/feature/head-tracking，指向 674bddd，已设上游跟踪） |
| 开发策略 | 头部追踪插件端所有改动提交到 `feature/head-tracking`，稳定后经评审 merge 回 main 发 v0.6.0 |
| 代码组织 | 电脑端接收模块 + Web 服务随插件仓库管理；手机端为 Web App（网页即代码），随仓库 `web/` 目录管理或独立仓库（备选）；共享协议文档 `docs/head-tracking-protocol.md` |
| 推送注意 | 本机 git 访问 GitHub 必须显式走代理：`git -c http.proxy=http://127.0.0.1:1082 -c http.version=HTTP/1.1`，否则报 Empty reply / HTTP2 framing error |

---

## 二、分阶段规划（企业级流程）

### Phase 0：工程基线治理（发布前卫生）— 优先级高

**目标**：消除仓库元数据矛盾，建立可发布的工程基线。

| 任务 | 说明 | 验收标准 |
|------|------|----------|
| 统一版本标识 | CMakeLists `VERSION 0.4.1` → 0.5.0；README 顶部同步 | 三处版本号一致 |
| 统一 License | README「待定」与捐赠区「MIT License」矛盾 → 统一为 MIT（框架层） | README 无矛盾表述 |
| 归档 ProjectPlan | Phase 2/3 checkbox 与实际状态对齐，或标记归档 | 文档与代码状态一致 |
| CI 接入回归测试 | 现有 DSP 回归测试（31 cases）纳入自动 CI | 每次提交自动跑测试 |

### Phase 1：头部追踪需求冻结 + 技术预研（Spike）

**目标**：在写业务代码前冻结需求、完成关键技术验证，降低返工。

1. **需求文档**
   - 用户故事：如「作为混音用户，我希望转动头部时监听声场同步旋转，延迟不超过 30ms」
   - 验收标准（Given/When/Then）逐条写出
   - 非功能需求：延迟、抖动、电池、连接稳定性
2. **手机端技术选型（已确定 Web 方案）**
   - 路径 A（推荐，已定）：**Web App（PWA）** —— 电脑端内嵌轻量 Web 服务（HTTPS + WebSocket），手机浏览器扫码即用，零安装零上架，一套代码通吃 iOS/Android；iOS 需 mkcert 自签证书
   - 路径 B（备选）：现成 OSC App（如 GyrOSC）—— 先用现成工具验证插件端，协议走 OSC/UDP，降低首版复杂度
   - 路径 C（暂缓）：自研原生 App —— 体验最稳但成本最高，除非 Web 方案延迟/稳定性不达标再启动
3. **协议选型**：WebSocket（Web 方案主选，双向低延迟） vs OSC/UDP（对接现成 OSC App 的备选）
4. **二维码配对方案设计**：内容格式、token 生成/过期、会话建立、失败 UX
5. **输出**：技术预研报告，明确选型与可行性结论，评审后进入开发

### Phase 2：MVP 端到端打通

**目标**：最小闭环——手机转动 → 声场跟随。

| 模块 | 内容 |
|------|------|
| 电脑端接收模块 | WebSocket 监听 → 姿态解析 → 平滑滤波 → HRTF/声场旋转映射（挂进现有信号链） |
| 电脑端 Web 服务 | 内嵌轻量 HTTPS + WebSocket 服务；mkcert 自签证书；生成二维码（`https://IP:端口`） |
| 手机端（Web App MVP） | 扫码打开网页 → DeviceOrientation 读取姿态 → WebSocket 回传；断线重连 |
| 二维码配对 | 电脑端生成二维码，内容为局域网网址，扫码即出网页自动连接 |
| 端到端验证 | 转动手机，声场实时跟随；延迟可测 |

**验收标准**：端到端延迟 ≤ 30ms；连接成功率 ≥ 95%（同局域网）；断线可重连。

### Phase 3：体验打磨

- 滤波与平滑：消除抖动、可调响应曲线（参考 Opentrack 的 Accela 思路）
- 校准：center 重置（按键/手势）、漂移补偿、传感器温漂处理
- 稳定性：保活、断线重连、多设备支持
- UX：扫码失败/连接中断的引导提示；低电量/屏幕常亮提示

### Phase 4：测试与发布

- **测试金字塔**：单元测试（协议解析、滤波算法）→ 集成测试（WebSocket 链路）→ 真机测试矩阵（不同手机 IMU / 浏览器差异）
- **公测**：发布 v0.6.0-beta，收集反馈，建立 issue 闭环
- **发布**：macOS AU/VST3 更新；手机端**无需上架**（扫码即用，可加 PWA 离线缓存优化二次打开体验）
- **版本管理**：语义化版本（SemVer），头部追踪作为 feature 进 v0.6.0

### Phase 5：Windows 支持（独立大项）

- Windows VST3 构建（mingw-w64 / libmysofa）
- 电脑端接收模块跨平台抽象（macOS + Windows 共用协议层）
- 摄像头 6DoF 追踪（可选增强，跨平台难度高，单列预研）

---

## 三、企业级流程方法论（本规划采用的实践）

| 实践 | 说明 |
|------|------|
| 需求管理 | User Story + 验收标准（Given/When/Then），需求冻结后再开发 |
| 技术预研（Spike） | Phase 1 先验证风险最高的技术点，避免后期返工 |
| 短迭代 | 每个 Phase 拆为 1-2 周迭代，可交付、可验收 |
| CI/CD | 回归测试自动化；构建脚本化（CMake + JUCE 已具备基础） |
| 测试金字塔 | 单元 → 集成 → 真机，逐层覆盖 |
| 发布管理 | SemVer；beta → 公测 → 正式；变更日志维护 |
| 反馈闭环 | 公测 issue 收集 → 分类 → 排期 → 修复 → 回归 |

---

## 四、建议执行顺序

```
Phase 0 基线治理（先做，1 周内）
   ↓
Phase 1 需求冻结 + 技术预研（输出预研报告，评审）
   ↓
Phase 2 MVP 端到端（核心攻坚）
   ↓
Phase 3 体验打磨
   ↓
Phase 4 测试 + 公测 + 发布 v0.6.0
   ↓
Phase 5 Windows 支持（可与头部追踪并行或随后）
```

> 备注：DSP 音质打磨（FDN 混响音色、Crossfeed HRTF、耳机 Profile 扩充）作为**贯穿性优化线**，与上述阶段并行，不阻塞头部追踪主线。
*（内容由AI生成，仅供参考）*
