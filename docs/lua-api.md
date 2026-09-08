# Lua Scripting API

EPANET-LSX embeds a Lua interpreter in the EPANET engine. A model opts in by
adding a `[SCRIPT]` section to its `.inp` file. That Lua code can read and modify
the network while the simulation runs. A model with no `[SCRIPT]` section behaves
exactly like stock EPANET.

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

The `[SCRIPT]` section runs until the next section header (`[...]`) or the end of
the file. The script is a normal Lua chunk with the full Lua standard library
available, plus the EPANET globals documented below.

## How scripts run

The script is compiled once when the project opens. It then runs at four points
in the engine's lifecycle, either as a **global script** (top-level code) or
through **event handlers** you define.

### Event handlers

Define any of these as global functions; each is optional.

| Handler | When it runs |
| --- | --- |
| `on_open` | After the hydraulic solver is initialized, before the first time step. |
| `on_hydraulic_step` | After each time step's hydraulics converge, before results are saved. Changes here trigger a re-solve (see below). |
| `on_hydraulics_solved` | After the step's results are saved, with those results still available. |
| `on_close` | When the hydraulic solver closes, while the network is still intact. |

### Global-script vs. handler mode

- **Handler mode** — if you define `on_hydraulic_step`, only that function runs
  on each converged step. Top-level code runs once at load time (typically to
  define functions and constants).
- **Global-script mode** — if you do *not* define `on_hydraulic_step`, the entire
  top-level chunk is re-evaluated on every converged step. Top-level `local`s are
  rebuilt each pass; globals persist across passes, so use a global table to keep
  state between steps.

Both modes may still define `on_open`, `on_hydraulics_solved`, and `on_close`.

```lua
-- Global-script mode: no on_hydraulic_step, so this whole chunk runs each step.
vsp_state = vsp_state or {}          -- global: survives between steps
local pump = link("PU2")             -- local: rebuilt each step
-- ...
```

### Re-solving after a change

When code running in `on_hydraulic_step` (or the top-level chunk in global-script
mode) changes a network property, the current solution is stale, so the engine
re-solves the step and runs the script again. This repeats until the script stops
changing the network or the iteration cap is reached.

- The cap is **``10** re-solves per time step.
- A write only counts as a change if the stored value actually moved (beyond a
  tiny tolerance). Re-writing a property to its current value does **not** keep
  the step iterating.
- Writes made in `on_hydraulics_solved` do **not** trigger a re-solve — that
  handler runs after the step is finalized and is intended for reading and
  reporting results.

## Global functions

### `node(id)`

Returns a node object for the given ID string. Reading or writing a field maps to
the EPANET toolkit's node values. An unknown ID or an unknown property raises an
error; writing a read-only property raises an error.

```lua
local p = node("J126").pressure
node("T1").elevation = 100.0
```

### `link(id)`

Returns a link object for the given ID string, with the same read/write behavior
as `node`.

```lua
link("V1").setting = link("V1").setting - diff
link("PU1").status = 0        -- close the pump
```

### `options()`

Returns the project-wide analysis options object. All fields are **read-only**.

```lua
print(options().demand_multiplier)
```

### `times()`

Returns the project-wide time parameters object. Values are in **seconds**;
writable fields are rounded to the nearest second when written.

```lua
local hour = (times().hydraulic_time % 86400) / 3600
times().report_step = 900
```

### `curve(id)`

Returns a curve's points as a 1-indexed array of `{x, y}` pairs. The returned
table is a snapshot — modifying it does not change the curve.

```lua
local pts = curve("PUMP1")
local first_x, first_y = pts[1][1], pts[1][2]
```

### `print(...)`

Replaces Lua's standard `print`. Writes its arguments, tab-separated, as a line
to the report file. Lines written from a timed context (`on_hydraulic_step`,
`on_hydraulics_solved`, or the global chunk during a step) are prefixed with the
current clock time; lines from `on_open`/`on_close` are not. Output is truncated
to 255 characters.

```lua
print("tank level", node("T1").head - node("T1").elevation)
```

## Property values and units

Field values match the EPANET toolkit's `EN_getnodevalue` / `EN_setnodevalue`,
`EN_getlinkvalue` / `EN_setlinkvalue`, `EN_getoption`, and `EN_gettimeparam` /
`EN_settimeparam`, and are expressed in the project's configured units. Writable
fields are exactly those the toolkit's setters accept.

### Node properties

