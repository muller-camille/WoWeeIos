#!/usr/bin/env python3
"""Translate the client's SPIR-V shaders to Metal Shading Language.

The GLSL under assets/shaders stays the source every shader is written in, and
its compiled .spv stays tracked beside it, so a shader change that comes from
upstream still merges. This turns each .spv into the MSL the Metal renderer
compiles into default.metallib (see CMakeLists.txt), and writes a manifest of
where each Vulkan binding ended up - which the renderer needs, because Metal
has no descriptor sets:

    assets/shaders/metal/<name>.metal    one per shader
    assets/shaders/metal/manifest.json   per function: stage, and for every
                                         (set, binding) the Metal buffer,
                                         texture and sampler index it became

    tools/metal/convert_shaders.py            # regenerate
    tools/metal/convert_shaders.py --check    # fail if anything is stale

Needs spirv-cross on PATH (apt install spirv-cross, brew install spirv-cross).
The output is tracked, as the .spv are, so building the app needs only Xcode.

Choices, so they are not re-argued in every shader:

- Discrete bindings, not argument buffers. SPIRV-Cross numbers argument
  buffer members in SPIR-V id order rather than by binding, so a set shared by
  a vertex and a fragment stage would not be laid out the same in both. With
  discrete bindings each stage is bound on its own and the manifest says where.
- One entry point name per shader, <file>_<stage>, because every SPIR-V module
  calls its entry point main and they all go into one library.
- Vertex shaders flip gl_Position.y. Vulkan's clip space has +y down and
  Metal's has +y up, with the framebuffer's origin top left in both; the flip
  makes the same projection draw the same picture. It also reverses winding,
  so a pipeline that culls has to swap its front face.
"""
import argparse
import json
import pathlib
import re
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
SHADERS = ROOT / "assets" / "shaders"
OUT = SHADERS / "metal"

MSL_VERSION = "30000"   # Metal 3.0: iOS 16, and Apple GPUs from the A13

# Shaders the Metal renderer does not port. Each is a feature iOS leaves out,
# said here so a missing function in the library is a decision, not an accident.
EXCLUDED = {
    "rt_": "ray traced lighting is not ported",
    "editor_water": "belongs to the desktop world editor",
    "recording_dot": "screen recording needs FFmpeg, which iOS does not have",
    "fsr2_": "the temporal upscaler becomes MetalFX",
}

STAGES = {"vert": "vertex", "frag": "fragment", "comp": "kernel"}

PARAM = re.compile(r"(\w+)\s*\[\[(buffer|texture|sampler)\((\d+)\)\]\]")


def excluded_reason(stem):
    for prefix, reason in EXCLUDED.items():
        if stem.startswith(prefix):
            return reason
    return None


def strip_names(spv_bytes):
    """The module without its OpName and OpMemberName instructions.

    Most of the tracked .spv are built with -O, which strips them, and a few
    are not. SPIRV-Cross names what it emits after them when they are there
    and after SPIR-V ids when they are not, and its reflection follows a
    different rule in each case - so the binding table below could only be
    read back reliably from one kind. This makes every module that kind.
    """
    words = struct.unpack(f"<{len(spv_bytes) // 4}I", spv_bytes)
    if words[0] != 0x07230203:
        raise RuntimeError("not a little-endian SPIR-V module")
    kept = list(words[:5])
    i = 5
    while i < len(words):
        count, opcode = words[i] >> 16, words[i] & 0xFFFF
        if count == 0:
            raise RuntimeError("malformed SPIR-V instruction")
        if opcode not in (5, 6):   # OpName, OpMemberName
            kept.extend(words[i:i + count])
        i += count
    return struct.pack(f"<{len(kept)}I", *kept)


def run(args):
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"{' '.join(args)}\n{result.stderr.strip()}")
    return result.stdout


def entry_signature(msl, function):
    """The parameter list of the entry point, from its declaration."""
    start = msl.find(f" {function}(")
    if start < 0:
        raise RuntimeError(f"entry point {function} not found in its own output")
    depth = 0
    for i in range(start, len(msl)):
        if msl[i] == "(":
            depth += 1
        elif msl[i] == ")":
            depth -= 1
            if depth == 0:
                return msl[start:i]
    raise RuntimeError(f"unterminated parameter list for {function}")


def convert(spv):
    # terrain.frag.spv -> stem terrain, stage frag
    parts = spv.name.split(".")
    if len(parts) != 3 or parts[1] not in STAGES:
        raise RuntimeError(f"{spv.name}: expected <name>.<vert|frag|comp>.spv")
    stem, stage = parts[0], parts[1]
    function = f"{stem}_{stage}"

    with tempfile.NamedTemporaryFile(suffix=".spv", delete=False) as tmp:
        tmp.write(strip_names(spv.read_bytes()))
        stripped = tmp.name
    try:
        return translate(spv, stripped, stem, stage, function)
    finally:
        pathlib.Path(stripped).unlink()


