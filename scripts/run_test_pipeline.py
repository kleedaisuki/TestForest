# -*- coding: utf-8 -*-
"""
run_test_pipeline.py
测试流水线 CLI：
1. 使用 CMake 在 build/ 目录构建 C++ 基准程序 test_forest_bench
2. 运行基准程序，生成 test-works/logs/*.csv
3. 调用 tscripts.metrics + tscripts.visualize 读取最新日志并绘制 N vs Time 图

运行完整流水线（configure + build + run + scatter）：
python ./scripts/run_test_pipeline.py

只分析已有日志，不重新编译 / 运行 C++：
python ./scripts/run_test_pipeline.py --only-scatter

如果你想手动控制 CMake generator（比如强制用 MinGW Makefiles）：
python ./scripts/run_test_pipeline.py --cmake-generator "MinGW Makefiles"
"""

from __future__ import annotations

import argparse
import os
import platform
import subprocess
import sys
from pathlib import Path
from typing import Dict, List

from tscripts import metrics

# ------------------------------------------------------------
# Python 包导入：假定已经通过 `pip install -e .` 可用 tscripts
# 如果用户尚未安装，可在这里做一个兜底的 sys.path 修正
# ------------------------------------------------------------

try:
    from tscripts import logger, visualize
except ImportError:
    # 尝试将 <project_root>/src 加入 sys.path 再导入一次
    this_file = Path(__file__).resolve()
    project_root = this_file.parents[1]
    src_dir = project_root / "src"
    if src_dir.is_dir():
        sys.path.insert(0, str(src_dir))
    from tscripts import logger, visualize  # type: ignore


# ============================================================
# 工具函数：子进程与路径
# ============================================================


def run_command(args: List[str], cwd: Path | None = None) -> None:
    """
    在子进程中运行命令，失败时抛出异常。

    Run command in subprocess and raise if it fails.
    """
    cmd_str = " ".join(args)
    logger.log_info(f"[pipeline] Running command: {cmd_str}")
    try:
        result = subprocess.run(args, cwd=str(cwd) if cwd is not None else None)
    except FileNotFoundError as exc:
        logger.log_error(f"[pipeline] Failed to run command: {cmd_str} ({exc})")
        raise

    if result.returncode != 0:
        logger.log_error(
            f"[pipeline] Command failed with exit code {result.returncode}: {cmd_str}"
        )
        raise RuntimeError(f"Command failed: {cmd_str}")


def detect_executable_path(build_dir: Path) -> Path:
    """
    根据 CMakeLists.txt 约定推断可执行文件路径：
    build/bin/test_forest_bench[.exe]
    """
    bin_dir = build_dir / "bin"
    exe_name = "test_forest_bench"

    # Windows 下通常带 .exe
    if platform.system().lower().startswith("win"):
        candidate = bin_dir / (exe_name + ".exe")
        if candidate.exists():
            return candidate
    # 其他平台 or 回退
    candidate = bin_dir / exe_name
    return candidate


# ============================================================
# CMake 阶段：configure + build
# ============================================================


def cmake_configure_and_build(
    project_root: Path,
    build_dir: Path,
    build_type: str = "Release",
    cmake_generator: str | None = None,
    skip_configure: bool = False,
    skip_build: bool = False,
) -> None:
    """
    使用 CMake 在指定 build 目录完成配置与编译。

    Configure & build project using CMake in given build directory.
    """
    build_dir.mkdir(parents=True, exist_ok=True)

    # --------- configure ---------
    if not skip_configure:
        configure_cmd: List[str] = [
            "cmake",
            "-S",
            str(project_root),
            "-B",
            str(build_dir),
            f"-DCMAKE_BUILD_TYPE={build_type}",
        ]
        if cmake_generator:
            configure_cmd.extend(["-G", cmake_generator])
        run_command(configure_cmd, cwd=project_root)
    else:
        logger.log_info("[pipeline] Skip CMake configure step")

    # --------- build ---------
    if not skip_build:
        build_cmd: List[str] = [
            "cmake",
            "--build",
            str(build_dir),
            "--config",
            build_type,
        ]
        run_command(build_cmd, cwd=project_root)
    else:
        logger.log_info("[pipeline] Skip CMake build step")


# ============================================================
# 运行基准程序
# ============================================================


def run_cpp_bench(project_root: Path, build_dir: Path) -> None:
    """
    运行 C++ 基准程序 test_forest_bench，可执行文件路径由 CMake 布局推断。

    Run C++ benchmark executable; logs will be written to test-works/logs/.
    """
    exe_path = detect_executable_path(build_dir)
    if not exe_path.exists():
        raise FileNotFoundError(
            f"Executable not found: {exe_path} "
            f"(did you run CMake build successfully?)"
        )

    logger.log_info(f"[pipeline] Running benchmark executable: {exe_path}")
    # 为了让 C++ 那边按 <cwd>/test-works/logs 写文件，这里 cwd 设为工程根目录
    run_command([str(exe_path)], cwd=project_root)


# ============================================================
# 结果分析与绘图
# ============================================================