| Property | Access |
| --- | --- |
| `elevation` | read/write |
| `base_demand` | read/write |
| `pattern` | read/write |
| `emitter` | read/write |
| `init_quality` | read/write |
| `source_quality` | read/write |
| `source_pattern` | read/write |
| `source_type` | read/write |
| `tank_level` | read/write |
| `tank_diameter` | read/write |
| `min_level` | read/write |
| `max_level` | read/write |
| `min_volume` | read/write |
| `volume_curve` | read/write |
| `mix_model` | read/write |
| `mix_fraction` | read/write |
| `bulk_coeff` | read/write |
| `can_overflow` | read/write |
| `demand` | read-only |
| `head` | read-only |
| `pressure` | read-only |
| `quality` | read-only |
| `source_mass` | read-only |
| `init_volume` | read-only |
| `mix_zone_volume` | read-only |
| `tank_volume` | read-only |
| `max_volume` | read-only |
| `demand_deficit` | read-only |
| `in_control` | read-only |
| `emitter_flow` | read-only |
| `leakage_flow` | read-only |
| `demand_flow` | read-only |
| `full_demand` | read-only |

> `tank_level` is the tank's *initial* level. For the current level during a run
> use `head - elevation`.

### Link properties

| Property | Access |
| --- | --- |
| `diameter` | read/write |
| `length` | read/write |
| `roughness` | read/write |
| `minor_loss` | read/write |
| `init_status` | read/write |
| `init_setting` | read/write |
| `status` | read/write |
| `setting` | read/write |
| `pattern` | read/write |
| `bulk_coeff` | read/write |
| `wall_coeff` | read/write |
| `pump_power` | read/write |
| `pump_hcurve` | read/write |
| `pump_ecurve` | read/write |
| `pump_ecost` | read/write |
| `pump_epattern` | read/write |
| `gpv_curve` | read/write |
| `pcv_curve` | read/write |
| `leak_area` | read/write |
| `leak_expansion` | read/write |
| `flow` | read-only |
| `velocity` | read-only |
| `headloss` | read-only |
| `quality` | read-only |
| `energy` | read-only |
| `pump_state` | read-only |
| `pump_efficiency` | read-only |
| `valve_type` | read-only |
| `in_control` | read-only |
| `leakage` | read-only |

### Options properties (all read-only)

`trials`, `accuracy`, `tolerance`, `emitter_exponent`, `demand_multiplier`,
`head_error`, `flow_change`, `headloss_form`, `global_efficiency`,
`global_price`, `global_pattern`, `demand_charge`, `specific_gravity`,
`specific_viscosity`, `unbalanced`, `check_frequency`, `max_check`,
`damp_limit`, `specific_diffusivity`, `bulk_order`, `wall_order`, `tank_order`,
`concentration_limit`, `demand_pattern`, `emitter_backflow`, `pressure_units`,
`status_report`.

### Times properties

Values are in seconds; writable fields are rounded to the nearest second.

| Property | Access |
| --- | --- |
| `duration` | read/write |
| `hydraulic_step` | read/write |
| `quality_step` | read/write |
| `pattern_step` | read/write |
| `pattern_start` | read/write |
| `report_step` | read/write |
| `report_start` | read/write |
| `rule_step` | read/write |
| `statistic` | read/write |
| `start_time` | read/write |
| `hydraulic_time` | read/write |
| `quality_time` | read/write |
| `periods` | read-only |
| `halt_flag` | read-only |
| `next_event` | read-only |
| `next_event_tank` | read-only |

## Examples

### Remote-controlled PRV

Hold a target pressure at a control node downstream of a PRV by shifting the
valve setting by the pressure error.

```lua
[SCRIPT]
local valve_id  = "V1"
local node_id   = "J126"
local set_point = 25.0

function on_hydraulic_step()
  local diff = node(node_id).pressure - set_point
  if math.abs(diff) > 0.01 then
    link(valve_id).setting = link(valve_id).setting - diff
  end
end
```

### Time-of-day tank level control

Switch a pump on and off against a level band that changes by time of day. This
uses global-script mode — there is no `on_hydraulic_step`, so the whole chunk
runs each step.

```lua
[SCRIPT]
local hour  = (times().hydraulic_time % 86400) / 3600
local tank  = node("T1")
local level = tank.head - tank.elevation
local pump  = link("PU1")

local on_below, off_above
if hour >= 20 or hour < 7 then
  on_below, off_above = 7.2, 7.4    -- night: fill the tank
else
  on_below, off_above = 6.0, 6.5    -- day: minimum top-up
end

if level < on_below then
  pump.status = 1
elseif level > off_above then
  pump.status = 0
end
```

More examples are available in
[`tests/fixtures/real-networks`](../tests/fixtures/real-networks): remote PRV
setpoints, flow-modulating PRVs, a PCV emulating a PRV, variable-speed pumps,
time-delayed standby pumps, float valves, and time-of-day level control.
