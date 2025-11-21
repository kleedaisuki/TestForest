# -*- coding: utf-8 -*-
"""
tscripts
测试流水线 Python 工具包 / Python toolkit for the test pipeline.

该包下提供四个核心模块（pure modules, 无副作用导入）：
- logger     : 文本日志与 CSV 日志工具
- metrics    : 读取并聚合 C++ 测试结果 CSV
- visualize  : 绘制性能图（例如 N vs Time 折线图）
- generation : 生成测试用例（cases）
"""

from __future__ import annotations

from . import logger as logger
from . import metrics as metrics
from . import visualize as visualize
from . import generation as generation

__all__ = [
    "logger",
    "metrics",
    "visualize",
    "generation",
]

__version__ = "0.1.0"
