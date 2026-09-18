# Adapter branch contract

Each adapter is a separate compatibility unit selected by `(MinecraftVersion, ModLoader)`.

## Version adapter

Owns:

- Java class candidates;
- method and field candidates;
- protocol family;
- packet constructors and serializers;
- login state transitions;
- world snapshot extraction;
- version-specific Netty pipeline behavior.

## Loader adapter

Owns:

- class-loader discovery;
- remapping namespace detection;
- loader-specific login/custom-payload behavior;
- loader-specific injection timing.

Forge, NeoForge, Fabric, and Quilt must not be treated as interchangeable merely because they expose similarly named Minecraft classes.

## Composition

The runtime first selects a version adapter, then composes the loader policy. It validates symbols before installing hooks and refuses to start if the selected combination cannot be validated.

The current native forwarding implementation still contains 1.20.1-oriented packet and snapshot logic. The adapter registry therefore deliberately reports `binding-validation` until each version family receives its own packet/lifecycle implementation and is tested in a real client.
