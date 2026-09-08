// DQ110 双速直流马达驱动器 —— 报价 PPT 生成脚本
const pptxgen = require("pptxgenjs");

const pres = new pptxgen();
pres.layout = "LAYOUT_16x9"; // 10 x 5.625 in
pres.author = "DQ110 Project";
pres.title = "DQ110 双速直流马达驱动器 项目报价";

// ---- 色板: Ocean Gradient (深蓝主调, 工业感) ----
const NAVY = "065A82";   // 主色
const TEAL = "1C7293";   // 支持色
const MIDN = "21295C";   // 深色
const ICE  = "CADCFC";   // 浅蓝(深底上用)
const PAPER= "FFFFFF";   // 白底
const TINT = "EAF3F8";   // 卡片浅底
const GRAY = "5B6B7A";   // 次要文字
const GOLD = "E8A33D";   // 强调(合计)

const F_TITLE = "Calibri";
const F_BODY  = "Calibri";

// ================= P1 封面 =================
const s1 = pres.addSlide();
s1.background = { color: NAVY };
// 装饰圆环(右上)
s1.addShape(pres.ShapeType.ellipse, { x: 7.6, y: -1.2, w: 4.2, h: 4.2, fill: { color: TEAL, transparency: 70 }, line: { color: ICE, width: 1.5, transparency: 50 } });
s1.addShape(pres.ShapeType.ellipse, { x: 8.8, y: -0.4, w: 2.0, h: 2.0, fill: { color: ICE, transparency: 85 }, line: { type: "none" } });
// 标签
s1.addText("产品项目报价  PRODUCT QUOTATION", { x: 0.7, y: 0.55, w: 8.6, h: 0.4, fontSize: 12, color: ICE, charSpacing: 2, fontFace: F_BODY });
// 主标题
s1.addText("DQ110 双速直流马达驱动器", { x: 0.7, y: 1.55, w: 8.6, h: 1.0, fontSize: 40, bold: true, color: PAPER, fontFace: F_TITLE, align: "left" });
s1.addText("项目开发报价 · 附产品规格书与开发计划书", { x: 0.7, y: 2.65, w: 8.6, h: 0.55, fontSize: 18, color: ICE, fontFace: F_BODY });
// 关键指标小条(文字即可, 非色条)
s1.addText("DC 2.4~3.2V  ·  双档 15/30 RPM  ·  霍尔闭环 PID 稳速  ·  过流保护", { x: 0.7, y: 3.35, w: 8.6, h: 0.4, fontSize: 13, color: ICE, fontFace: F_BODY });
// 底部信息
s1.addText("报价编号: DQ110-QUO-20260827-001    报价日期: 2026-08-27    有效期: 30 天", { x: 0.7, y: 4.75, w: 8.6, h: 0.4, fontSize: 12, color: ICE, fontFace: F_BODY });

