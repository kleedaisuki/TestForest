# 🌲 TestForest — 多种树结构的并行性能基准与可视化套件

> ⚡ 这是一个面向算法与系统性能研究的实验平台，基于 C++17 实现多类树结构并行基准测试，并使用 Python 工具链完成数据分析与可视化。

---

## ✨ 项目亮点（Overview）

* **四大树结构容器（C++17）**

  * Binary Tree（二叉树）
  * AVL Tree（AVL 平衡树）
  * Red-Black Tree（红黑树）
  * B-Tree (B 树，模板阶数可调）
* **统一接口、仿 `std::set` 风格**
* **并行性能基准（Parallel Benchmarking）**
  自动对不同 N 的 `insert / search_hit / search_miss / erase` 进行基准测试
  （参见 main.cpp 的 `run_all_benchmarks` 实现）
* **CSV 日志自动输出**（跨平台 filesystem 实现）
* **Python 分析流水线（build → run → visualize）**
  `scripts/run_test_pipeline.py` 自动构建 C++ 程序并绘图
* **完全自动化的可视化图表（Matplotlib）**
  使用散点图展示 `N vs Time` 性能走势

---

## 📁 目录结构（Project Structure）

（同步于 STRUCTURE.md）

```
/
├─ README.md
├─ LICENSE
├─ .gitignore      
│
├─ requirements.txt
├─ pyproject.toml
│
├─ scripts/        
│   ├─ install-dev.ps1               
│   └─ run_test_pipeline.py    
│
├─ CMakeLists.txt
│
├─ test-works/
│   └─ logs/      # 基准测试输出的 CSV
│
└─ src/
    ├─ tscripts/           # Python 工具链
    │   ├─ logger.py       # 文本日志 & CSV 日志
    │   ├─ metrics.py      # 读取 CSV
    │   └─ visualize.py    # 绘图组件（散点图 N vs Time）
    │
    └─ proj/
        ├─ headers/
        │   ├─ utils.hpp
        │   ├─ Binary-Tree.hpp
        │   ├─ B-Tree.hpp
        │   ├─ AVL-Tree.hpp
        │   └─ Red-Black-Tree.hpp
        │
        ├─ src/
        │   └─ utils.cpp
        │
        └─ main.cpp
```

---

## 🖥 环境要求（Environment Requirements）

本项目目前 **已在以下环境上验证通过**：

* Windows 10/11
* MSYS2 + MinGW-w64 (GCC, C++17)
* CMake ≥ 3.16
* Python 3.11 + pip + venv

只要有一个支持 C++17 的编译器和 CMake，**理论上也应当可以在 Linux / macOS 上构建**：

* GCC ≥ 9 或 Clang ≥ 10
* CMake ≥ 3.16

> ⚠️ 注意：Linux / macOS 上暂未系统性测试，可能需要：
>
> * 通过 `find_package(Threads)` 显式链接线程库
> * 在老版本 GCC 上显式链接 `stdc++fs` 以支持 `std::filesystem`

---

## ⚡ install-dev（Windows 一键开发环境初始化）

在 Windows + PowerShell 环境下，可以使用：

```powershell
./scripts/install-dev.ps1


## 🚀 启动完整测试流水线（Python）

推荐通过 Python 脚本一键完成 **configure → build → run → scatter**：

```bash
python ./scripts/run_test_pipeline.py
```

脚本将：

1. 调用 CMake 生成构建目录
2. 编译 C++ 基准程序
3. 运行基准程序 → 生成 `test-works/logs/*.csv`
4. 自动绘制 `N vs Time` 散点图 → 输出到 `test-works/figs/`

如果只想绘图（跳过 C++）：

```bash
python ./scripts/run_test_pipeline.py --only-scatter
```

脚本核心逻辑参见 `run_test_pipeline.py` 中的 `main()`。

---

## ⚙️ C++ 构建

### 配置 + 构建（CMake）

Windows + MinGW-w64：

```bash
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release .
cmake --build .
```

Linux / macOS：

```bash
cmake -DCMAKE_BUILD_TYPE=Release .
cmake --build .
```

生成的可执行文件位于：

```
build/bin/test_forest_bench
```

---

## 📊 性能指标格式（CSV）

所有基准结果写入：

```
test-works/logs/{timestamp}.csv
```

CSV 表头：

```
test_func_name,count,time_usage
```

例如：

```
BinaryTree.insert.N=100,100,0.000723001
BinaryTree.search_hit.N=100,100,0.000312000
```

C++ 写日志由 `utils::CsvLogger` 实现。
Python 解析对应 `tscripts/metrics.py`。

---

## 🎨 可视化（Visualization）

使用 Matplotlib 绘制散点图。
对应函数为 `scatter_time_vs_n()`：

图像自动保存到：

```
test-works/figs/{title}_P{timestamp}.png
```

图例区分容器类型，例如：BinaryTree / AVL / RedBlackTree / BTreeSet。

---

## 🧪 如何扩展（Extensions）

你可以：

* 添加新的容器，仿照 `BinaryTree`, `AVLTree` 的接口样式
* 扩展 Python 可视化以支持 QPS、吞吐量、内存占用
* 替换为 GPU/TPU 后端进行更高维度评测
* 为 C++ 容器添加 `iterator`、`emplace` 等更标准库化接口
* 扩展 metrics 为“多日志合并分析工具”

---

## 📜 许可证（License）

GPL-3.0
