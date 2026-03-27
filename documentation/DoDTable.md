# Dragons of Destiny — Table Specification

> **Purpose:** This document teaches a new programmer how the *Dragons of Destiny* (DoD) pinball table is implemented so they can confidently build their own table using the RasPin engine. It covers the gameplay overview, all state machines with transition diagrams, I/O pin assignments, per-player data structures, the screen-manager system, and for each mode a sample AI prompt you could use to generate that mode's code from scratch.

---

## Table of Contents

1. [Gameplay Overview](#1-gameplay-overview)
2. [Hardware I/O Map](#2-hardware-io-map)
3. [Per-Player State (pbGameState)](#3-per-player-state-pbgamestate)
4. [Architecture Fundamentals](#4-architecture-fundamentals)
   - 4.1 [The Three-Function Pattern](#41-the-three-function-pattern)
   - 4.2 [The Screen Manager](#42-the-screen-manager)
5. [Top-Level State Machine](#5-top-level-state-machine)
6. [Mode: PBTBL_INIT](#6-mode-pbtbl_init)
7. [Mode: PBTBL_START (Attract / Start Screen)](#7-mode-pbtbl_start-attract--start-screen)
8. [Mode: PBTBL_MAIN (Gameplay)](#8-mode-pbtbl_main-gameplay)
   - 8.1 [Main Screen Sub-States](#81-main-screen-sub-states)
   - 8.2 [Scoring Rules](#82-scoring-rules)
   - 8.3 [Inn Lane System](#83-inn-lane-system)
   - 8.4 [Key Target System](#84-key-target-system)
   - 8.5 [Inlane / Ball-Save System](#85-inlane--ball-save-system)
   - 8.6 [Extra Ball System](#86-extra-ball-system)
   - 8.7 [Ball Drain and Player Rotation](#87-ball-drain-and-player-rotation)
   - 8.8 [Status Panel (Right Side)](#88-status-panel-right-side)
   - 8.9 [Score Display and Animation](#89-score-display-and-animation)
9. [Mode: PBTBL_PLAYEREND (Between-Turn Transition)](#9-mode-pbtbl_playerend-between-turn-transition)
10. [Mode: PBTBL_RESET (Reset / Confirmation)](#10-mode-pbtbl_reset-reset--confirmation)
11. [Mode: PBTBL_GAMEEND (High Score Entry)](#11-mode-pbtbl_gameend-high-score-entry)
12. [Adding a New Table Mode — Checklist](#12-adding-a-new-table-mode--checklist)

---

## 1. Gameplay Overview

Dragons of Destiny is a fantasy-themed pinball table for 1–4 players. Each player is an adventurer trying to unlock the dungeon, recruit party members, and accumulate the highest score across a configurable number of balls (default: 3).

**Core gameplay loops:**

| Loop | How it works |
|------|-------------|
| **Pop bumpers** | 3 pops on the upper playfield. Each hit scores 50 pts and adds +1 gold. Auto-fire solenoid triggers immediately via the auto-output system. |
| **Slingshots** | L/R slings score 1,000 pts each hit and play a sword-cut sound. Every 10,000-point milestone earned from slings awards an **extra ball** and activates **ball save**. |
| **Inn lanes** | 3 lanes (Inn1/Inn2/Inn3) at the top. Completing all 3 scores 250 pts each, sets `bInnOpen = true`, flashes "Inn Open!" for 2 s, and resets the lane LEDs. Flippers **rotate** the lane LEDs (left flipper rotates left, right flipper rotates right). |
| **Key targets** | 3 standup targets (Key1/Key2/Key3). Completing all 3 scores 250 pts each, sets `bKeyObtained = true`, and flashes "Key Obtained!" for 2 s. Flipper buttons do **not** rotate key LEDs. |
| **Inlanes + ball save** | Hitting both left and right inlane sensors lights the SAVE LED and activates a 5-second ball-save timer. If the ball drains while ball save is active it is returned, a "Ball Saved" message is shown, and the extra-ball flag is cleared. |
| **Character recruit** | Three party members (Knight/Priest/Ranger) with levels tracked in the player state. The status panel shows headshots grayed out until recruited. *(Full recruitment mechanic not yet implemented — state is tracked, display is live.)* |
| **Dungeon** | Floor and level tracked per player. Displayed in the status panel. |

**Attract sequence (no game running):**  
Castle doors → animated torches → "Press Start" blink → cycles through Instructions and High Score screens every 18 s.

---

## 2. Hardware I/O Map

### Outputs

| ID | Board | Pin | Purpose |
|----|-------|-----|---------|
| `IDO_STARTLED` | RASPI 0 | 23 | Start button LED |
| `IDO_RSLING` | IO 0 | 0 | Right slingshot solenoid (250 ms pulse) |
| `IDO_LSLING` | IO 0 | 1 | Left slingshot solenoid (250 ms pulse) |
| `IDO_LFLIP` | IO 0 | 2 | Left flipper solenoid (100 ms) |
| `IDO_RFLIP` | IO 0 | 3 | Right flipper solenoid (100 ms) |
| `IDO_EJECT` | IO 0 | 4 | Ball hopper ejector solenoid (200 ms) |
| `IDO_NEOPIXEL0` | NEOPIXEL 0 | 10 | NeoPixel LED chain |
| `IDO_LSLINGLED` | LED 0 | 0 | Left slingshot LED |
| `IDO_RSLINGLED` | LED 0 | 1 | Right slingshot LED |
| `IDO_LINLANELED` | LED 0 | 2 | Left inlane LED |
| `IDO_RINLANELED` | LED 0 | 3 | Right inlane LED |
| `IDO_SAVELED` | LED 0 | 4 | Ball save LED |
| `IDO_POP1` | IO 1 | **9** | Pop1 solenoid (50 ms pulse) |
| `IDO_POP2` | IO 1 | **10** | Pop2 solenoid (50 ms pulse) |
| `IDO_POP3` | IO 1 | **11** | Pop3 solenoid (50 ms pulse) |
| `IDO_INN1LED` | LED 1 | 0 | Inn lane 1 LED |
| `IDO_INN2LED` | LED 1 | 1 | Inn lane 2 LED |
| `IDO_INN3LED` | LED 1 | 2 | Inn lane 3 LED |
| `IDO_KEY1LED` | LED 1 | 3 | Key target 1 LED |
| `IDO_KEY2LED` | LED 1 | 4 | Key target 2 LED |
| `IDO_KEY3LED` | LED 1 | 5 | Key target 3 LED |

> **Design note:** Pop solenoids (outputs, pins 9–11) and pop sensors (inputs, pins 0–2) are on the same IO board (boardIdx 1) but on **different pins**. Inputs occupy pins 0–8; outputs use pins 9–11.

### Inputs

| ID | Board | Pin | Msg Type | Purpose |
|----|-------|-----|----------|---------|
| `IDI_LFLIP` | RASPI 0 | 27 | BUTTON | Left flipper button |
| `IDI_RFLIP` | RASPI 0 | 17 | BUTTON | Right flipper button |
| `IDI_LACTIVATE` | RASPI 0 | 5 | BUTTON | Left activate button |
| `IDI_RACTIVATE` | RASPI 0 | 22 | BUTTON | Right activate button |
| `IDI_START` | RASPI 0 | 6 | BUTTON | Start / add player button |
| `IDI_RESET` | RASPI 0 | 24 | BUTTON | Reset button |
| `IDI_LINLANE` | IO 0 | 6 | SENSOR | Left inlane sensor |
| `IDI_RINLANE` | IO 0 | 5 | SENSOR | Right inlane sensor |
| `IDI_BALLDRAIN` | IO 0 | 7 | SENSOR | Ball drain sensor |
| `IDI_BALLREADY` | IO 0 | 8 | SENSOR | Ball-ready sensor (hopper) |
| `IDI_BALLDELIVERED` | IO 0 | 9 | SENSOR | Ball-delivered sensor |
| `IDI_RSLING` | IO 0 | 10 | SLING | Right slingshot sensor (auto-fires `IDO_RSLING`) |
| `IDI_LSLING` | IO 0 | 11 | SLING | Left slingshot sensor (auto-fires `IDO_LSLING`) |
| `IDI_POP1` | IO 1 | 0 | POPBUMPER | Pop 1 sensor (auto-fires `IDO_POP1`) |
| `IDI_POP2` | IO 1 | 1 | POPBUMPER | Pop 2 sensor (auto-fires `IDO_POP2`) |
| `IDI_POP3` | IO 1 | 2 | POPBUMPER | Pop 3 sensor (auto-fires `IDO_POP3`) |
| `IDI_INN1` | IO 1 | 3 | SENSOR | Inn lane 1 |
| `IDI_INN2` | IO 1 | 4 | SENSOR | Inn lane 2 |
| `IDI_INN3` | IO 1 | 5 | SENSOR | Inn lane 3 |
| `IDI_KEY1` | IO 1 | 6 | TARGET | Key target 1 |
| `IDI_KEY2` | IO 1 | 7 | TARGET | Key target 2 |
| `IDI_KEY3` | IO 1 | 8 | TARGET | Key target 3 |

**Simulator key mapping:**

```
Z=Start  A=LFlip  D=RFlip  Q=LActivate  E=RActivate  C=Reset
8=Pop1   9=Pop2   0=Pop3
I=Inn1   O=Inn2   P=Inn3
T=Key1   Y=Key2   U=Key3
1=LInlane  2=RInlane  3=Drain  4=BallReady  5=BallDelivered
6=RSling   7=LSling
```

---

## 3. Per-Player State (pbGameState)

Each of the 4 player slots is an instance of `pbGameState` stored in `m_playerStates[4]`. Key fields:

| Field | Type | Description |
|-------|------|-------------|
| `score` | `unsigned long` | Current total score |
| `inProgressScore` | `unsigned long` | Score at start of current animated count-up |
| `currentBall` | `unsigned int` | Current ball number (1-based) |
| `enabled` | `bool` | False once all balls are exhausted |
| `inGame` | `bool` | Was this player added to the current game |
| `ballSaveEnabled` | `bool` | Ball-save is currently active |
| `extraBallEnabled` | `bool` | Player has earned an extra ball |
| `lastExtraBallThreshold` | `unsigned long` | Last 10k milestone that triggered extra ball |
| `knightJoined / priestJoined / rangerJoined` | `bool` | Party member recruited |
| `knightLevel / priestLevel / rangerLevel` | `int` | Party member levels (start at 1) |
| `goldValue` | `int` | Accumulated gold (from pop bumpers) |
| `attackValue` | `int` | Attack stat (sword icon) |
| `defenseValue` | `int` | Defense stat (shield icon) |
| `dungeonFloor` | `int` | Current dungeon floor (start at 1) |
| `dungeonLevel` | `int` | Dungeon difficulty level (start at 1) |
| `bInnOpen` | `bool` | All 3 inn lanes have been completed |
| `bKeyObtained` | `bool` | All 3 key targets have been hit |
| `tableState` | `stPlayerTableState` | Inlane LED state (reset each ball) |
| `modeState` | `ModeState` | Mode system sub-states |

`pbGameState::reset(ballsPerGame)` is called on each player slot at game start and when they are disabled.

---

## 4. Architecture Fundamentals

### 4.1 The Three-Function Pattern

Every table mode (`PBTBL_X`) is implemented in its own file (`Pinball_Table_ModeX.cpp`) using exactly three functions:

```
pbeLoadXxx()         — Run once. Load textures, create animations, etc.
                       Guard-flagged: returns immediately if already loaded.

pbeRenderXxx()       — Called every frame. Calls Load, then draws the screen.

pbeUpdateStateXxx()  — Called with each input message. Changes m_tableState,
                       m_tableSubScreenState, and issues screen requests.
```

The main dispatch in `Pinball_Table.cpp` routes to these three functions via `pbeRenderGameScreen()` and `pbeUpdateGameState()`.

**To add a new mode:**
1. Add a value to the `PBTableState` enum in `Pinball_Table.h`.
2. Create `tablemodes/Pinball_Table_ModeX.h` (sub-state enum) and `Pinball_Table_ModeX.cpp` (the three functions).
3. Include the new header in `Pinball_Table.h`.
4. Add function declarations in `Pinball_Engine.h`.
5. Add dispatch `case` blocks in `pbeRenderGameScreen()` and `pbeUpdateGameState()`.
6. Add the `.cpp` file to `CMakeLists.txt`.

### 4.2 The Screen Manager

The screen manager decouples *what state the game is in* from *what is currently displayed*. This lets high-priority events (extra ball, ball saved, inn open) temporarily overlay gameplay without changing the game state.

```
pbeRequestScreen(state, subState, priority, durationMs, canBePreempted)
   — Queues a screen request. Only one priority-0 request exists at a time
     (permanent background). Higher-priority requests display on top and
     auto-expire after durationMs milliseconds.

pbeUpdateScreenManager(currentTick)
   — Updates the queue: removes expired requests, sorts by priority,
     selects the top entry as m_currentDisplayedTableState.

pbeGetCurrentScreenState() / pbeGetCurrentSubScreenState()
   — Return whatever is currently visible (may differ from m_tableState).

pbeClearScreenRequests()
   — Clears the entire queue (used at state transitions).
```

**Priority levels:**

| Level | Enum | Used for |
|-------|------|---------|
| 0 | `PRIORITY_LOW` | Persistent background (the current game state) |
| 1 | `PRIORITY_MEDIUM` | Mode transitions, bonus screens |
| 2 | `PRIORITY_HIGH` | Ball saved, extra ball, inn open, key obtained |
| 3 | `PRIORITY_CRITICAL` | Cannot be preempted |

---

## 5. Top-Level State Machine

```
          ┌────────────┐
          │ PBTBL_INIT │  (entry point on engine start)
          └─────┬──────┘
                │ immediately (first render frame)
                ▼
          ┌─────────────┐
     ┌───▶│ PBTBL_START │◀────────────────────────────────────┐
     │    └─────┬───────┘                                     │
     │          │ START button pressed → door animation done  │
     │          ▼                                             │
     │    ┌─────────────┐   RESET button ──┐                 │
     │    │  PBTBL_MAIN │◀─────────────────┼─────────────┐   │
     │    └──────┬──────┘                  │             │   │
     │           │ ball drain (no save)    ▼             │   │
     │           │              ┌──────────────────┐     │   │
     │           │              │   PBTBL_RESET    │     │   │
     │           │              └──────────────────┘     │   │
     │           │                  │ RESET again: → PBTBL_START
     │           │                  │ any other btn: restore state
     │           ▼                  │
     │    ┌──────────────────┐      │
     │    │ PBTBL_PLAYEREND  │      │
     │    └──────┬───────────┘      │
     │           │ ball delivered   │
     │           └──────────────────┘
     │    ┌──────────────────┐
     │    │  PBTBL_GAMEEND   │
     │    └──────┬───────────┘
     │           │ timer (20s) or any button
     └───────────┘
```

The RESET button is intercepted in `pbeUpdateGameState()` **before** the mode dispatch, from **any** state except `PBTBL_RESET` itself. It saves `m_StateBeforeReset` and navigates to `PBTBL_RESET`.

---

## 6. Mode: PBTBL_INIT

**File:** `Pinball_Table_ModeInit.cpp`

**Purpose:** One-time hardware and engine initialization on startup.

**What it does:**
- Creates and registers the ball hopper ejector device (`pbdHopperEjector`) watching `IDI_BALLREADY` / `IDO_EJECT` / `IDI_BALLDELIVERED`.
- Sets all NeoPixels to black.
- Immediately transitions `m_tableState` to `PBTBL_START` on the first render frame.

**State diagram:**

```
[Engine Start]
     │
     ▼
PBTBL_INIT ──(first render)──► PBTBL_START
```

No sub-states. No user input handling.

### Sample AI Prompt — PBTBL_INIT

```
Create the PBTBL_INIT mode for my RasPin pinball table. It should initialize hardware
(register the ball hopper ejector device, blank all NeoPixels) and then immediately
transition to PBTBL_START. Use the Dragons of Destiny table as a reference.
```

---

## 7. Mode: PBTBL_START (Attract / Start Screen)

**File:** `Pinball_Table_ModeStart.cpp`

**Sub-states (`PBTBLStartScreenState`):**

| Value | Name | Description |
|-------|------|-------------|
| 0 | `START_START` | "Press Start" blink (default, timeout resets here) |
| 1 | `START_INST` | Instructions screen |
| 2 | `START_SCORES` | High score board |
| 3 | `START_OPENDOOR` | Door animation playing → transitions to MAIN |

**State diagram:**

```
         ┌──────────────────────────────────────────────────┐
         │                   (18 s timeout)                 │
         ▼                                                  │
  ┌─────────────┐  any btn    ┌────────────┐  any btn   ┌──────────────┐
  │ START_START │────────────▶│ START_INST │───────────▶│ START_SCORES │
  └──────┬──────┘             └──────┬─────┘            └──────┬───────┘
         │ START btn                  │ START btn               │ START btn
         ▼                            ▼                         ▼
  ┌──────────────────────────────────────────────────────────────────────┐
  │                       START_OPENDOOR                                 │
  │   Left door animates left, right door animates right (1.25s)        │
  │   On completion: m_tableState = PBTBL_MAIN, Player 1 enabled,       │
  │   hopper starts, auto-outputs enabled, music switches                │
  └──────────────────────────────────────────────────────────────────────┘
```

**Key details:**
- Backglass, dungeon background, animated flame torches, and door assets are loaded once.
- "Press Start" text fades in with a color animation (2s). Blinks 2 s on / 0.5 s off.
- Instructions come from `PBTableInst[]` string array.
- High score display: Grand Champion shown in large gold text; ranks 2–10 in small white text below.
- On door-open completion: Player 1 is initialized, all other player slots are reset/disabled, the main screen fade-in animation starts, and the screen manager is seeded with `PBTBL_MAIN / MAIN_NORMAL`.
- Adding a second player mid-attract: not applicable — players are only added once doors open.

### Sample AI Prompt — PBTBL_START

```
Create the PBTBL_START attract screen for my RasPin pinball table called "Space Rangers".
Theme is space exploration. The attract screen should cycle through a "Press Start" screen,
an instructions screen, and a high score board. Pressing START opens an animated transition
(doors or a hatch) that leads into gameplay. Use the Dragons of Destiny table as a reference
for structure — same sub-states, same timeout/cycling behavior, just with space-themed
assets and text.
```

---

## 8. Mode: PBTBL_MAIN (Gameplay)

**File:** `Pinball_Table_ModeMain.cpp`

This is the primary gameplay mode. It is structured as a **base layer** (always rendered) plus a **sub-state overlay** managed by the screen manager.

### 8.1 Main Screen Sub-States

| Value | Name | Display | Duration | Player score shown? |
|-------|------|---------|----------|---------------------|
| 0 | `MAIN_NORMAL` | Scores + status | Persistent | Yes |
| 1 | `MAIN_EXTRABALL` | Extra ball video loop | 3.9 s | Yes (base still draws) |
| 2 | `MAIN_BALLSAVED` | "Ball Saved" gold text | 2 s | No (entire score area hidden) |
| 3 | `MAIN_INN_OPEN` | "Inn Open!" gold text | 2 s | No main score; other players shown |
| 4 | `MAIN_KEY_OBTAINED` | "Key Obtained!" gold text | 2 s | No main score; other players shown |

**Sub-state transition diagram:**

```
               ┌─────────────────────────────────────────┐
               │              MAIN_NORMAL                │  ◄─ priority 0 (permanent)
               └─────────────────────────────────────────┘
                       ▲ (all overlays expire/auto-remove)
                       │
        ┌──────────────┼──────────────┬────────────────┐
        │              │              │                │
   sling hit       inn complete   key complete   drain+save active
  (10k milestone)                               or drain+return
        ▼              ▼              ▼                ▼
  MAIN_EXTRABALL  MAIN_INN_OPEN  MAIN_KEY_OBTAINED  MAIN_BALLSAVED
  (3.9s, P2 HIGH) (2s, P2 HIGH)  (2s, P2 HIGH)      (2s, P2 HIGH)
```

All overlays are requested via `pbeRequestScreen()` with `PRIORITY_HIGH` and a fixed duration. When the timer expires, the screen manager automatically falls back to the priority-0 `MAIN_NORMAL` background.

### 8.2 Scoring Rules

| Event | Points | Other effects |
|-------|--------|--------------|
| Slingshot hit | 1,000 | Sound: sword cut; every 10k = extra ball + ball save |
| Pop bumper hit | 50 | +1 `goldValue` |
| Inn lane sensor (new) | 250 | Light inn LED; if all 3 lit → complete inn |
| Key target sensor (new) | 250 | Light key LED; if all 3 lit → complete key |
| Ball saved | 0 | Ball returned, extra ball cleared, save cleared |

### 8.3 Inn Lane System

```
State: m_innLaneLEDOn[3]  (tracks which of the 3 inn lane LEDs are on)

  Ball hits IDI_INN1 (not already lit)
    → m_innLaneLEDOn[0] = true
    → SendOutputMsg(IDO_INN1LED, ON)
    → +250 score
    → if all 3 lit:
         bInnOpen = true
         reset all 3 LEDs to OFF
         Request MAIN_INN_OPEN (2s, PRIORITY_HIGH)

  Left flipper pressed (IDI_LFLIP)
    → rotate array LEFT: [1]→[0], [2]→[1], [0]→[2]
    → update all 3 LEDs

  Right flipper pressed (IDI_RFLIP)
    → rotate array RIGHT: [1]→[2], [0]→[1], [2]→[0]
    → update all 3 LEDs

  Same logic applies to IDI_INN2 and IDI_INN3.
```

The rotate mechanic allows the player to line up a partially-lit set to complete the bank without needing to hit all three lanes consecutively.

### 8.4 Key Target System

```
State: m_keyTargetLEDOn[3]

  Ball hits IDI_KEY1 (not already lit)
    → m_keyTargetLEDOn[0] = true
    → SendOutputMsg(IDO_KEY1LED, ON)
    → +250 score
    → if all 3 lit:
         bKeyObtained = true
         Request MAIN_KEY_OBTAINED (2s, PRIORITY_HIGH)
         (key LEDs are NOT reset — they stay lit)

  Flipper buttons do NOT affect key target LEDs.
  Same logic for IDI_KEY2, IDI_KEY3.
```

### 8.5 Inlane / Ball-Save System

```
  IDI_LINLANE triggered (not already lit)
    → m_leftInlaneLEDOn = true
    → SendOutputMsg(IDO_LINLANELED, ON)

  IDI_RINLANE triggered (not already lit)
    → m_rightInlaneLEDOn = true
    → SendOutputMsg(IDO_RINLANELED, ON)

  Both inlanes lit?
    → ps.ballSaveEnabled = true
    → SendOutputMsg(IDO_SAVELED, ON)
    → pbeSetTimer(BALLSAVE_TIMER_ID, 5000)

  BALLSAVE_TIMER_ID fires:
    → ballSaveEnabled = false
    → clear both inlane LEDs and save LED

  Inlane state is TRANSIENT — reset every turn by pbeActivatePlayer()
```

### 8.6 Extra Ball System

```
  Slingshot hit:
    threshold = (ps.score / 10000) * 10000
    if threshold > 0 AND threshold > ps.lastExtraBallThreshold:
      ps.lastExtraBallThreshold = threshold
      Request MAIN_EXTRABALL (3.9s, PRIORITY_HIGH)
      if !extraBallEnabled:
        extraBallEnabled = true
        ballSaveEnabled  = true
        IDO_SAVELED ON

  Ball drain while ballSaveEnabled:
    ballSaveEnabled = false
    extraBallEnabled = false
    pbeActivatePlayer(m_currentPlayer)   ← resets LEDs
    start hopper (re-eject ball)
    Request MAIN_BALLSAVED (2s)

  The ball counter (currentBall) is NOT incremented on a saved drain.
```

### 8.7 Ball Drain and Player Rotation

```
IDI_BALLDRAIN triggered:

  IF ballSaveEnabled → see §8.6 (ball returned)

  ELSE:
    ps.currentBall++
    if currentBall > ballsPerGame:
      ps.enabled = false

    Stop BALLSAVE_TIMER_ID

    Find next enabled player (circular search, all 4 slots)
      ├── Found: m_tableState = PBTBL_PLAYEREND
      └── Not found (all done): m_tableState = PBTBL_GAMEEND
```

**Player rotation:** Players take turns in index order (0→1→2→3→0...). A player is removed from rotation only when all their balls are exhausted (`enabled = false`). `inGame` is never cleared mid-game so the score display continues to show them at the bottom.

### 8.8 Status Panel (Right Side)

The right one-third of the active display area is dedicated to non-score status:

```
Layout (right side, ACTIVEDISPX+700 to ACTIVEDISPX+1000):

  ┌────────────────────────────────┐
  │  [Shidea]    [Kahriel]        │  ← Ranger + Priest headshots in circles
  │  Level: X    Level: X         │     (greyed out until joined)
  │         [Caiphos]             │  ← Knight headshot
  │         Level: X              │
  │                               │
  │  [Treasure] GOLD_VALUE        │
  │  [Sword]    ATTACK_VALUE      │
  │  [Shield]   DEFENSE_VALUE     │
  │  [Dungeon]  Floor: X          │
  │             Level: X          │
  └────────────────────────────────┘
```

Character headshots display at 50% transparency with `(85,85,85)` grey tint when the character has not yet joined. Full-color `(255,255,255)` when joined.

The dungeon sprite also has a blinking eye animation: visible for 1 second, hidden for a random 1–15 second interval.

### 8.9 Score Display and Animation

**Main player score** (center area):
- Renders at scale 1.2, centered at `ACTIVEDISPX + (1024/3), ACTIVEDISPY + 350`.
- "Player X" label at `ACTIVEDISPY + 280`.
- Score animates as a count-up over `UPDATESCOREMS` (1000 ms) when points are added.
- Fade-in animates alpha from 0→255 over 1.5 s when first entering the main screen.
- **Hidden** during MAIN_BALLSAVED, MAIN_INN_OPEN, MAIN_KEY_OBTAINED.

**Secondary player scores** (bottom of screen):
- Up to 3 other active players rendered at scale 0.375 in grey.
- Positions: left, middle-third, right-third of active width.
- Scroll-up animation (50px over 1s) when a new player is added mid-game.

**NeoPixel:**
- Ball save active → dramatic snake animation (white/purple pulses).
- Normal play → gradual blue fade pulsing.

### Sample AI Prompt — PBTBL_MAIN

```
Create the PBTBL_MAIN gameplay mode for my RasPin pinball table called "Space Rangers".
Base the implementation on the Dragons of Destiny table but with a space theme:
- Pop bumpers score 50 pts and add Credits (instead of gold)
- Slingshots score 1000 pts; every 10,000-point milestone awards an extra ball
- 3 Fuel Cell lane sensors complete a bank (like inn lanes) — flippers rotate the lit LEDs
- 3 Nav Star standup targets complete a different bank (like key targets) — no flipper rotation
- Both inlanes lit activates ball save for 5 seconds
- Ball drain logic: save returns ball, otherwise rotate players or go to game end
- Right status panel shows 3 crew members (Captain, Pilot, Engineer) and
  resource counters for Credits, Shields, Speed, and Sector
- Flash gold "Fueled Up!" or "Nav Locked!" messages (2s) when each bank completes,
  suppressing the main score during the flash
```

---

## 9. Mode: PBTBL_PLAYEREND (Between-Turn Transition)

**File:** `Pinball_Table_ModePlayerEnd.cpp`

**Purpose:** Displays the upcoming player's name briefly, then ejects the next ball.

**Sub-states (`PBTBLPlayerEndState`):**

| Value | Name | Description |
|-------|------|-------------|
| 0 | `PLAYEREND_DISPLAY` | Name displayed, waiting for 2s timer |
| 1 | `PLAYEREND_EJECTING` | Hopper ejecting, waiting for ball-delivered sensor |

**State diagram:**

```
[Enter from PBTBL_MAIN on drain]
        │
        ▼
  PLAYEREND_DISPLAY
  "Player X" (gold, scale 1.2)
  "Your Turn" (grey, scale 0.5)
        │ PLAYEREND_DISPLAY_TIMER fires (2s)
        │  → switch m_currentPlayer to nextPlayer
        │  → pbeActivatePlayer() (reset LEDs)
        │  → start hopper
        ▼
  PLAYEREND_EJECTING
  "Player X" (same display)
  "Get Ready!" (grey)
        │ IDI_BALLDELIVERED sensor
        │  → disable hopper
        │  → m_tableState = PBTBL_MAIN / MAIN_NORMAL
        ▼
  [Back to PBTBL_MAIN]
```

**Key details:**
- `m_playerEndNextPlayer` is set by the drain handler in PBTBL_MAIN before transitioning here.
- `m_playerEndInitialized` is a one-shot flag; initialization happens in the first `pbeUpdateStatePlayerEnd()` call.
- The render always calls `pbeRenderPlayerScores(false)` and `pbeRenderStatus()` so all player scores and the status panel remain visible — only the main player total is hidden.

### Sample AI Prompt — PBTBL_PLAYEREND

```
Create the PBTBL_PLAYEREND between-turn transition mode for my RasPin pinball table.
It should display the next player's name in the center for 2 seconds, then eject the ball
and wait for the ball-delivered sensor before returning to PBTBL_MAIN. Show all other
players' scores and the status panel during this screen. Use the same pattern as
Dragons of Destiny.
```

---

## 10. Mode: PBTBL_RESET (Reset / Confirmation)

**File:** `Pinball_Table_ModeReset.cpp`

**Purpose:** Catches the Reset button from any state and asks the player to confirm before returning to the attract screen.

**No sub-states.** Single render state.

```
[Any state] RESET button pressed
        │
        ▼
  PBTBL_RESET
  Green full-screen background
  Black box overlay (700×200)
  "Press reset for menu"   (white)
  "Any button to cancel"   (white, smaller)
        │
        ├── RESET pressed again
        │     → pbeEngineReload() + pbeTableReload()
        │     → m_mainState = PB_STARTMENU
        │     → m_tableState = PBTBL_START
        │     → stop all music/effects
        │
        └── START / LACTIVATE / RACTIVATE / LFLIP / RFLIP pressed
              → restore m_tableState = m_StateBeforeReset
              → if PBTBL_MAIN was running, restore MAIN_NORMAL sub-state
              → re-request the saved screen as priority 0
```

**Key details:**
- `m_StateBeforeReset`, `m_ScreenBeforeResetState`, `m_ScreenBeforeResetSubState` are saved atomically when the button is first pressed.
- The reset screen renders **over** the previous screen by clearing with a distinct green color (classic BSOD-style). This makes it instantly recognizable.
- Only the 5 interactive buttons cancel the reset — raw sensors (`IDI_BALLDRAIN` etc.) are ignored.

### Sample AI Prompt — PBTBL_RESET

```
Create the PBTBL_RESET confirmation screen for my RasPin pinball table. It should
clear the screen to a vivid color, show a centered message asking the player to
press Reset again to return to the menu or any other button to cancel and resume.
Pressing Reset again should reload the engine and go to the attract screen.
Any other button should restore the previous game state. Use Dragons of Destiny as a reference.
```

---

## 11. Mode: PBTBL_GAMEEND (High Score Entry)

**File:** `Pinball_Table_ModeGameEnd.cpp`

**Purpose:** After the last player's last ball drains, identify which players qualify for the top-10 high score board and let each enter 3-character initials using the flipper/activate buttons.

**Sub-states (`PBTBLGameEndState`):**

| Value | Name | Description |
|-------|------|-------------|
| 0 | `GAMEEND_INIT` | Placeholder; transitions immediately on first render |
| 1 | `GAMEEND_ENTERINITIALS` | Active qualifier entering initials |
| 2 | `GAMEEND_COMPLETE` | "Game Over" screen, then return to attract |

**State diagram:**

```
[Enter from PBTBL_MAIN — no more players enabled]
        │
        ▼
  pbeLoadGameEnd()
  → SetAutoOutputEnable(false)
  → All LEDs off, NeoPixels off

  First render (m_gameEndInitialized == false):
  → Freeze all score animations
  → determineHighScoreQualifiers()  ← merge existing scores + player scores, sort, find top-N active players
        │
        ├── qualifiers empty → GAMEEND_COMPLETE (10s timer)
        └── qualifiers found → GAMEEND_ENTERINITIALS
                                 (set m_currentPlayer = first qualifier)
        │
        ▼
  GAMEEND_ENTERINITIALS
  Renders:
    "Player X - New High Score!" (gold)
    Score in large white text
    "Enter Initials"
    3 letter slots: active=red blinking, inactive=gold
  Controls:
    LFLIP / RFLIP: cycle active letter down/up through " A-Z0-9"
    LACTIVATE: move cursor left
    RACTIVATE: advance cursor, or on last position → confirm entry
        │ On confirm of last qualifier:
        │   merge new scores into save data, sort, trim to NUM_HIGHSCORES
        │   pbeSaveFile()
        │   GAMEEND_COMPLETE (20s timer)
        ▼
  GAMEEND_COMPLETE
  Renders:
    "Game Over" (gold, scale 1.0)
    All player scores — high score qualifiers get red shimmer shadow
  Controls:
    20s timer OR any button → reset all player states,
    m_tableState = PBTBL_START, m_RestartTable = true.
```

**`determineHighScoreQualifiers()` algorithm:**
1. Collect all active-player scores (score > 0) + all existing 10 high scores into one list.
2. Sort descending by score.
3. Walk the top 10: every active-player entry in that range becomes a qualifier.
4. Sort qualifiers by player index (P1 before P2, etc.).

This prevents a player from entering initials only to be bumped out by a higher-scoring teammate.

### Sample AI Prompt — PBTBL_GAMEEND

```
Create the PBTBL_GAMEEND end-of-game mode for my RasPin pinball table. When all players
run out of balls, check which players scored in the top 10 high scores. For each qualifier
(in player order), let them enter 3-character initials using the flippers to cycle letters
and the activate buttons to move the cursor. After all initials are entered, save the high
score file and show a "Game Over" screen with all scores for 20 seconds, then return to
the attract screen. Use Dragons of Destiny as a reference.
```

---

## 12. Adding a New Table Mode — Checklist

Use this as a step-by-step guide when creating a new game mode (e.g., a bonus round, a multiball mode, or a dungeon-attack sequence):

```
[ ] 1. Add a new entry to PBTableState in src/system/Pinball_Table.h

[ ] 2. Create src/user/tablemodes/Pinball_Table_ModeX.h
        — Define the sub-state enum class (PBTBLXState)
        — Include any mode-specific constants or structs

[ ] 3. Create src/user/tablemodes/Pinball_Table_ModeX.cpp
        — bool pbeLoadX()          — guard-flagged resource loading
        — bool pbeRenderX()        — per-frame rendering (calls Load first)
        — void pbeUpdateStateX()   — input message handler

[ ] 4. Include the new header in src/system/Pinball_Table.h
        #include "tablemodes/Pinball_Table_ModeX.h"

[ ] 5. Add function declarations in src/system/Pinball_Engine.h
        bool pbeLoadX();
        bool pbeRenderX(unsigned long currentTick, unsigned long lastTick);
        void pbeUpdateStateX(stInputMessage inputMessage);
        bool m_xLoaded = false;           // load guard member
        int  m_xSubScreenState = 0;       // if sub-states are needed

[ ] 6. Add dispatch cases in Pinball_Table.cpp:
        In pbeRenderGameScreen()  → case PBTableState::PBTBL_X: pbeRenderX(...)
        In pbeUpdateGameState()   → case PBTableState::PBTBL_X: pbeUpdateStateX(...)
        In pbeRenderGameScreen()  → case PBTBL_X: pbeRequestScreen(PBTBL_X, ...)

[ ] 7. Add the new .cpp to CMakeLists.txt and all tasks.json build args

[ ] 8. Trigger the mode:
        From another mode's update function: m_tableState = PBTableState::PBTBL_X;
        Call pbeClearScreenRequests() and pbeRequestScreen(PBTBL_X, 0, PRIORITY_LOW, 0, true)

[ ] 9. Return from the mode:
        Set m_tableState back to the appropriate state (usually PBTBL_MAIN)
        Call pbeActivatePlayer() if hardware state needs resetting
        Call pbeClearScreenRequests() and request the return screen
```

**Remember:** The load guard (`if (m_xLoaded) return true;`) is critical — render functions are called every frame, so resource loading must be idempotent. Reset the flag in `pbeTableReload()` so a full game reset re-loads everything correctly.