// ================= P2 项目概述 =================
const s2 = pres.addSlide();
s2.background = { color: PAPER };
s2.addText("项目概述", { x: 0.6, y: 0.3, w: 6, h: 0.6, fontSize: 28, bold: true, color: NAVY, fontFace: F_TITLE });
// 左栏: 产品说明
s2.addText([
  { text: "DQ110 双速直流马达驱动器 (型号 DQ110-V1.0)", options: { bold: true, fontSize: 15, color: MIDN, breakLine: true, paraSpaceAfter: 10 } },
  { text: "用于驱动直流减速马达，实现双档位调速及过流保护，适用于电池供电的小型电动工具/设备。", options: { fontSize: 13, color: GRAY, breakLine: true, paraSpaceAfter: 10 } },
  { text: "2 节 1 号电池供电，三档船型开关 (0 档关 / 一档慢 / 二档快)，霍尔传感器反馈 + PID 闭环稳速。", options: { fontSize: 13, color: GRAY, breakLine: true, paraSpaceAfter: 10 } },
], { x: 0.6, y: 1.15, w: 4.7, h: 2.6, valign: "top", fontFace: F_BODY });
// 主控卡
s2.addShape(pres.ShapeType.roundRect, { x: 0.6, y: 3.95, w: 4.7, h: 1.05, rectRadius: 0.08, fill: { color: TINT }, line: { type: "none" } });
s2.addText([
  { text: "主控 MCU: PY32F002B", options: { bold: true, fontSize: 13, color: NAVY, breakLine: true, paraSpaceAfter: 4 } },
  { text: "ARM Cortex-M0+ · 24MHz · 24KB Flash / 3KB SRAM · SOP16/20", options: { fontSize: 11, color: GRAY } },
], { x: 0.85, y: 4.05, w: 4.3, h: 0.85, valign: "middle", fontFace: F_BODY });
// 右栏: 4 个大数字卡片
const stats = [
  { n: "2.4~3.2V", l: "工作电压 (2节1号电池)" },
  { n: "15 / 30 RPM", l: "双档输出转速 (±5%)" },
  { n: "≤ 5 µA", l: "待机功耗 (Stop模式)" },
  { n: "0.65 / 1.3A", l: "过流保护阈值 (可配置)" },
];
const cw = 2.0, ch = 1.55, cx0 = 5.55, cy0 = 1.15, gap = 0.22;
stats.forEach((st, i) => {
  const cx = cx0 + (i % 2) * (cw + gap);
  const cy = cy0 + Math.floor(i / 2) * (ch + gap);
  s2.addShape(pres.ShapeType.roundRect, { x: cx, y: cy, w: cw, h: ch, rectRadius: 0.1, fill: { color: TINT }, line: { type: "none" } });
  s2.addText(st.n, { x: cx + 0.12, y: cy + 0.18, w: cw - 0.24, h: 0.6, fontSize: 20, bold: true, color: NAVY, fontFace: F_TITLE, align: "center" });
  s2.addText(st.l, { x: cx + 0.12, y: cy + 0.85, w: cw - 0.24, h: 0.55, fontSize: 10.5, color: GRAY, fontFace: F_BODY, align: "center", valign: "top" });
});

// ================= P3 报价明细 =================
const s3 = pres.addSlide();
s3.background = { color: PAPER };
s3.addText("开发费用报价", { x: 0.6, y: 0.3, w: 6, h: 0.6, fontSize: 28, bold: true, color: NAVY, fontFace: F_TITLE });
const qRows = [
  [{ text: "序号", options: { bold: true, color: PAPER, fill: { color: NAVY } } }, { text: "项目", options: { bold: true, color: PAPER, fill: { color: NAVY } } }, { text: "费用 (元)", options: { bold: true, color: PAPER, fill: { color: NAVY }, align: "right" } }, { text: "说明", options: { bold: true, color: PAPER, fill: { color: NAVY } } }],
  ["1", "硬件设计", "8,000", "原理图设计、PCB Layout、BOM 整理"],
  ["2", "软件设计", "12,000", "驱动开发、PID 算法、保护逻辑、测试"],
  ["3", "PCB 打样", "1,500", "10 片样板（含运费）"],
  ["4", "元器件采购", "2,000", "样品阶段元器件（MCU、MOS、运放等）"],
  ["5", "焊接与组装", "1,500", "10 片样板手工焊接与组装"],
  ["6", "调试与测试", "3,000", "功能测试、性能验证、过流保护标定"],
  ["7", "项目管理费", "2,000", "项目协调、文档整理、客户沟通"],
  [{ text: "小计", options: { bold: true, fill: { color: TINT } } }, { text: "", options: { fill: { color: TINT } } }, { text: "30,000", options: { bold: true, fill: { color: TINT }, align: "right" } }, { text: "", options: { fill: { color: TINT } } }],
  [{ text: "税费 (6%)", options: { bold: true, fill: { color: TINT } } }, { text: "", options: { fill: { color: TINT } } }, { text: "1,800", options: { bold: true, fill: { color: TINT }, align: "right" } }, { text: "", options: { fill: { color: TINT } } }],
  [{ text: "合计", options: { bold: true, color: PAPER, fill: { color: NAVY }, fontSize: 14 } }, { text: "", options: { fill: { color: NAVY } } }, { text: "31,800", options: { bold: true, color: PAPER, fill: { color: NAVY }, align: "right", fontSize: 14 } }, { text: "", options: { fill: { color: NAVY } } }],
];
s3.addTable(qRows, { x: 0.6, y: 1.05, w: 7.1, colW: [0.6, 1.9, 1.4, 3.2], rowH: 0.36, fontSize: 11.5, fontFace: F_BODY, border: { pt: 0.5, color: "D5E2EC" }, valign: "middle", align: "left" });
// 右侧: 合计大数字
s3.addShape(pres.ShapeType.roundRect, { x: 8.0, y: 1.05, w: 1.5, h: 3.95, rectRadius: 0.08, fill: { color: NAVY }, line: { type: "none" } });
s3.addText("总报价 (含税)", { x: 8.0, y: 1.35, w: 1.5, h: 0.5, fontSize: 13, bold: true, color: ICE, fontFace: F_BODY, align: "center" });
s3.addText("¥31,800", { x: 8.0, y: 2.0, w: 1.5, h: 0.9, fontSize: 24, bold: true, color: GOLD, fontFace: F_TITLE, align: "center" });
s3.addText("开发 + 样品\n6 周交付", { x: 8.0, y: 3.3, w: 1.5, h: 0.9, fontSize: 12, color: ICE, fontFace: F_BODY, align: "center" });

