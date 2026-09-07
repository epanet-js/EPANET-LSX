# EPANET-LSX

Lua Scripting Extensions for [EPANET](https://github.com/OpenWaterAnalytics/EPANET).

EPANET-LSX builds the EPANET engine shared library with an embedded Lua
interpreter, so a hydraulic model can express custom control logic inside the
`.inp` file.

The `[SCRIPT]` section holds Lua code that modifies the behavior of the
network as the simulation runs. This allows control schemes that EPANET's
built-in controls and rules cannot express.

## Why include Lua in EPANET

EPANET's built-in controls (simple and rule-based) have served us well, but their limitations are well known. Until now, the best way to model more advanced control logic has been to use the toolkit to modify settings during the hydraulic loop - which works, but ties the model to the toolkit and whatever programming language you picked.

As hydraulic models shift from planning tools towards near real-time operational tools, being able to replicate the actual control logic of a system matters more, and portability matters with it.

Commercial tools have solved this by building their own derivatives of the EPANET engine, but that has created interoperability problems of its own. Embedding Lua as a scripting language directly in EPANET gives modellers the flexibility to build advanced controls while keeping the model portable - the same INP file, controls included, runs in any tool built on the engine.

There are a set of working examples in the [`tests/fixtures/real-networks`](tests/fixtures/real-networks) directory, including:
remote PRV setpoints, flow-modulating PRVs, a PCV emulating a PRV, variable-speed
pumps, time-delayed standby pumps, float valves, and time-of-day level control.

## How it works

A model opts in by adding a `[SCRIPT]` section to its `.inp` file:

```lua
[SCRIPT]
-- Hold 25 m of pressure at a remote control node by shifting a PRV's setting.
function on_hydraulic_step()
  local diff = node("J126").pressure - 25.0
  if math.abs(diff) > 0.01 then
    link("V1").setting = link("V1").setting - diff
  end
end
```

Scripts run either as a global script, run on every hydraulic step, or
through event handlers: `on_open`, `on_hydraulic_step`, `on_hydraulics_solved`,
and `on_close`.

When a script changes the network, the step is re-solved and the
script runs again until it settles or an iteration cap is reached. The Lua API
exposes `node(id)`, `link(id)`, `options()`, `times()`, `curve(id)`, and
`print()`. A model with no `[SCRIPT]` section behaves exactly like stock EPANET.

## Building

There are pre-compiled binaries for Windows, Linux and macOS available to download in releases.
If you want to build LSX yourself, read [building.md](docs/building.md).

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) and the guidelines under [`docs/`](docs).
