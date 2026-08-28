# InTower Tower Progression Revamp Plan

## Goal

Revamp InTower as a self-contained persisted tower-progression state machine. Reuse the existing dungeon, dice, enemy, and avatar presentation, including its internal sub-animation methodology, while making the gameplay flow explicit and persisting only intended tower progress between entries.

Keep the screen manager responsible only for the outer InTower mode. Do not add individual InTower gameplay or animation states to screen-manager requests. A new DragonMultiball placeholder will receive the completed-top-tower handoff.

## 1. Define Persistent Data and Internal States

- In `src/user/tablemodes/Pinball_Table_ModeInTower.h`, extend persisted tower grid and door data with:
  - A one-time-generated marker.
  - Door role: ordinary, staircase, or dragon.
  - Original and current monster counts.
  - Challenge assignment on its staircase door: required champion type and challenge level.
  - Saved avatar location and state.
- In `src/system/Pinball_Table.h`, add player-owned InTower values:
  - Current attempt HP.
  - Missing-champion failure/display flag.
  - `towerNeedsReset`.
  - Minimal resume data not owned by the grid.
- Initialize `towerNeedsReset` to `true` for a new player.
- Preserve `attackValue` and `defenseValue` as the normal persistent player economy. Do not create temporary tower copies.
- Follow the existing InTower internal phase and sub-animation methodology. Add or clarify internal gameplay states such as:
  - `TowerInit`
  - `TowerClimb`
  - `RoomFight`
  - `FloorChallenge`
  - `ExitTower`
  - Completion or handoff, if required for video timing
- Retain granular internal animation states for dungeon zoom/shrink/grow, avatar door entry, D20 ready/rolling/stopped, combat resolution, challenge video, success/failure hold, and exit hold.
- Keep screen-manager and `pbeRequestScreen` interaction at the outer `PBTBL_INTOWER` boundary. Do not request individual internal InTower states through the screen manager.

## 2. Generate and Resume Towers

- Refactor `pbeInitDungeonGrid()` so `towerNeedsReset` is the authoritative initialization signal.
- When `towerNeedsReset` is set:
  - Use the player's current tower level and floor to generate that tower.
  - Clear `towerNeedsReset` immediately after successful initialization.
- When `towerNeedsReset` is clear, resume the existing grid without regenerating it.
- When a later DragonMultiball victory advances tower level and floor, set `towerNeedsReset` so the next InTower entry generates the correct new tower.
- Preserve the existing dungeon layout and door animation basis. Tag exactly one staircase door for each ascent and one dragon door on the final floor.
- Generate and persist challenge character and level data once, using a shuffled party-member sequence that guarantees knight, priest, and ranger appear before a duplicate.
- Challenge schedules:
  - Tower 1: 3 floors; two between-floor challenges at Level 1 and Level 1.
  - Tower 2: 4 floors; three between-floor challenges at Level 1, Level 1, and Level 2.
  - Tower 3: 5 floors; four between-floor challenges at Level 1, Level 2, Level 2, and Level 3.
- On successful ascent, update player tower level and floor data immediately. A challenge belongs to the ascent from floor $n$ to floor $n+1$; keep `dungeonFloor == n` until the challenge succeeds.
- On each InTower entry, reset attempt HP to 20 while preserving door state, partially depleted monster counts, challenge assignments, floor, and avatar resume location.
- On defeat, close only the losing door and retain its depleted monster count.
- Persist only the player-facing floor and selected door for resume; do not persist animation timing or D20/video progress.
- If a player returns after reaching a FloorChallenge, resume in stable `TowerClimb` at that closed staircase door and replay the challenge intro and roll from the beginning.

## 3. TowerInit and TowerClimb

- `TowerInit` has no input. It:
  - Selects the player's current tower.
  - Generates only if `towerNeedsReset` is set, then clears the flag.
  - Resets HP to 20.
  - Restores the saved avatar location.
  - Transitions to `TowerClimb`.
- `TowerClimb` renders the full dungeon with the existing full-size grid and avatar path.
- On a newly reached floor, start at the far-right door. Otherwise restore the saved avatar location.
- Left and right flippers navigate legal closed doors on the active floor.
- Either activation button opens the selected closed door and starts the existing avatar-door animation.
- Already-open doors cannot be selected.
- After avatar animation completes, transition to the RoomFight shrink sequence.
- Retain the status area's two alternating values throughout InTower:
  - First status: always `Climb the Tower!`.
  - Second status: current-state guidance, for example `Flipper to select door, Activate to open!`, roll/stop instructions, challenge targets, resolution feedback, or exit result.

## 4. RoomFight

- Reuse current shrink-to-right, D20, enemy spawning, roll/stop, face/snap, slash, enemy fade, and grow-back presentation in `pbeRenderInTower()` and `pbeUpdateStateInTower()`.
- Either activation button advances D20 state: ready, rolling, then stopped. Do not consume flipper input during a fight.
- Display current attempt HP above the small dungeon.
- On roll resolution:
  - Remove enemies equal to the D20 result.
  - Automatically consume up to the remaining enemy count from persistent `attackValue`, with established sword/flame reduction feedback.
  - Resolve one damage for each surviving enemy.
  - Automatically consume `defenseValue` down to zero with established shield feedback.
  - Reduce attempt HP by any uncovered damage.
- Use short timed resolution holds and visual emphasis so enemy, attack, defense, and HP reductions are understandable.
- On HP less than or equal to zero:
  - Persist avatar and door location.
  - Close the losing door.
  - Retain remaining monsters.
  - Set normal defeat cause.
  - Enter `ExitTower`.
- When all enemies are defeated:
  - Ordinary door: return through grow animation to `TowerClimb`.
  - Staircase door: enter `FloorChallenge`.
  - Dragon door: play `dragonmultiball.mp4`, then hand off to DragonMultiball.

