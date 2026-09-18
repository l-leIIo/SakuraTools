# Version / loader compatibility matrix

SakuraTools uses two independent axes:

- **Minecraft version adapter**: describes Minecraft's Java classes, method descriptors, packet layout, login state machine, and snapshot format.
- **Loader adapter**: describes how the game classes are loaded and remapped by Vanilla, Forge, NeoForge, Fabric, or Quilt.

A loader adapter is not a promise that the protocol is compatible. Forge and Fabric can share Minecraft classes while still changing class loading, mixins, login payloads, and packet handlers.

## Resolver order

Every Java symbol must be resolved in this order:

1. official/Mojmap or stable runtime name;
2. version-specific SRG/MCP name;
3. Yarn/intermediary name;
4. exact JVM descriptor through JVMTI enumeration.

The descriptor fallback is only valid for methods and already-loaded classes. A descriptor cannot reconstruct a Java class name and must never be treated as a wildcard.

## Version families

| Family | Versions | Network connection | Client listener | JVM baseline |
|---|---|---|---|---|
| legacy | 1.8.0–1.12.2 | `NetworkManager` | `NetHandlerPlayClient` | Java 8, normally x64 for this DLL |
| transitional | 1.13.2–1.16.5 | version-specific names; do not reuse legacy packet constructors blindly | version-specific | Java 8–16 |
| modern | 1.17.1–1.21.x | `Connection` | `ClientPacketListener` | Java 16–21 |

The table is a lookup-family description, not a claim that one implementation can safely service every release. Packet construction, login packets, bundle packets, registry sync, and world snapshots still require a per-version implementation.

## Support policy

An adapter may start only after its required classes and method descriptors validate in the live JVM. A class-name hit alone is insufficient. Unsupported or unvalidated combinations must fail closed and leave the Minecraft process untouched.

Production support should be promoted only after testing all of:

- pre-login injection;
- post-login injection and world snapshot;
- B disconnect/reconnect;
- login timeout and snapshot failure;
- packet forwarding in both directions;
- DEL stop/resume;
- representative modded login payloads.

The current native protocol implementation was written around 1.20.1 and therefore must not be advertised as complete support for legacy, transitional, Fabric, Quilt, or NeoForge combinations until their packet and lifecycle adapters are implemented and tested.
