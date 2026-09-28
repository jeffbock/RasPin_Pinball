# Table State Maps

The state-map registries describe intended table and mode transitions in a format that renders deterministically. JSON registries are the source of truth; the generator writes Graphviz files under `documentation/state_map/generated_maps/`.

## Files

- `transitions.json`: top-level `PBTableState` transitions and mode entry/exit.
- `intower_transitions.json`: focused `InTowerFlowState` transitions.
- `../../scripts/generate_state_map.py`: validates registries and generates diagrams.
- `generated_maps/*.dot`: generated Graphviz source. Do not edit these files manually.
- `generated_maps/*.svg`: generated only when Graphviz is installed.

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

The generator always writes DOT files. When Graphviz's `dot` executable is on `PATH`, it also writes SVG files. To require SVG rendering and fail when Graphviz is unavailable:

```powershell
python scripts/generate_state_map.py --render-svg
```

To create PNG files after Graphviz is installed:

```powershell
dot -Tpng documentation/state_map/generated_maps/transitions.dot -o documentation/state_map/generated_maps/transitions.png
dot -Tpng documentation/state_map/generated_maps/intower_transitions.dot -o documentation/state_map/generated_maps/intower_transitions.png
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

1. When adding or changing a `PBTableState`, `PBTableMode`, `InTowerFlowState`, or major gameplay-flow transition, update the appropriate JSON registry in this directory in the same change set.
2. Add a node before referencing it in a transition.
3. Add the source file and the user-visible trigger or guard for every transition.
4. Run `python scripts/generate_state_map.py --check`.
5. Run `python scripts/generate_state_map.py` and review the generated DOT or rendered SVG/PNG output.

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
Update the affected table-state map registry in documentation/state_map/ as part of this change.
Find the actual state/mode assignment or pbeEnterMode/pbeExitMode call in the code.
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
6. The generator validation passes before accepting the change.
