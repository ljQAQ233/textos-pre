# 性能问题

- 就本项目而言

## 时钟中断

- `10ms` -> `1ms`

调度时间过长, 可能导致 `block` 时间过多, 然而这个操作也许早就完成了
