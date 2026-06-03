# AnDrop

AnDrop 是一个使用 Qt5 + C++ 开发的局域网文件传输工具。

当前项目严格遵循以下文档推进：

- `docs/superpowers/specs/2026-06-03-androp-design.md`
- `docs/superpowers/plans/2026-06-03-androp-mvp.md`

## 构建

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
