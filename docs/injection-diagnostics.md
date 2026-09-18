# SakuraTools 注入前诊断清单

## 当前已修复/需要注意的问题

1. **目标选择**：存在多个 Java/Minecraft 进程时不能默认选最新窗口；现在应先列出 PID、版本和加载器，再由用户选择。
2. **配置时序**：必须在第一次加载 DLL 之前写入目标 profile；否则 DLL 只能看到旧配置。重新激活驻留 DLL 不会加载磁盘上的新 DLL。
3. **版本识别来源**：安装目录中最高版本不等于当前运行版本；应优先读取目标进程的命令行和 JVM 类，再使用 `.minecraft/versions` 作为回退。
4. **跨位数**：注入器、DLL 和目标 JVM 必须同为 x64。x86 Java/Minecraft 不能由当前 x64 构建安全注入。
5. **权限**：目标以管理员运行时，注入器也必须以管理员运行；必要时需要 SeDebugPrivilege。
6. **路径编码**：当前远程 LoadLibrary 路径使用 ANSI 字节；包含非 ASCII 路径可能失败，应使用 ASCII 路径或改成 Unicode 注入路径。
7. **端口冲突**：25565 被其他程序占用时，代理不能启动；目前没有自动改端口机制，B 端必须使用同一个端口配置。
8. **旧版本协议**：仅替换类名不足以支持 1.8–1.16；登录、区块、玩家列表、队伍、Bundle、注册表和自定义 Payload 需要协议族适配。
9. **加载器差异**：Forge、NeoForge、Fabric、Quilt 不能仅凭同名 Minecraft 类视为兼容；必须识别 ClassLoader、映射命名空间和登录 Payload。
10. **JNI/JVMTI**：Java 8–21 的 JVMTI 能力、类初始化和线程行为不同；JVMTI 缺失时必须失败关闭。
11. **类解析**：descriptor 不是万能类名。只能枚举已加载类，不能据此推导任意未加载类。
12. **动态类生成**：生成的 class 文件版本和父类方法在旧 JVM 上可能不兼容，需要按 Java 版本选择 classfile major version。
13. **快照一致性**：A 主线程快照和实时增量必须保持同一版本的包顺序；不能跨版本复用 1.20.1 的构造器。
14. **Hook 清理**：部分 Hook 安装成功、后续步骤失败时必须完整回滚；否则重试会重复注册 handler 或连接 Hook。
15. **线程锁**：停止时不能让 JVM 主线程等待持有 dispatch/client/target 锁的 native 线程，否则会死锁。
16. **自定义模组包**：未知 Forge/Fabric/NeoForge/Quilt 登录 Payload 不能盲转发或丢弃，应按协议策略处理。
17. **自动识别准确性**：命令行不包含版本时只能返回 auto，不能猜成最高已安装版本。
18. **B 客户端版本**：B 必须使用与 A/代理协议兼容的 Minecraft 版本；代理不是通用跨协议转换器。
19. **GUI 窗口名**：窗口名不应作为唯一识别条件；无窗口、启动器包装或多个实例都会导致误选。
20. **注入器退出码**：LoadLibrary 返回 NULL、远程线程超时、目标进程提前退出，都必须分别报告，不能只显示“注入成功”。

## 尚未达到生产可用的根本原因

当前核心 `native/server/*` 和 `native/world_snapshot.cpp` 仍以 1.20.1 的数据包和 JNI 结构为中心。适配器表可以阻止明显错误注入，但不能自动生成旧版本数据包或完成不同加载器的登录握手。因此这些组合在完成对应协议实现前必须标记为 binding-validation/unsupported，而不是 production-ready。
