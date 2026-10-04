# mod-race-class-combos

An [AzerothCore](https://www.azerothcore.org/) module (WotLK 3.3.5a) that unlocks additional
race/class combinations.

## What it does

Adds server-side character-creation data for combinations that are normally unavailable, for
example Gnome Priest, Human Hunter, Dwarf Shaman, Orc Mage, Troll Warlock and Undead Paladin.

The heavy lifting is data-driven and minimal: because starting skills and spells are keyed by
race/class **bitmasks** (`playercreateinfo_skills`, `playercreateinfo_spell_custom`), a race's
racials and a class's abilities already apply to any new pairing. The module only needs to add
a **start position** and an **action bar** per combination.

## Client patch required

The 3.3.5a client decides which combinations to *offer* in the character-creation screen from
`CharBaseInfo.dbc`. To see and pick the new combinations you must load a patched
`CharBaseInfo.dbc` on the client (a small MPQ patch). Without it the server accepts the
combinations but the client won't let you select them.

## Configuration

`conf/mod_race_class_combos.conf.dist`:

| Key                          | Default | Description                         |
|------------------------------|---------|-------------------------------------|
| `RaceClassCombos.Announce`   | `1`     | Log a line at startup when active   |

## Installation

Clone into your AzerothCore `modules/` directory and rebuild the worldserver; the SQL applies
automatically on the next start. Then load the client `CharBaseInfo.dbc` patch.

## License

Released under the GNU GPL v2 (or later).
