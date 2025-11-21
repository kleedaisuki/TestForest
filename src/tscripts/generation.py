# -*- coding: utf-8 -*-
"""
generation.py
测试用例生成器，可爱教授版♡
"""

import os
import json
import random
from pathlib import Path
from typing import List, Dict


# ===============================================================
# 默认 case 目录
# ===============================================================


def default_cases_directory() -> Path:
    cwd = Path(os.getcwd())
    return cwd / "test-works" / "cases"


# ===============================================================
# 生成主入口
# ===============================================================


def generate_case(
    seed: int, size: int, op_ratio=None, distribution: str = "uniform"
) -> List[Dict]:
    """
    生成 size 条测试操作。
    - seed: 随机种子
    - size: 操作条数
    - op_ratio: 操作比例 dict，如 {"insert": 0.6, "find": 0.4}
    - distribution: key 生成方式，可选：uniform/sorted/reverse/gaussian

    返回一个 list，每个元素形如：
        {"op": "insert", "key": 123}
    """

    if op_ratio is None:
        # 默认：70% 插入，30% 查找
        op_ratio = {"insert": 0.7, "find": 0.3}

    r = random.Random(seed)
    ops = list(op_ratio.keys())
    weights = list(op_ratio.values())

    # 生成 key 分布
    if distribution == "uniform":
        keys = [r.randint(1, 10**9) for _ in range(size)]
    elif distribution == "sorted":
        keys = sorted(r.randint(1, 10**9) for _ in range(size))
    elif distribution == "reverse":
        keys = sorted((r.randint(1, 10**9) for _ in range(size)), reverse=True)
    elif distribution == "gaussian":
        # 高斯分布（均值 5e8，标准差 1e8）
        keys = [int(r.gauss(5e8, 1e8)) for _ in range(size)]
    else:
        raise ValueError(f"unknown distribution: {distribution}")

    case = []

    for k in keys:
        op = r.choices(ops, weights=weights, k=1)[0]
        case.append({"op": op, "key": k})

    return case


# ===============================================================
# 写入 case 文件
# ===============================================================


def write_case_to_file(case: List[Dict], seed: int) -> Path:
    """
    将 case 写入 test-works/cases/{seed}.jsonl
    """
    dirpath = default_cases_directory()
    dirpath.mkdir(parents=True, exist_ok=True)

    filepath = dirpath / f"{seed}.jsonl"

    with open(filepath, "w", encoding="utf-8") as fp:
        for item in case:
            fp.write(json.dumps(item) + "\n")

    return filepath


# ===============================================================
# 顶层 API：generate_and_save
# ===============================================================


def generate_and_save(
    seed: int, size: int, op_ratio=None, distribution="uniform"
) -> Path:
    """
    一步生成并保存。
    """
    case = generate_case(seed, size, op_ratio, distribution)
    return write_case_to_file(case, seed)


# ===============================================================
# 如果直接运行 generation.py，则生成 1 个样例
# ===============================================================

if __name__ == "__main__":
    path = generate_and_save(seed=42, size=20)
    print("生成完成:", path)