def build_time_vs_n_data(
    records: List[metrics.Record],
) -> Dict[str, Dict[str, Dict[str, List[float]]]]:
    """
    将 metrics.Record 列表转换为可供 visualize.plot_time_vs_n 使用的数据结构。

    输入：多条像 "BinaryTree.insert.N=100" 这样的 test_func_name 记录
    输出：
        {
            "insert": {
                "BinaryTree": {"x": [...], "y": [...]},
                "AVLTree": {...},
                ...
            },
            "search_hit": {...},
            "search_miss": {...},
            "erase": {...},
        }
    """
    # op -> container_name -> {"x": [...], "y": [...]}
    data: Dict[str, Dict[str, Dict[str, List[float]]]] = {}

    for r in records:
        name = r.test_func_name
        parts = name.split(".")
        if len(parts) < 3:
            # 不符合 "Container.op.N=xxx" 约定的记录先跳过
            logger.log_error(f"[pipeline] Unexpected test_func_name format: {name}")
            continue

        container = parts[0]
        op = parts[1]

        n_value = None
        for p in parts[2:]:
            if p.startswith("N="):
                try:
                    n_value = int(p.split("N=", 1)[1])
                except ValueError:
                    n_value = None
                break

        if n_value is None:
            logger.log_error(
                f"[pipeline] Failed to parse N from test_func_name: {name}"
            )
            continue

        op_dict = data.setdefault(op, {})
        series = op_dict.setdefault(container, {"x": [], "y": []})
        series["x"].append(float(n_value))
        series["y"].append(float(r.time_usage))

    # 对每条曲线按 N 排序，保证图像是单调向右的折线
    for op, containers in data.items():
        for cname, series in containers.items():
            xs = series["x"]
            ys = series["y"]
            if len(xs) != len(ys):
                logger.log_error(
                    f"[pipeline] length mismatch for op={op}, container={cname}"
                )
                continue
            pairs = sorted(zip(xs, ys), key=lambda t: t[0])
            if pairs:
                series["x"], series["y"] = map(list, zip(*pairs))

    return data


def analyze_and_plot(project_root: Path) -> None:
    """
    从 test-works/logs 中找到最新一份 CSV，
    解析记录并按操作类型绘制 N vs Time 图。
    """
    # 保证 cwd 为工程根目录，这样 metrics.default_logs_directory() 就是 <root>/test-works/logs
    os.chdir(project_root)

    latest_log = metrics.find_latest_log()
    if latest_log is None:
        raise FileNotFoundError(
            "No CSV log file found under test-works/logs. "
            "Have you run the benchmark executable?"
        )

    logger.log_info(f"[pipeline] Using latest log file: {latest_log}")
    records = metrics.load_log(latest_log)
    logger.log_info(f"[pipeline] Loaded {len(records)} records from log")

    time_vs_n_data = build_time_vs_n_data(records)

    # 对每种操作单独画一张图
    for op, curves in time_vs_n_data.items():
        if not curves:
            continue
        title = f"{op} time vs N"
        logger.log_info(f"[pipeline] Scattering figure for op={op}")
        save_path = visualize.scatter_time_vs_n(
            data=curves,
            title=title,
            xlabel="N (elements)",
            ylabel="Time (seconds)",
            save_path=None,
            show=False,
        )
        logger.log_info(f"[pipeline] Figure saved to: {save_path}")


# ============================================================
# CLI
# ============================================================


def parse_args(argv: List[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run C++ benchmarks (via CMake) and analyze the results "
            "using tscripts.metrics & tscripts.visualize."
        )
    )
    parser.add_argument(
        "--build-dir",
        type=str,
        default="build",
        help="CMake build directory (default: build)",
    )
    parser.add_argument(
        "--build-type",
        type=str,
        default="Release",
        choices=["Debug", "Release"],
        help="CMake build type (default: Release)",
    )
    parser.add_argument(
        "--cmake-generator",
        type=str,
        default=None,
        help='Optional CMake generator, e.g. "MinGW Makefiles"',
    )
    parser.add_argument(
        "--skip-configure",
        action="store_true",
        help="Skip CMake configure step",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip CMake build step",
    )
    parser.add_argument(
        "--skip-run",
        action="store_true",
        help="Skip running C++ benchmark executable",
    )
    parser.add_argument(
        "--only-scatter",
        action="store_true",
        help=(
            "Only analyze latest CSV log and scatter figures; "
            "skip CMake configure/build and C++ benchmark run"
        ),
    )
    return parser.parse_args(argv)


def main(argv: List[str] | None = None) -> int:
    args = parse_args(argv)

    # scripts/run_test_pipeline.py → <project_root>
    this_file = Path(__file__).resolve()
    project_root = this_file.parents[1]
    build_dir = project_root / args.build_dir

    logger.log_info(f"[pipeline] Project root: {project_root}")
    logger.log_info(f"[pipeline] Build dir    : {build_dir}")

    try:
        if not args.only_scatter:
            cmake_configure_and_build(
                project_root=project_root,
                build_dir=build_dir,
                build_type=args.build_type,
                cmake_generator=args.cmake_generator,
                skip_configure=args.skip_configure,
                skip_build=args.skip_build,
            )

            if not args.skip_run:
                run_cpp_bench(project_root=project_root, build_dir=build_dir)
            else:
                logger.log_info("[pipeline] Skip running C++ benchmark executable")
        else:
            logger.log_info("[pipeline] --only-plot: skip CMake & C++ benchmark run")

        # 无论是否刚刚跑完 C++，都可以做一次分析，只要有日志文件即可
        analyze_and_plot(project_root=project_root)

        logger.log_info("[pipeline] All steps finished successfully")
        return 0

    except Exception as exc:
        logger.log_error(f"[pipeline] Fatal error: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
