#!/usr/bin/env python3
"""
generate_io_header.py - Generate io_defs_generated.h from io_definitions.json

Reads the IO definitions JSON file and produces a C/C++ header file containing
#define constants for all input (IDI_*) and output (IDO_*) identifiers, plus
NUM_INPUTS and NUM_OUTPUTS counts.

Performs full schema validation on the JSON file, checking for:
  - Required fields and correct data types
  - Valid enum values for msg, board, state, autoState
  - Duplicate IDs and indices
  - ID naming conventions (IDO_* for outputs, IDI_* for inputs)
  - autoOut references pointing to valid output IDs

Usage:
    python scripts/generate_io_header.py [json_path] [header_path]

Defaults:
    json_path   = src/user/io_definitions.json
    header_path = src/system/io_defs_generated.h
"""

import json
import sys
import os

# --- Valid enum values (must match the C++ enum parsers in Pinball_IO.cpp) ---
VALID_OUTPUT_MSG = {"LED", "LEDCFG_GROUPDIM", "LEDCFG_GROUPBLINK", "LEDSET_BRIGHTNESS",
                    "LED_SEQUENCE", "GENERIC_IO", "NEOPIXEL", "NEOPIXEL_SEQUENCE"}
VALID_INPUT_MSG = {"EMPTY", "SENSOR", "TARGET", "SLING", "POPBUMPER", "BUTTON", "TIMER"}
VALID_BOARD = {"RASPI", "IO", "LED", "NEOPIXEL"}
VALID_STATE = {"ON", "OFF", "BLINK", "BRIGHTNESS"}

# --- Schema definitions ---
OUTPUT_SCHEMA = {
    "id":       str,
    "name":     str,
    "msg":      str,
    "pin":      int,
    "board":    str,
    "boardIdx": int,
    "state":    str,
    "onMs":     int,
    "offMs":    int,
    "neo":      int,
}

INPUT_SCHEMA = {
    "id":        str,
    "name":      str,
    "key":       str,
    "msg":       str,
    "pin":       int,
    "board":     str,
    "boardIdx":  int,
    "state":     str,
    "tick":      int,
    "debMs":     int,
    "auto":      bool,
    "autoOut":   str,
    "autoState": str,
    "autoPulse": bool,
}


def validate_entry(entry, schema, section, index, errors):
    """Validate a single entry against its schema. Appends error strings to 'errors'."""
    entry_id = entry.get("id", f"<entry #{index}>")

    # Check for missing required fields
    for field, expected_type in schema.items():
        if field not in entry:
            errors.append(f"{section} '{entry_id}': missing required field '{field}'")
            continue
        # Check data type
        value = entry[field]
        if not isinstance(value, expected_type):
            errors.append(f"{section} '{entry_id}': field '{field}' must be {expected_type.__name__}, "
                          f"got {type(value).__name__} ({value!r})")

    # Warn about unknown fields (skip _docs fields)
    for field in entry:
        if field not in schema and not field.startswith("_"):
            errors.append(f"{section} '{entry_id}': unknown field '{field}'")


def validate_outputs(outputs, errors):
    """Validate all output entries."""
    seen_ids = set()

    for i, o in enumerate(outputs):
        validate_entry(o, OUTPUT_SCHEMA, "output", i, errors)

        oid = o.get("id", "")

        # ID naming convention
        if isinstance(oid, str) and oid and not oid.startswith("IDO_"):
            errors.append(f"output '{oid}': id must start with 'IDO_'")

        # Duplicate id check
        if oid in seen_ids:
            errors.append(f"output '{oid}': duplicate id")
        seen_ids.add(oid)

        # Enum validation
        msg = o.get("msg", "")
        if isinstance(msg, str) and msg and msg not in VALID_OUTPUT_MSG:
            errors.append(f"output '{oid}': invalid msg '{msg}', "
                          f"must be one of: {', '.join(sorted(VALID_OUTPUT_MSG))}")
        board = o.get("board", "")
        if isinstance(board, str) and board and board not in VALID_BOARD:
            errors.append(f"output '{oid}': invalid board '{board}', "
                          f"must be one of: {', '.join(sorted(VALID_BOARD))}")
        state = o.get("state", "")
        if isinstance(state, str) and state and state not in VALID_STATE:
            errors.append(f"output '{oid}': invalid state '{state}', "
                          f"must be one of: {', '.join(sorted(VALID_STATE))}")

    return seen_ids


