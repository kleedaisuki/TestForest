# -*- coding: utf-8 -*-
"""
logger.py
与 C++ utils.cpp 的日志保持一致。
"""

import os
import sys
import csv
import threading
from datetime import datetime
from pathlib import Path


# ============================================================
# 工具函数：时间戳 / Timestamp
# ============================================================


def make_timestamp_string() -> str:
    """
    生成 YYYYMMDD_HHMMSS 格式的时间戳（与 C++ 保持一致）
    """
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def default_logs_directory() -> Path:
    """
    返回 test-works/logs 的绝对路径
    """
    cwd = Path(os.getcwd())
    return cwd / "test-works" / "logs"


# ============================================================
# CSV Logger
# ============================================================


class CsvLogger:
    """
    CSV 日志输出。
    与 C++ CsvLogger 行为对齐：
    - 自动写表头 test_func_name,count,time_usage
    - append(name, count, seconds)
    - flush()
    """

    def __init__(self, filepath: Path, write_header: bool = True):
        self.filepath = Path(filepath)
        self.filepath.parent.mkdir(parents=True, exist_ok=True)

        self._fp = open(self.filepath, "w", newline="", encoding="utf-8")
        self._csv = csv.writer(self._fp)
        self._lock = threading.Lock()

        if write_header:
            self._csv.writerow(["test_func_name", "count", "time_usage"])

    @classmethod
    def open_default(cls, write_header=True):
        """
        在 default_logs_directory() 中创建以 timestamp 命名的 csv
        """
        dirpath = default_logs_directory()
        ts = make_timestamp_string()
        return cls(dirpath / f"{ts}.csv", write_header)

    @classmethod
    def open_at(cls, directory: Path, write_header=True):
        """
        在 directory 中创建 timestamp csv
        """
        ts = make_timestamp_string()
        return cls(Path(directory) / f"{ts}.csv", write_header)

    def append(self, test_func_name: str, count: int, time_usage: float):
        """
        写入一行：与 C++ 格式完全一致。
        """
        with self._lock:
            self._csv.writerow([test_func_name, count, f"{time_usage:.9f}"])

    def flush(self):
        with self._lock:
            self._fp.flush()

    def close(self):
        with self._lock:
            self._fp.close()

    def valid(self) -> bool:
        return not self._fp.closed


# ============================================================
# 文本日志（INFO / ERROR）
# ============================================================

_log_lock = threading.Lock()


def _make_prefix(level: str) -> str:
    ts = make_timestamp_string()
    return f"[{level} {ts}] "


def log_info(msg: str):
    """
    打印 INFO + 时间戳 到 stderr（与 C++ 行为一致）
    """
    with _log_lock:
        sys.stderr.write(_make_prefix("INFO") + msg + "\n")


def log_error(msg: str):
    """
    打印 ERROR + 时间戳 到 stderr
    """
    with _log_lock:
        sys.stderr.write(_make_prefix("ERROR") + msg + "\n")
