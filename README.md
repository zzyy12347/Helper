# 物品店铺查询

这是一个 C++ / Qt Widgets 桌面软件原型，用来按物品类目筛选店铺报价。

## 已实现功能

- 三级下拉类目筛选，类目来自物品的标签，例如 `召唤物/地府/普通`
- 搜索结果显示：物品、店铺编号、价格、库存、类目
- 价格从低到高排序
- 标记没货后，该店铺报价会排到最后
- 支持恢复有货
- 支持新增、编辑、删除物品
- 支持编辑每个物品的店铺报价
- 支持窗口置顶
- 数据保存到本机应用数据目录的 `items.json`

## 数据编辑格式

类目标签用 `/` 分隔：

```text
召唤物/地府/普通
```

店铺报价每行一条，用英文逗号分隔：

```text
1001,88,有货
1002,76,没货
```

## 编译运行

需要先安装 Qt 5 或 Qt 6 的开发环境。

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="你的Qt安装路径"
cmake --build build --config Release
```

如果 Qt 已经加入系统路径，通常可以直接运行：

```powershell
cmake -S . -B build
cmake --build build --config Release
```

