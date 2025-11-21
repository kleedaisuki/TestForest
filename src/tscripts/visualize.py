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


def scatter_time_vs_n(
    data: Dict[str, Dict[str, List[float]]],
    title: str = "Time vs N",
    xlabel: str = "N (input size)",
    ylabel: str = "Time (seconds)",
    save_path: Optional[Path] = None,
    show: bool = False,
) -> Path:
    """
    绘制 N vs Time 散点图版本（更适合性能基准测试）。
    """

    # 自动生成保存路径
    if save_path is None:
        fig_dir = default_figures_directory()   
        fig_dir.mkdir(parents=True, exist_ok=True)
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        save_path = fig_dir / f"{title}_P{ts}.png".replace(" ", "_")
    else:
        save_path = Path(save_path)
        if save_path.parent:
            save_path.parent.mkdir(parents=True, exist_ok=True)

    fig, ax = plt.subplots()

    # 绘制多条散点
    for name, series in data.items():
        x = series["x"]
        y = series["y"]
        if not x or not y or len(x) != len(y):
            raise ValueError(
                f"Invalid data for curve '{name}': x and y must have same length."
            )

        # 使用更美观的散点图（替代折线）
        ax.scatter(x, y, s=8, alpha=0.6, label=name)

    ax.set_title(title)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend()

    fig.tight_layout()
    fig.savefig(save_path)

    if show:
        plt.show()
    else:
        plt.close(fig)

    return save_path
