# Autonomous AI ΓÇö Phase 2

This phase adds the external AI transport and basic server-side action execution.

## Architecture

```text
PlayerAI / world thread
        |
        | perception JSON (queued)
        v
ExternalAITransport
        |
        | TCP, newline-delimited JSON
        v
External AI server
        |
        | one action JSON line
        v
ExternalAITransport
        |
        | queued action
        v
AutonomousBotController
        |
        +--> TrinityCore movement
        +--> TrinityCore combat targeting
        +--> future quest/interaction APIs
```

Network I/O is performed on a worker thread. The world thread only builds
perception, queues requests, consumes completed actions, and executes validated
game actions.

## Default endpoint

`127.0.0.1:8765`

Use `ConfigureExternalAI(host, port)` before `StartExternalAI()`.

## Request format

Each request is one JSON object followed by `\n`:

```json
{
  "type": "perception",
  "bot_guid": 123,
  "map_id": 0,
  "level": 10,
  "class_id": 1,
  "health": 1000,
  "max_health": 1000,
  "power": 500,
  "max_power": 500,
  "position": {
    "map_id": 0,
    "x": -9000.0,
    "y": 0.0,
    "z": 80.0,
    "orientation": 1.57
  },
  "quests": []
}
```

## Action format

The AI server returns one JSON object followed by `\n`.

Move:

```json
{
  "action": "move_to",
  "destination": {
    "map_id": 0,
    "x": -8900.0,
    "y": 20.0,
    "z": 80.0,
    "orientation": 0.0
  }
}
```

Attack:

```json
{
  "action": "attack",
  "target_guid": 123456
}
```

Stop:

```json
{"action": "stop"}
```

Other recognized action names are `accept_quest`, `complete_quest`,
`talk_to`, `follow`, and `explore`. Their execution is intentionally
server-gated and will be implemented in the quest/interaction phase.

## Important

The external AI should make coarse decisions. It should NOT issue a command
every combat tick. TrinityCore remains authoritative for movement, combat,
spell rules, quest validation, and world state.


## Phase 11 ΓÇö combat roles

Phase 11 adds a server-side combat coordinator. It remains deliberately high-level: TrinityCore's normal PlayerAI still owns the detailed combat loop, while the coordinator supplies group-level
healing, tank assistance/taunt, and target assistance. The external AI receives a `role` field in
perception (`damage`, `tank`, `healer`, or `ranged`) so it can make role-aware high-level decisions.

Healing and taunt spells are selected only from spells the character actually knows, which keeps the
behavior compatible with different levels and spell-learning states.

## Phases 31-35 ΓÇö world lifecycle

The world simulation layer now exposes navigation stage, resource/objective opportunities,
economy pressure, and a persistent-style life-cycle state. These are planning signals rather than
permission to bypass TrinityCore. Movement, combat, inventory, quest state, and interactions remain
server authoritative.
