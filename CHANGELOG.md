# Changelog

## v1.0-opt - 2025-11-15
- 优化点：
  - 将 `Buzzer_Short` 与 `Buzzer_Long` 合并为共用的 `Buzzer_Pulse`，移除重复检查逻辑。
  - 精简 `Buzzer_Update` 分支，减少重复条件判断与分支。
- 修改文件： `buzzer.c`
- Git 提交信息： `[优化] 减少代码量，保持功能不变`

(自动生成：首轮无人工确认的代码体积优化）