// ================= P4 商务条款 =================
const s4 = pres.addSlide();
s4.background = { color: PAPER };
s4.addText("付款条款与交付物", { x: 0.6, y: 0.3, w: 7, h: 0.6, fontSize: 28, bold: true, color: NAVY, fontFace: F_TITLE });
// 左: 付款条款 3 阶段
const pay = [
  { t: "首付款 50%", a: "¥15,900", d: "合同签订后 3 日内" },
  { t: "中期款 30%", a: "¥9,540", d: "原理图评审通过、PCB 发板后" },
  { t: "尾款 20%", a: "¥6,360", d: "样品验收合格后" },
];
pay.forEach((p, i) => {
  const y = 1.1 + i * 1.32;
  s4.addShape(pres.ShapeType.roundRect, { x: 0.6, y: y, w: 4.35, h: 1.12, rectRadius: 0.09, fill: { color: i === 0 ? TINT : PAPER }, line: { color: "D5E2EC", width: 1 } });
  s4.addShape(pres.ShapeType.ellipse, { x: 0.85, y: y + 0.28, w: 0.56, h: 0.56, fill: { color: NAVY }, line: { type: "none" } });
  s4.addText(String(i + 1), { x: 0.85, y: y + 0.28, w: 0.56, h: 0.56, fontSize: 16, bold: true, color: PAPER, fontFace: F_BODY, align: "center", valign: "middle" });
  s4.addText(p.t, { x: 1.6, y: y + 0.14, w: 2.1, h: 0.4, fontSize: 14, bold: true, color: MIDN, fontFace: F_BODY });
  s4.addText(p.a, { x: 3.55, y: y + 0.12, w: 1.3, h: 0.45, fontSize: 15, bold: true, color: NAVY, fontFace: F_TITLE, align: "right" });
  s4.addText(p.d, { x: 1.6, y: y + 0.58, w: 3.2, h: 0.35, fontSize: 11, color: GRAY, fontFace: F_BODY });
});
// 右: 交付物清单
s4.addText("交付物清单", { x: 5.45, y: 1.0, w: 4, h: 0.4, fontSize: 16, bold: true, color: MIDN, fontFace: F_BODY });
const dels = [
  ["功能样品 PCBA（完整功能测试）", "5 片"],
  ["原理图 (PDF) / PCB Gerber / BOM", "各 1 份"],
  ["软件源代码（含注释）", "1 份"],
  ["测试报告 / 产品规格书", "各 1 份"],
];
dels.forEach((d, i) => {
  const y = 1.55 + i * 0.62;
  s4.addShape(pres.ShapeType.roundRect, { x: 5.45, y: y, w: 4.0, h: 0.5, rectRadius: 0.08, fill: { color: TINT }, line: { type: "none" } });
  s4.addText("✓", { x: 5.6, y: y + 0.02, w: 0.4, h: 0.46, fontSize: 13, bold: true, color: TEAL, fontFace: F_BODY, align: "center", valign: "middle" });
  s4.addText(d[0], { x: 6.0, y: y + 0.02, w: 2.55, h: 0.46, fontSize: 12, color: MIDN, fontFace: F_BODY, valign: "middle" });
  s4.addText(d[1], { x: 8.55, y: y + 0.02, w: 0.8, h: 0.46, fontSize: 12, bold: true, color: NAVY, fontFace: F_BODY, align: "right", valign: "middle" });
});
s4.addText("量产预估: PCBA 18~25 元/片 · 整机成本 25~35 元（不含马达、电池、开关）", { x: 0.6, y: 5.15, w: 8.8, h: 0.35, fontSize: 11, color: GRAY, fontFace: F_BODY });

