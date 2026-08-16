# PLand

**[简体中文](README.md)** | **[English](README_EN.md)**

[![CI](https://img.shields.io/github/actions/workflow/status/IceBlcokMC/pland/build.yml?branch=main&style=for-the-badge&logo=github&label=CI)](https://github.com/IceBlcokMC/pland/actions)
[![License](https://img.shields.io/badge/license-AGPLv3-blue?style=for-the-badge&logo=open-source-initiative)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![Release](https://img.shields.io/github/v/release/IceBlcokMC/pland?include_prereleases&style=for-the-badge)](https://github.com/IceBlcokMC/pland/releases)

---

PLand is a high-performance land management plugin developed for the **LeviLamina** ecosystem. It provides fine-grained claim protection, multi-role permission controls, a flexible economy and leasing system, data migration support, and comprehensive third-party extension APIs for BDS (Bedrock Dedicated Server).

---

## ✨ Feature Overview

### 📐 Multiple Claim Types

- **2D Claim**: A rectangular area covering the **entire vertical height** of a dimension.
- **3D Claim**: A rectangular volume bounded within a **specified height range**.
- **Sub-claims**: Create nested child claims within a parent claim, supporting multi-level nesting.

### 🛠️ Convenient Utilities

- **Resizing**: Freely adjust the boundaries of an existing claim with automatic cost/refund adjustments.
- **Claim Teleports**: Set custom spawn points inside claims for quick fast-travel.
- **Claim Transfer**: Owners can transfer land ownership directly to other players.
- **Boundary Visualization**: Render visual borders in-game for easy claim boundary viewing.
- **Permission Control**: Fine-tune player action permissions within the claim.
- **Claim Aliases**: Set custom names to quickly identify and organize different claims.

> Check the in-game management menu for more features.

### ⚙️ Customizable Management

- **Mode Switching**: Supports both **One-Time Purchase** and **Subscription Leasing** modes.
- **Rules & Constraints**: Configure restricted/whitelisted dimensions, lease-only zones, claim spacing, min/max spatial dimensions, max claim limits per player, and max sub-claim depth.

> For additional configuration parameters, check the config files and documentation [Click Here](https://iceblcokmc.github.io/PLand/).

### 👥 4-Role Permission Model

Utilizes a clear 4-tier role hierarchy with permission inheritance to sub-claims or local overrides:

1. **Admin**: Holds global master permissions and access to a dedicated admin interface.
2. **Owner**: Controls claim settings, assigns member permissions, and manages ownership transfers.
3. **Member**: Granted fine-grained, action-specific permissions configured by the owner.
4. **Entity**: The default public group controlling permissions for guests and non-player entities.

### 🛡️ Fine-Grained Protection (60+ Permission Nodes)

PLand offers precise controls over both **Player Actions** and **Environmental Events**:

| Player Action Control                     | Environmental Event Control                                 |
| :---------------------------------------- | :---------------------------------------------------------- |
| Block Placement / Destruction             | Explosion Damage (TNT / Creeper)                            |
| Container Access (Chests / Barrels, etc.) | Fire Spread / Moss Growth                                   |
| Workstation & Special Block Interaction   | Mob Spawning / Dripstone / Piston Movement                  |
| PvP Combat & Ranged Weapon Usage          | Liquid Flow (Water / Lava)                                  |
| Entity Interaction / Vehicle Riding       | Sculk Spread / Dragon Egg Teleportation                     |
| Projectile Launching / Item Dropping      | Wither Destruction / Lightning Strikes / Farmland Trampling |

> Showing partial permission nodes only. View the complete list in-game.

### 💰 Economy & Pricing System

- **Dual Economy Engines**: Flexibly integrates with server economy plugins (Scoreboard, LegacyMoney, etc.).
- **Custom Formulas**: Independent pricing formulas and multipliers configurable per claim type and dimension.
- **Discounts & Refunds**: Full configuration support for purchase discounts and claim surrender/lease refund rates.

### 💻 Interface & Usability

- **GUI Forms**: Fully encapsulated GUI menus providing seamless player interaction, advanced search filters, and dedicated admin forms.
- **Command Support**: Comprehensive CLI commands for creating, purchasing, and drawing claims.
- **Multilingual (i18n)**：Native Simplified Chinese support, along with built-in language packs for American English, Russian, and more.

> Explore more features in the in-game management menu and command list.

---

## 🔧 Dynamic Permission Mapping

PLand features a **Dynamic Permission Mapping** mechanism. Blocks, items, and entity types can be directly mapped to specific permission nodes via configuration files:

```json
{
  "minecraft:barrel": "useContainer",
  "minecraft:lectern": "useLectern",
  "minecraft:bee_nest": "useBeeNest"
}
```

> For details, refer to the configuration guide and documentation [Click Here](https://iceblcokmc.github.io/PLand/).

---

## 🛡️ Multi-Layer Interception Architecture

To maximize coverage of game behaviors and fix bypass vulnerabilities in standard event handlers, PLand builds a multi-layered defense architecture:

- **Event Layer**: Built on the loader ecosystem, covering 40+ low-level standard events, including block modifications, player interactions, entity behaviors, mob spawning, and combat damage.
- **Hook Layer**: Uses low-level C++ Hooks to intercept mechanics unhandled by standard event systems (e.g., cross-border hopper extraction, lightning strikes, farmland trampling, and special block/entity interactions).
- **Configurable Toggles**: Every listener and hook can be toggled individually for performance tuning and troubleshooting compatibility.

---

## ⚡ Performance & Data Storage

- **Storage Architecture**: Uses a **Memory Cache + LevelDB Persistence** dual-layer architecture. Main thread reads and writes execute in memory, while background threads asynchronously persist data to the database.
- **Algorithm Optimization**: Employs spatial hash indexes for claim queries, precalculated sub-claim hierarchies, and bidirectional mapping tables to accelerate complex claim searches and deletions.
- **Data Versioning**: All claim data includes version metadata. During plugin upgrades, the system automatically migrates legacy data structures smoothly to ensure backward compatibility.

---

## 🔌 Developer Ecosystem & API

PLand provides public Service interfaces and event registration mechanisms for third-party plugin integration:

- **Core API**: Query claim details, perform permission checks, retrieve ownership relationships, and access runtime data.
- **DevTool Package (`src-devtool`)**: Includes claim visualization overlays, real-time boundary editing, and visual tree hierarchy analysis for claims.

---

## 📂 Project Structure

```bash
PLand
├─ assets/              # Plugin resources and i18n JSON language packs
├─ docs/                # User documentation and developer guides
├─ src/
│  └─ pland/
│     ├─ aabb/          # Spatial and collision calculations
│     ├─ drawer/        # Boundary rendering and visual drawing
│     ├─ economy/       # Dual economy plugin adapters
│     ├─ events/        # Domain, economy, and player events
│     ├─ gui/           # Player and Admin GUI forms
│     ├─ infra/         # Infrastructure and data Migrator
│     ├─ internal/      # Command handlers and multi-layer Interceptors
│     ├─ land/          # Core claim logic, repository, and validators
│     ├─ selector/      # Selection tools
│     └─ service/       # Open low-level services and API interfaces
└─ src-devtool/         # Developer visual analysis and debugging tools

```

---

## 📖 Documentation & Support

If you encounter issues, discover bugs, or have feature ideas, feel free to contribute:

- 📘 Detailed Documentation: [PLand Docs Site](https://iceblcokmc.github.io/PLand/)
- 🐞 Report Issues: [GitHub Issues](https://github.com/IceBlcokMC/PLand/issues)
- 💬 Community Discussions: [GitHub Discussions](https://github.com/IceBlcokMC/PLand/discussions)

---

## 📄 License

This project is licensed under the [AGPL-3.0 or later](https://www.google.com/search?q=LICENSE) License.

> **Disclaimer**: The developers are not responsible for any loss or damage resulting from the use of this software. By using this project and its derivatives, you agree to assume all associated risks.

[![Star History Chart](https://api.star-history.com/svg?repos=IceBlcokMC/PLand&type=Date)](https://star-history.com/#IceBlcokMC/PLand&Date)
