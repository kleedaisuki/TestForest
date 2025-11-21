# -*- coding: utf-8 -*-
"""
visualize.py
性能指标可视化模块
"""

from __future__ import annotations

import os
from datetime import datetime
from pathlib import Path
from typing import Dict, Optional, List

import matplotlib.pyplot as plt

# ============================================================
# 工具函数：目录
# ============================================================


def default_figures_directory() -> Path:
    """
    返回默认图像输出目录：<cwd>/test-works/figs。
    / Return default figure output directory: <cwd>/test-works/figs.
    """
    cwd = Path(os.getcwd())
    return cwd / "test-works" / "figs"


def plot_time_vs_n(
    data: Dict[str, Dict[str, List[float]]],
    title: str = "Time vs N",
    xlabel: str = "N (input size)",
    ylabel: str = "Time (seconds)",
    save_path: Optional[Path] = None,
    show: bool = False,
) -> Path:
    """
    绘制算法分析常用的 N vs Time 折线图。
    可绘制多条曲线（多个算法或多种数据结构对比）。

    参数说明：
    - data:
        {
            "AVL": {"x": [...], "y": [...]},
            "RedBlack": {"x": [...], "y": [...]},
            "BTree": {"x": [...], "y": [...]},
        }
    - title: 图标题
    - xlabel / ylabel: 坐标轴标签
    - save_path: 保存路径（None 则自动在 test-works/figs/ 下按时间戳命名）
    - show: 是否 plt.show()

    返回保存路径。
    """
    # 自动生成保存路径
    if save_path is None:
        fig_dir = default_figures_directory()
        fig_dir.mkdir(parents=True, exist_ok=True)
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        save_path = fig_dir / f"time_vs_n_{ts}.png"
    else:
        save_path = Path(save_path)
        if save_path.parent:
            save_path.parent.mkdir(parents=True, exist_ok=True)

    fig, ax = plt.subplots()

    # 绘制多条折线
    for name, series in data.items():
        x = series.get("x")
        y = series.get("y")
        if not x or not y or len(x) != len(y):
            raise ValueError(f"Invalid data for curve '{name}': x and y must have same length.")

        ax.plot(x, y, marker="o", label=name)

    ax.set_title(title)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.grid(True, linestyle="--", alpha=0.6)
    ax.legend()

    fig.tight_layout()
    fig.savefig(save_path)

    if show:
        plt.show()
    else:
        plt.close(fig)

    return save_path