## 5. FloorChallenge

- Add a challenge-intro substate that plays the assigned `knight.mp4`, `priest.mp4`, or ranger video through the existing PBVideo/PBVideoPlayer lifecycle.
- Create placeholder InTower videos by copying `extraball.mp4` and renaming the copies to the final knight, priest, ranger, and dragon filenames. These assets will be replaced later without code changes.
- In full tower presentation:
  - Render the robed avatar first, positioned between floors as if entering the next tower level.
  - Render the closed relevant tower-door sprite in front of the avatar.
- After a short explicit input guard, either activation button skips the video; natural completion takes the same continuation.
- Use a 750 ms input guard before either activation button can skip a challenge or dragon video.
- After video, check required party member availability:
  - If unavailable, set the persisted missing-champion failure/display flag and transition to `ExitTower` without rolling.
  - For a failed roll, set the normal defeat result before entering `ExitTower`.
- Otherwise use the RoomFight D20 ready/rolling/stopped controls.
- Replace normal RoomFight enemy sprite rendering with the required champion presentation. Render challenge level, character level, and calculated pass target in large text.
- Pass target:
  - Challenge level above champion level: 20.
  - Equal levels: 10.
  - Champion level above challenge level: $10 - 4(\text{champion level} - \text{challenge level})$.
  - Clamp target to a minimum of 1.
- On success:
  - Display `Success!` briefly.
  - Persist ascent and avatar state.
  - Increment floor.
  - Return to `TowerClimb` at the next floor's far-right door.
- On failure, enter `ExitTower` with normal defeat cause.

## 6. ExitTower and DragonMultiball Placeholder

- In the main status area, render either:
  - `Defeated!` and `Try Again`, or
  - `Missing Champion` for the special missing-party failure.
- Render the avatar at 200% scale underneath the result text.
- Clear only the missing-champion display flag after use; retain intended tower progress.
- After the exit hold, return cleanly to normal main gameplay using existing mode lifecycle behavior.
- Add `PBTBL_DRAGONMULTIBALL`, or the project's equivalent, with dispatch, load, render, and update hooks plus a minimal placeholder screen/mode.
- Play `dragonmultiball.mp4` with the same input guard and activation-button skip convention as challenge videos, then hand off to DragonMultiball.
- The placeholder initially displays `Dragon Multiball`.
- Left Activate selects failure: display `Failed!` beneath the title, wait a short result hold, close the dragon door, and return to Main without changing tower level, floor, or `towerNeedsReset`.
- Right Activate selects victory: display `Dragon is defeated!` beneath the title, wait a short result hold, return to Main, reset tower floor to 1, and set `towerNeedsReset`.
- On victory below Tower Level 3, increment tower level. On Tower Level 3 victory, retain Level 3, reset its floor to 1, and set `towerNeedsReset`; tower level never exceeds 3.

## 7. D20 Calibration and Presentation

- Do not update D20 calibration logic or data as part of this revamp.
- Use existing calibration data for normal D20 presentation and keep calibration mode disabled in normal builds.
- Preserve the existing `#ifdef D20_CALIBRATION` entry, input, and render path when that define is enabled.
- Order new InTower input routing so existing calibration controls remain reachable when enabled, without changing calibration behavior.
- Ensure tower-section-open flags are rendered and restored so staircase progress remains clear after resume.
- Update relevant mode/state documentation with the new persistence contract.

## Verification

1. Run the VS Code `Windows: Full Pinball Build (Debug)` task after compile-affecting phases. Run the Raspberry Pi equivalent before hardware deployment.
2. Add or extend deterministic debug or simulator coverage for:
   - `towerNeedsReset` generating exactly once and clearing after success.
   - Clear reset flag resuming state.
   - Each party member appearing before duplicates.
   - Challenge schedules: 1/1, 1/1/2, and 1/2/2/3.
   - Exactly one dragon endpoint per tower.
   - Correct floor persistence.
3. Manually exercise Tower 1: ordinary room completion, a partially completed failed-room retry, challenge success/failure, and exit/re-entry with 20 HP and retained grid.
4. Exercise resource cases: no attack/defense, partial and full defense mitigation, attack exceeding enemy count, and HP reaching exactly zero.
5. Exercise missing knight, priest, and ranger separately. Verify each video plays and `Missing Champion` appears once; verify normal defeat copy for a lost roll or fight.
6. Complete all floors, open the dragon door, and verify `dragonmultiball.mp4` naturally completes or skips after its 750 ms input guard before mode handoff. Verify left Activate reports failure, closes the dragon door, and preserves tower progress; verify right Activate reports victory, resets floor, sets `towerNeedsReset`, and caps tower level at 3.
7. On Raspberry Pi hardware, verify D20 face presentation, activation buttons, flippers, video playback, status legibility, and timing holds.
8. In a separate build with `D20_CALIBRATION` enabled, enter the existing calibration path and verify its current controls remain reachable. Do not recalibrate or otherwise alter calibration data.

## Final Decisions

- HP resets to 20 on each tower entry; generated layout, completed doors, floor, challenge assignments, avatar resume state, and partial monster depletion persist.
- Defeat closes only the losing door. Enemies eliminated before defeat remain eliminated.
- Attack and defense remain spent after tower failure.
- Challenges occur between floors; floor increments only after success.
- Videos can be skipped with either activation button after a short input guard.
- The video input guard is 750 ms.
- Challenge targets clamp to 1.
- `towerNeedsReset` is set after a DragonMultiball victory. TowerInit clears it after successful generation.
- DragonMultiball is a minimal placeholder: left Activate fails and reopens the dragon-door attempt; right Activate wins, advances up to Level 3, resets floor to 1, and schedules a tower reset.