// ================= P5 开发计划 =================
const s5 = pres.addSlide();
s5.background = { color: PAPER };
s5.addText("开发计划（6 周交付）", { x: 0.6, y: 0.3, w: 7, h: 0.6, fontSize: 28, bold: true, color: NAVY, fontFace: F_TITLE });
const ph = [
  { w: "第 1 周", t: "需求确认", d: "需求评审\n方案确认" },
  { w: "第 2-3 周", t: "硬件设计", d: "原理图\nPCB Layout" },
  { w: "第 2-4 周", t: "软件设计", d: "驱动 / PID\n保护逻辑" },
  { w: "第 3-4 周", t: "PCBA 制造", d: "打样 10 片\n采购焊接" },
  { w: "第 4-5 周", t: "调试与验证", d: "功能调试\n过流标定" },
  { w: "第 5-6 周", t: "优化与交付", d: "Bug 修复\n交付 5 片" },
];
const pw = 1.36, pgap = 0.11, px0 = 0.6, py = 1.25;
ph.forEach((p, i) => {
  const x = px0 + i * (pw + pgap);
  // 上方节点圆
  s5.addShape(pres.ShapeType.ellipse, { x: x + pw / 2 - 0.14, y: py - 0.1, w: 0.28, h: 0.28, fill: { color: NAVY }, line: { type: "none" } });
  // 卡片
  s5.addShape(pres.ShapeType.roundRect, { x: x, y: py + 0.3, w: pw, h: 2.15, rectRadius: 0.09, fill: { color: i % 2 ? TINT : PAPER }, line: { color: "D5E2EC", width: 1 } });
  s5.addText(p.w, { x: x + 0.05, y: py + 0.42, w: pw - 0.1, h: 0.32, fontSize: 10.5, bold: true, color: TEAL, fontFace: F_BODY, align: "center" });
  s5.addText(p.t, { x: x + 0.05, y: py + 0.8, w: pw - 0.1, h: 0.45, fontSize: 14, bold: true, color: MIDN, fontFace: F_BODY, align: "center" });
  s5.addText(p.d, { x: x + 0.05, y: py + 1.3, w: pw - 0.1, h: 1.0, fontSize: 10.5, color: GRAY, fontFace: F_BODY, align: "center", valign: "top" });
});
// 底部: 团队 + 交付目标
s5.addText("项目团队", { x: 0.6, y: 4.0, w: 2, h: 0.4, fontSize: 15, bold: true, color: MIDN, fontFace: F_BODY });
const team = ["项目经理\n协调 · 进度 · 沟通", "硬件工程师\n原理图 · Layout · BOM", "软件工程师\n驱动 · PID · 保护", "测试工程师\n功能 · 性能验证"];
team.forEach((t, i) => {
  const x = 0.6 + i * 1.78;
  s5.addShape(pres.ShapeType.roundRect, { x: x, y: 4.42, w: 1.66, h: 0.85, rectRadius: 0.08, fill: { color: TINT }, line: { type: "none" } });
  s5.addText(t, { x: x + 0.06, y: 4.48, w: 1.54, h: 0.75, fontSize: 10.5, color: MIDN, fontFace: F_BODY, align: "center", valign: "middle" });
});

