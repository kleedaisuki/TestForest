```text
/
├─ README.md
├─ LICENSE
├─ .gitignore      
│
├─ requirements.txt
├─ pyproject.toml
│
├─ scripts/        
|   ├─ install-dev.ps1                     
│   └─ run_test_pipeline.py    # 创建子进程和测试流水线入口    
│   
├─ CMakeLists.txt # 构建
│
├─ test-works/
│   │
│   ├─ cases/ # 以 txt 存放生成的测试用例   
│   │
│   └─ logs/  # 存放测试结果
│
└─ src/
    ├─ tscripts/
    │   ├─ __init__.py
    │   ├─ logger.py # 日志
    │   ├─ metrics.py # 读取测试结果、做计算
    │   ├─ visualize.py # 调用图形库
    │   └─ generation.py # 生成测试用例，种子可选
    │
    └─ proj/
        │
        ├─ headers/
        │   ├─ utils.hpp # 测时工具、日志与并发IO
        │   ├─ Binary-Tree.hpp # 二叉树
        │   ├─ B-Tree.hpp # B树
        │   ├─ AVL-Tree.hpp # AVL树
        │   └─ Red-Black-Tree.hpp # 红黑树
        │
        ├─ src/
        │   ├─ utils.cpp
        │   ├─ Binary-Tree.cpp
        │   ├─ B-Tree.cpp
        │   ├─ AVL-Tree.cpp
        │   └─ Red-Black-Tree.cpp
        │
        └─ main.cpp # 启动并行测试
```
