# -*- coding: utf-8 -*-
"""
metrics.py
纯模块：提供 CSV 性能日志的加载、聚合与指标计算。
不包含 CLI，不会自动执行。
"""

from __future__ import annotations

import csv
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional

from tscripts.logger import log_error


# ============================================================
# 日志目录工具
# ============================================================


def default_logs_directory() -> Path:
    """返回默认日志目录 test-works/logs。"""
    return Path(os.getcwd()) / "test-works" / "logs"


def list_log_files(directory: Optional[Path] = None) -> List[Path]:
    """列出目录中所有 .csv 日志文件。"""
    directory = directory or default_logs_directory()
    if not directory.exists():
        return []
    return sorted(
        p for p in directory.iterdir() if p.is_file() and p.suffix.lower() == ".csv"
    )


def find_latest_log(directory: Optional[Path] = None) -> Optional[Path]:
    """找出最新一份日志文件（按文件名排序）。"""
    files = list_log_files(directory)
    return files[-1] if files else None


# ============================================================
# 数据模型
# ============================================================


@dataclass
class Record:
    """单行 CSV 记录。"""

    test_func_name: str
    count: int
    time_usage: float

    @property
    def throughput(self) -> float:
        """吞吐量 = count / time_usage（op/s）。"""
        if self.time_usage <= 0:
            return float("inf")
        return self.count / self.time_usage


@dataclass
class TestSummary:
    """针对某个测试函数的聚合结果。"""

    name: str
    total_count: int
    total_time: float
    runs: int

    @property
    def avg_time_per_run(self) -> float:
        """平均每次运行耗时。"""
        return 0.0 if self.runs <= 0 else self.total_time / self.runs

    @property
    def throughput(self) -> float:
        """整体吞吐量。"""
        if self.total_time <= 0:
            return float("inf")
        return self.total_count / self.total_time


# ============================================================
# 加载 CSV
# ============================================================


def load_log(path: Path) -> List[Record]:
    """
    读取 CSV 文件并解析为 Record 列表。
    出错行会记录错误日志，但不会中断整个文件的读取。
    """
    path = Path(path)
    result: List[Record] = []

    try:
        fp = path.open("r", newline="", encoding="utf-8")
    except Exception as e:
        # 文件级 I/O 错误 → 严重 → 直接报错
        log_error(f"[metrics] Failed to open log file: {path} ({e})")
        raise

    with fp:
        reader = csv.DictReader(fp)

        line_no = 1  # DictReader 不含标题行 → 从 1 开始计数
        for row in reader:
            line_no += 1
            try:
                name = str(row["test_func_name"])
                count = int(row["count"])
                time_usage = float(row["time_usage"])
            except Exception as exc:
                # 精准记录错误信息
                log_error(
                    f"[metrics] Bad CSV row in {path} at line {line_no}: "
                    f"row={row}, error={exc}"
                )
                continue

            result.append(Record(name, count, time_usage))

    return result


# ============================================================
# 指标聚合
# ============================================================


def summarize_records(records: Iterable[Record]) -> Dict[str, TestSummary]:
    """对多条 Record 进行聚合，按 test_func_name 分组。"""
    acc: Dict[str, TestSummary] = {}

    for r in records:
        if r.test_func_name not in acc:
            acc[r.test_func_name] = TestSummary(
                name=r.test_func_name,
                total_count=0,
                total_time=0.0,
                runs=0,
            )
        s = acc[r.test_func_name]
        s.total_count += r.count
        s.total_time += r.time_usage
        s.runs += 1

    return acc


def summarize_file(path: Path) -> Dict[str, TestSummary]:
    """读取单个日志文件并返回 summary。"""
    return summarize_records(load_log(path))


def summarize_all_logs(directory: Optional[Path] = None) -> Dict[str, TestSummary]:
    """读取目录下所有日志文件并整体聚合。"""
    directory = directory or default_logs_directory()

    all_records: List[Record] = []
    for f in list_log_files(directory):
        all_records.extend(load_log(f))

    return summarize_records(all_records)