// ================= P6 附录A 规格书要点 =================
const s6 = pres.addSlide();
s6.background = { color: PAPER };
s6.addText("附录 A · 产品规格书要点", { x: 0.6, y: 0.3, w: 8, h: 0.6, fontSize: 28, bold: true, color: NAVY, fontFace: F_TITLE });
// 左表: 电气规格
s6.addText("电气规格", { x: 0.6, y: 1.0, w: 3, h: 0.35, fontSize: 14, bold: true, color: MIDN, fontFace: F_BODY });
const eRows = [
  [{ text: "参数", options: { bold: true, color: PAPER, fill: { color: NAVY } } }, { text: "规格", options: { bold: true, color: PAPER, fill: { color: NAVY } } }],
  ["工作电压", "DC 2.4V ~ 3.2V（2 节 1 号电池）"],
  ["额定电压", "DC 3.0V"],
  ["待机电流", "≤ 5 µA（Stop 模式）"],
  ["马达工作电流", "≤ 0.35A 额定 / ≤ 1.3A 堵转"],
];
s6.addTable(eRows, { x: 0.6, y: 1.38, w: 4.5, colW: [1.5, 3.0], rowH: 0.3, fontSize: 11, fontFace: F_BODY, border: { pt: 0.5, color: "D5E2EC" }, valign: "middle" });
// 性能参数表
s6.addText("性能参数（带载）", { x: 0.6, y: 3.05, w: 3, h: 0.35, fontSize: 14, bold: true, color: MIDN, fontFace: F_BODY });
const pRows = [
  [{ text: "参数", options: { bold: true, color: PAPER, fill: { color: NAVY } } }, { text: "一档", options: { bold: true, color: PAPER, fill: { color: NAVY } } }, { text: "二档", options: { bold: true, color: PAPER, fill: { color: NAVY } } }],
  ["输出转速", "15 RPM ±5%", "30 RPM ±5%"],
  ["马达扭矩", "≥ 101 mN·m", "≥ 202 mN·m"],
  ["过流保护阈值", "0.65A", "1.3A"],
  ["保护响应时间", "≤ 100ms", "≤ 100ms"],
];
s6.addTable(pRows, { x: 0.6, y: 3.42, w: 4.5, colW: [1.5, 1.5, 1.5], rowH: 0.28, fontSize: 11, fontFace: F_BODY, border: { pt: 0.5, color: "D5E2EC" }, valign: "middle" });
// 右栏: 功能规格
s6.addText("功能规格", { x: 5.5, y: 1.0, w: 3, h: 0.35, fontSize: 14, bold: true, color: MIDN, fontFace: F_BODY });
const funcs = [
  "档位控制 — 三档船型开关（0/一档/二档）",
  "马达驱动 — H 桥 MOSFET，支持正反转",
  "PWM 调速 — 16 位定时器，频率可调",
  "转速闭环 — 霍尔反馈 + PID 稳速",
  "过流保护 — 采样电阻 + 运放 + ADC 软件保护",
  "过载保护 — 堵转检测自动关断",
  "看门狗 — 独立 IWDG 防程序跑飞",
];
funcs.forEach((f, i) => {
  const y = 1.45 + i * 0.4;
  s6.addText("●", { x: 5.5, y: y + 0.02, w: 0.3, h: 0.3, fontSize: 9, color: TEAL, fontFace: F_BODY });
  s6.addText(f, { x: 5.85, y: y, w: 3.7, h: 0.38, fontSize: 11, color: MIDN, fontFace: F_BODY, valign: "middle" });
});
// 环境可靠性
s6.addText("环境与可靠性", { x: 5.5, y: 4.3, w: 3, h: 0.35, fontSize: 14, bold: true, color: MIDN, fontFace: F_BODY });
s6.addText("工作温度 -10°C ~ +60°C · 存储 -20°C ~ +70°C · ESD ≥ ±4kV (HBM)", { x: 5.5, y: 4.68, w: 4.0, h: 0.55, fontSize: 11.5, color: GRAY, fontFace: F_BODY, valign: "top" });