def translate(spv, stripped, stem, stage, function):
    args = ["spirv-cross", stripped, "--msl", "--msl-ios",
            "--msl-version", MSL_VERSION,
            "--rename-entry-point", "main", function, stage]
    if stage == "vert":
        args.append("--flip-vert-y")
    msl = run(args)
    reflection = json.loads(run(["spirv-cross", stripped, "--reflect"]))

    params = {name: (kind, int(index))
              for name, kind, index in PARAM.findall(entry_signature(msl, function))}

    def index_of(name, kind):
        found = params.get(name)
        if not found or found[0] != kind:
            raise RuntimeError(f"{function}: no [[{kind}]] parameter for {name}")
        return found[1]

    bindings = []
    # Uniform and storage buffers are reflected as <type>_<variable>; the MSL
    # parameter carries the variable's name.
    for key, kind in (("ubos", "uniform_buffer"), ("ssbos", "storage_buffer")):
        for res in reflection.get(key, []):
            variable = "_" + res["name"].rsplit("_", 1)[-1]
            bindings.append({"set": res["set"], "binding": res["binding"], "type": kind,
                             "buffer": index_of(variable, "buffer")})
    for res in reflection.get("textures", []):          # combined image samplers
        bindings.append({"set": res["set"], "binding": res["binding"],
                         "type": "sampled_texture",
                         "texture": index_of(res["name"], "texture"),
                         "sampler": index_of(res["name"] + "Smplr", "sampler")})
    for res in reflection.get("separate_images", []):
        bindings.append({"set": res["set"], "binding": res["binding"], "type": "texture",
                         "texture": index_of(res["name"], "texture")})
    for res in reflection.get("separate_samplers", []):
        bindings.append({"set": res["set"], "binding": res["binding"], "type": "sampler",
                         "sampler": index_of(res["name"], "sampler")})
    for res in reflection.get("images", []):            # storage images
        bindings.append({"set": res["set"], "binding": res["binding"],
                         "type": "storage_texture",
                         "texture": index_of(res["name"], "texture")})
    for key in reflection:
        if key not in ("types", "entryPoints", "inputs", "outputs", "ubos", "ssbos",
                       "textures", "separate_images", "separate_samplers", "images",
                       "push_constants", "specialization_constants"):
            raise RuntimeError(f"{function}: resource kind {key} is not handled")
    bindings.sort(key=lambda b: (b["set"], b["binding"]))

    entry = {"file": f"{spv.stem}.metal", "stage": STAGES[stage], "bindings": bindings}
    push = reflection.get("push_constants", [])
    if push:
        entry["push_constants"] = {"buffer": index_of(push[0]["name"], "buffer")}
    if stage == "vert":
        entry["inputs"] = sorted(({"location": i["location"], "type": i["type"]}
                                  for i in reflection.get("inputs", [])),
                                 key=lambda i: i["location"])
    if stage == "comp":
        entry["threads_per_threadgroup"] = reflection["entryPoints"][0]["workgroup_size"]

    # A stage's buffer slots are shared with the vertex buffers the renderer
    # binds for stage_in, so the manifest says how many the shader itself uses.
    used = [b["buffer"] for b in bindings if "buffer" in b]
    if push:
        used.append(entry["push_constants"]["buffer"])
    entry["buffers_used"] = (max(used) + 1) if used else 0
    return function, msl, entry


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--check", action="store_true",
                        help="report stale or missing output instead of writing it")
    args = parser.parse_args()

    if not shutil.which("spirv-cross"):
        sys.exit("spirv-cross not found: apt install spirv-cross, or brew install spirv-cross")

    outputs = {}
    manifest = {"msl_version": MSL_VERSION, "functions": {}, "excluded": {}}
    for spv in sorted(SHADERS.glob("*.spv")):
        reason = excluded_reason(spv.name.split(".")[0])
        if reason:
            manifest["excluded"][spv.name] = reason
            continue
        function, msl, entry = convert(spv)
        if function in manifest["functions"]:
            sys.exit(f"two shaders make the function {function}")
        manifest["functions"][function] = entry
        outputs[OUT / entry["file"]] = msl
    outputs[OUT / "manifest.json"] = json.dumps(manifest, indent=1, sort_keys=True) + "\n"

    stale = [p for p, text in outputs.items()
             if not p.exists() or p.read_text() != text]
    orphans = [p for p in OUT.glob("*.metal") if p not in outputs] if OUT.exists() else []

    if args.check:
        for p in stale:
            print(f"stale: {p.relative_to(ROOT)}")
        for p in orphans:
            print(f"no longer generated: {p.relative_to(ROOT)}")
        if stale or orphans:
            sys.exit("Metal shaders are out of date: run tools/metal/convert_shaders.py")
        print(f"{len(manifest['functions'])} Metal shaders up to date")
        return

    OUT.mkdir(parents=True, exist_ok=True)
    for p in stale:
        p.write_text(outputs[p])
    for p in orphans:
        p.unlink()
    print(f"{len(manifest['functions'])} shaders translated, "
          f"{len(manifest['excluded'])} left out, {len(stale)} file(s) written, "
          f"{len(orphans)} removed")


if __name__ == "__main__":
    main()
