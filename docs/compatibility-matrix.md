# Compatibility matrix and adapter manifest

This file is deliberately conservative: a matrix entry describes the names and protocol family that the resolver will try; it does **not** claim that the complete proxy protocol is production-ready for that row.

## Version families

| Family | Minecraft versions | Connection class candidate | Client listener candidate | Common mapping family | Status |
|---|---|---|---|---|---|
| legacy | 1.8.0, 1.8.8, 1.8.9, 1.9.4, 1.10.2, 1.11.2, 1.12.2 | `net.minecraft.network.NetworkManager` | `net.minecraft.client.network.NetHandlerPlayClient` | MCP/SRG | binding validation only |
| transitional | 1.13.2, 1.14.4, 1.15.2, 1.16.5 | `net.minecraft.network.Connection` | `net.minecraft.client.multiplayer.ClientPacketListener` | Mojmap/SRG/Yarn | binding validation only |
| modern | 1.17.1, 1.18.2, 1.18.3, 1.19.2, 1.19.4, 1.20.1, 1.20.2, 1.20.4, 1.20.6 | `net.minecraft.network.Connection` | `net.minecraft.client.multiplayer.ClientPacketListener` | Mojmap/SRG/Yarn | binding validation only |
| modern-1.21 | 1.21.0, 1.21.1, 1.21.3 | `net.minecraft.network.Connection` | `net.minecraft.client.multiplayer.ClientPacketListener` | Mojmap/Yarn | binding validation only |

## Loader matrix

| Loader | Version range represented | Class-loader/remapping path | Adapter status |
|---|---|---|---|
| Vanilla | all listed versions | official/Mojmap or legacy names | binding validation only |
| Forge | 1.8–1.20.x | LaunchWrapper/ModLauncher plus MCP/SRG/Mojmap | binding validation only |
| NeoForge | 1.20.2+ | ModLauncher and Mojmap-style names | binding validation only |
| Fabric | 1.16.5+ | Fabric Loader, intermediary/Yarn | binding validation only |
| Quilt | 1.18.2+ | Quilt Loader, intermediary/Yarn-compatible names | binding validation only |

## Resolver contract

Every symbol is resolved in this order:

1. official/Mojmap name;
2. version-specific SRG or MCP name;
3. Yarn/intermediary name;
4. exact descriptor through JVMTI method enumeration or an already-loaded class signature.

A descriptor does not identify a Java class by itself. Therefore class descriptor fallback only searches already-loaded classes and never guesses a binary name.

## Runtime validation contract

Before hooks are installed, the adapter validates:

- the Minecraft class;
- the connection class;
- the client listener class;
- `Minecraft` instance accessor;
- `Minecraft` client-listener accessor;
- listener connection accessor.

Validation is necessary but insufficient for complete support. Packet constructors, login state, Netty pipeline handlers, world snapshots, registry synchronization, bundle packets, custom payloads, teams, and disconnect/reconnect behavior remain protocol-specific.

## Promotion criteria

A row can only be marked production-ready after real client testing covers both pre-login and post-login injection, snapshot handoff, B reconnect, packet forwarding, stop/resume, and representative modded login payloads. Until then, the runtime must fail closed rather than advertise unsupported compatibility.