// ================= P7 附录B 计划书要点 =================
const s7 = pres.addSlide();
s7.background = { color: PAPER };
s7.addText("附录 B · 开发计划书要点", { x: 0.6, y: 0.3, w: 8, h: 0.6, fontSize: 28, bold: true, color: NAVY, fontFace: F_TITLE });
// 验收标准
s7.addText("验收标准", { x: 0.6, y: 1.0, w: 3, h: 0.35, fontSize: 14, bold: true, color: MIDN, fontFace: F_BODY });
const accRows = [
  [{ text: "验收项", options: { bold: true, color: PAPER, fill: { color: NAVY } } }, { text: "标准", options: { bold: true, color: PAPER, fill: { color: NAVY } } }],
  ["功能完整性", "档位切换、调速、保护全部正常工作"],
  ["转速精度", "一档 15RPM ±5%，二档 30RPM ±5%"],
  ["过流保护", "一档 0.65A / 二档 1.3A 可靠保护"],
  ["文档完整性", "原理图、BOM、源代码、测试报告齐全"],
];
s7.addTable(accRows, { x: 0.6, y: 1.4, w: 4.5, colW: [1.4, 3.1], rowH: 0.34, fontSize: 11, fontFace: F_BODY, border: { pt: 0.5, color: "D5E2EC" }, valign: "middle" });
// 风险应对
s7.addText("风险分析与应对", { x: 5.5, y: 1.0, w: 3, h: 0.35, fontSize: 14, bold: true, color: MIDN, fontFace: F_BODY });
const rRows = [
  [{ text: "风险", options: { bold: true, color: PAPER, fill: { color: NAVY } } }, { text: "应对措施", options: { bold: true, color: PAPER, fill: { color: NAVY } } }],
];
const risks = [
  ["马达参数与规格不符", "提前获取马达样品实测"],
  ["转速闭环效果不达标", "预留 PID 调试接口、延长调试期"],
  ["过流保护误触发", "硬件滤波 + 软件多次平均分级保护"],
  ["元器件交期延误", "提前采购、备选替代型号"],
];
risks.forEach((r) => rRows.push(r));
s7.addTable(rRows, { x: 5.5, y: 1.4, w: 4.0, colW: [1.7, 2.3], rowH: 0.5, fontSize: 10.5, fontFace: F_BODY, border: { pt: 0.5, color: "D5E2EC" }, valign: "middle" });
// 底部: 项目目标
s7.addShape(pres.ShapeType.roundRect, { x: 0.6, y: 3.6, w: 8.9, h: 1.1, rectRadius: 0.09, fill: { color: TINT }, line: { type: "none" } });
s7.addText([
  { text: "项目目标", options: { bold: true, fontSize: 13, color: NAVY, breakLine: true, paraSpaceAfter: 5 } },
  { text: "技术: 一档 15RPM、二档 30RPM 转速稳定，过流保护可靠  |  交付: 6 周内交付 5 片功能完好的 PCBA 样品及全套设计文档", options: { fontSize: 11.5, color: GRAY } },
], { x: 0.85, y: 3.72, w: 8.4, h: 0.9, valign: "middle", fontFace: F_BODY });

// ================= P8 封底 =================
const s8 = pres.addSlide();
s8.background = { color: MIDN };
s8.addShape(pres.ShapeType.ellipse, { x: -1.0, y: 3.6, w: 3.6, h: 3.6, fill: { color: NAVY, transparency: 40 }, line: { type: "none" } });
s8.addText("谢谢", { x: 0.7, y: 1.7, w: 8.6, h: 1.0, fontSize: 40, bold: true, color: PAPER, fontFace: F_TITLE, align: "center" });
s8.addText("期待与您合作，欢迎垂询", { x: 0.7, y: 2.8, w: 8.6, h: 0.5, fontSize: 16, color: ICE, fontFace: F_BODY, align: "center" });
s8.addText("报价编号 DQ110-QUO-20260827-001 · 有效期至 2026-09-26", { x: 0.7, y: 4.6, w: 8.6, h: 0.4, fontSize: 12, color: ICE, fontFace: F_BODY, align: "center" });

pres.writeFile({ fileName: "/Users/andrewlau/qtprj/DQ110/DQ110_项目报价.pptx" }).then((f) => {
  console.log("WROTE:", f);
}).catch((e) => { console.error("ERR", e); process.exit(1); });
