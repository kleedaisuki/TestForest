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
│   └─ logs/  # 存放测试结果
│
└─ src/
    ├─ tscripts/
    │   ├─ __init__.py
    │   ├─ logger.py # 日志
    │   ├─ metrics.py # 读取测试结果、做计算
    │   └─ visualize.py # 调用图形库
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
        │   └─ utils.cpp
        │
        └─ main.cpp # 启动并行测试
```

并行测试的结果以 CSV 写到 `/test-works/logs` 目录中；文件取名为`{精确到秒的无空格时间戳}.csv`。CSV 表头为 `test_func_name,count,time_usage`。文件操作使用 `<filesystem>` 中的函数，路径操作跨平台为妙。
