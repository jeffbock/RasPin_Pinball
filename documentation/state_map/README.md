# Table State Maps

The C++ table-mode code is the source of truth for gameplay behavior. The JSON registries are an AI-maintained specification derived from that code, and the generator visualizes the specification as Graphviz diagrams under `documentation/state_map/generated_maps/`.

## Source of Truth

The dependency direction is deliberately one-way:

```text
C++ state-machine code -> AI-generated JSON transition specification -> state-map generator -> DOT/SVG/PNG diagrams
```

The JSON files do **not** drive runtime behavior, generate C++ code, or replace code review. Change the C++ state machine first. Then ask AI to inspect the actual assignments and mode calls, update the matching JSON specification, and run the visualizer. If the diagram and code disagree, fix the JSON specification to match the code unless the code itself is wrong.

## Files

- `transitions.json`: top-level `PBTableState` transitions and mode entry/exit.
- `intower_transitions.json`: focused `InTowerFlowState` transitions.
- `../../scripts/generate_state_map.py`: validates registries and generates diagrams.
- `generated_maps/*_YYYY-MM-DD.dot`: dated generated Graphviz source. Do not edit these files manually.
- `generated_maps/*_YYYY-MM-DD.svg`: dated rendered diagrams, generated only when Graphviz is installed.

## Generate Maps

From the repository root, validate the registries without writing output:

```powershell
python scripts/generate_state_map.py --check
```

Generate the DOT diagrams:

```powershell
python scripts/generate_state_map.py
```

The same command is available in VS Code as the `Generate State Map` task.

The generator writes files with an ISO generation-date suffix, for example `transitions_2026-09-28.dot`. When Graphviz's `dot` executable is on `PATH`, it also writes an SVG with the same suffix. To require SVG rendering and fail when Graphviz is unavailable:

```powershell
python scripts/generate_state_map.py --render-svg
```

To create PNG files after Graphviz is installed:

```powershell
dot -Tpng documentation/state_map/generated_maps/transitions_YYYY-MM-DD.dot -o documentation/state_map/generated_maps/transitions_YYYY-MM-DD.png
dot -Tpng documentation/state_map/generated_maps/intower_transitions_YYYY-MM-DD.dot -o documentation/state_map/generated_maps/intower_transitions_YYYY-MM-DD.png
```

## Install Graphviz on Windows

Graphviz is required to render SVG or PNG diagrams; it is not required to validate registries or generate DOT files.

Open an elevated PowerShell window and install it with Winget:

```powershell
winget install --id Graphviz.Graphviz --exact --accept-package-agreements --accept-source-agreements
```

Restart VS Code or open a new terminal after installation, then verify it:

```powershell
dot -V
```

If Winget opens an installer window, complete it there. Do not use `--silent` when the installer requires confirmation or elevation.

## Workflow

1. Add or change the C++ state-machine behavior first.
2. Ask AI to inspect the controlling `m_tableState` assignment, `pbeEnterMode(...)`, `pbeExitMode(...)`, or state-flow branch and update the matching JSON registry in the same change set.
3. Have AI add any required node before referencing it in a transition, along with the source file and user-visible trigger or guard.
4. Run `python scripts/generate_state_map.py --check`.
5. Run `python scripts/generate_state_map.py` and review the generated DOT or rendered SVG/PNG output against the code.

Keep screen requests and visual-only substates out of these maps unless they change gameplay flow. Create another focused registry when a mode cannot be understood at the global-map level.

## Transition Kinds

| Kind | Meaning | Diagram style |
| --- | --- | --- |
| `table-state` | Direct game/table state transition | solid blue |
| `mode-entry` / `mode-exit` | `pbeEnterMode` or `pbeExitMode` relationship | dashed purple |
| `success` | Positive gameplay outcome | solid green |
| `failure` | Negative gameplay outcome | solid red |
| `conditional` | Branch depends on a runtime guard | dotted amber |
| `loop` | Repeats within the same flow state | solid gray |
| `external` | Leaves the scope of the focused map | dashed gray |

## Using AI to Update Registries

Give the AI both the code change and this explicit requirement:

```text
The C++ state machine is authoritative. Inspect the affected code first, then update the derived table-state map specification in documentation/state_map/ as part of this change.
Find the actual state/mode assignment or pbeEnterMode/pbeExitMode call that controls the behavior.
Use the existing node IDs and transition kinds; add a node first if needed.
For every transition, record the user-visible trigger or guard and its source file.
Do not edit files in `generated_maps` directly.
Run python scripts/generate_state_map.py --check, regenerate the maps, and report which registry entries changed.
```

Review an AI-generated registry update against the following checklist:

1. Every added `m_tableState =`, `pbeEnterMode(...)`, or `pbeExitMode(...)` that changes gameplay flow is represented.
2. The `from` and `to` node IDs are declared in the registry.
3. `trigger` states the input, timer, event, or gameplay condition rather than merely repeating the destination name.
4. `source` names the file that contains the controlling transition.
5. The selected `kind` matches the table above.
6. The JSON describes the implemented code rather than proposing a different runtime behavior.
7. The generator validation passes before accepting the change.