def validate_inputs(inputs, output_ids, errors):
    """Validate all input entries."""
    seen_ids = set()

    for i, inp in enumerate(inputs):
        validate_entry(inp, INPUT_SCHEMA, "input", i, errors)

        iid = inp.get("id", "")

        # ID naming convention
        if isinstance(iid, str) and iid and not iid.startswith("IDI_"):
            errors.append(f"input '{iid}': id must start with 'IDI_'")

        # Duplicate id check (also against output IDs)
        if iid in seen_ids or iid in output_ids:
            errors.append(f"input '{iid}': duplicate id")
        seen_ids.add(iid)

        # Enum validation
        msg = inp.get("msg", "")
        if isinstance(msg, str) and msg and msg not in VALID_INPUT_MSG:
            errors.append(f"input '{iid}': invalid msg '{msg}', "
                          f"must be one of: {', '.join(sorted(VALID_INPUT_MSG))}")
        board = inp.get("board", "")
        if isinstance(board, str) and board and board not in VALID_BOARD:
            errors.append(f"input '{iid}': invalid board '{board}', "
                          f"must be one of: {', '.join(sorted(VALID_BOARD))}")
        state = inp.get("state", "")
        if isinstance(state, str) and state and state not in VALID_STATE:
            errors.append(f"input '{iid}': invalid state '{state}', "
                          f"must be one of: {', '.join(sorted(VALID_STATE))}")
        auto_state = inp.get("autoState", "")
        if isinstance(auto_state, str) and auto_state and auto_state not in VALID_STATE:
            errors.append(f"input '{iid}': invalid autoState '{auto_state}', "
                          f"must be one of: {', '.join(sorted(VALID_STATE))}")

        # autoOut reference validation
        auto = inp.get("auto", False)
        auto_out = inp.get("autoOut", "")
        if auto and isinstance(auto_out, str):
            if not auto_out:
                errors.append(f"input '{iid}': auto is true but autoOut is empty")
            elif auto_out not in output_ids:
                errors.append(f"input '{iid}': autoOut '{auto_out}' does not match any output id")


def main():
    # Determine paths (default to project-root-relative)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)

    json_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(project_root, "src", "user", "io_definitions.json")
    header_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(project_root, "src", "system", "io_defs_generated.h")

    # Read JSON
    try:
        with open(json_path, "r") as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        print(f"ERROR: Invalid JSON in {json_path}: {e}", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print(f"ERROR: File not found: {json_path}", file=sys.stderr)
        sys.exit(1)

    # Check top-level structure
    if not isinstance(data, dict):
        print(f"ERROR: Top-level JSON must be an object, got {type(data).__name__}", file=sys.stderr)
        sys.exit(1)

    if "outputs" not in data:
        print("ERROR: Missing required top-level key 'outputs'", file=sys.stderr)
        sys.exit(1)
    if "inputs" not in data:
        print("ERROR: Missing required top-level key 'inputs'", file=sys.stderr)
        sys.exit(1)

    outputs = data["outputs"]
    inputs = data["inputs"]

    if not isinstance(outputs, list):
        print(f"ERROR: 'outputs' must be an array, got {type(outputs).__name__}", file=sys.stderr)
        sys.exit(1)
    if not isinstance(inputs, list):
        print(f"ERROR: 'inputs' must be an array, got {type(inputs).__name__}", file=sys.stderr)
        sys.exit(1)

    # Validate all entries
    errors = []
    output_ids = validate_outputs(outputs, errors)
    validate_inputs(inputs, output_ids, errors)

    if errors:
        print(f"ERROR: {len(errors)} validation error(s) in {json_path}:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        sys.exit(1)

    # Compute NUM_OUTPUTS and NUM_INPUTS from JSON array length (order = index)
    num_outputs = len(outputs)
    num_inputs = len(inputs)

    # Find the longest id name for alignment
    all_ids = [o["id"] for o in outputs] + [i["id"] for i in inputs] + ["NUM_OUTPUTS", "NUM_INPUTS"]
    max_id_len = max(len(name) for name in all_ids)

    # Generate header content
    lines = []
    lines.append("// io_defs_generated.h - AUTO-GENERATED from src/user/io_definitions.json")
    lines.append("// Do not edit this file manually. Edit src/user/io_definitions.json instead,")
    lines.append("// then run: python scripts/generate_io_header.py")
    lines.append("")
    lines.append("#ifndef IO_DEFS_GENERATED_H")
    lines.append("#define IO_DEFS_GENERATED_H")
    lines.append("")

    # Output defines
    lines.append("// --- Output definitions (IDO_*) ---")
    for idx, o in enumerate(outputs):
        padding = " " * (max_id_len - len(o["id"]))
        lines.append(f"#define {o['id']}{padding}  {idx}")
    padding = " " * (max_id_len - len("NUM_OUTPUTS"))
    lines.append(f"#define NUM_OUTPUTS{padding}  {num_outputs}")
    lines.append("")

    # Input defines
    lines.append("// --- Input definitions (IDI_*) ---")
    for idx, i in enumerate(inputs):
        padding = " " * (max_id_len - len(i["id"]))
        lines.append(f"#define {i['id']}{padding}  {idx}")
    padding = " " * (max_id_len - len("NUM_INPUTS"))
    lines.append(f"#define NUM_INPUTS{padding}  {num_inputs}")
    lines.append("")
    lines.append("#endif // IO_DEFS_GENERATED_H")
    lines.append("")

    new_content = "\n".join(lines)

    # Only write if content changed (avoid triggering unnecessary rebuilds)
    if os.path.exists(header_path):
        with open(header_path, "r") as f:
            existing = f.read()
        if existing == new_content:
            print(f"io_defs_generated.h is up to date (no changes)")
            return

    os.makedirs(os.path.dirname(header_path), exist_ok=True)
    with open(header_path, "w") as f:
        f.write(new_content)

    print(f"Generated {header_path} with {len(outputs)} outputs and {len(inputs)} inputs")


if __name__ == "__main__":
    main()
