# PRD: MCP-Friendly Batch Window Layout + Timeline for test-tui

Author: You + AI assist
Status: Draft for implementation
Scope: FastAPI server (tools/api_server) + MCP tools + light Controller logic

## TL;DR

- Problem: LLMs need to treat the Turbo Vision desktop as a “pixel canvas,” spawning and arranging many windows without issuing dozens of calls.
- Solution: Add a single, idempotent HTTP endpoint and MCP tool to batch-create/arrange windows (with compact macros like create_grid), plus optional scheduling fields for simple timeline choreography.
- MVP: Immediate-apply batch (no animation yet), macro.create_grid, dry_run + request_id. Keep fields for future timeline/tweening.
- Where: Extend tools/api_server (FastAPI and MCP) and Controller, reusing the current style and IPC bridge patterns.

## Link + Article Summary

- Source: https://vercel.com/blog/the-second-wave-of-mcp-building-for-llms-not-developers
- Summary: The “second wave” of MCP emphasizes building small, predictable, composable tools with tight schemas and idempotency for LLMs—not humans. Tools should be designed for iteration: observe state, plan, apply changes atomically, and stream outcomes. Our batch layout endpoint fits this approach by enabling one declarative call (with dry_run) that an LLM can use to construct complex window “scenes,” then refine.

## Goals

- One-call batch layout: create/move/close/zorder many windows in a single request.
- Compact macros: macro.create_grid (and optional macro.create_ring later) to avoid verbose payloads.
- Timeline-ready: Reserve schedule fields (`at_ms`, `delay_ms`, `stagger_ms`, `duration_ms`, `easing`) for staggered spawns and tweened move/resize.
- Idempotency + dry runs: `request_id` to dedupe and `dry_run` to simulate.
- Observability: Keep responses and events LLM-friendly; preserve current `/state`, `/screenshot`, and `/ws` flows.

## Non-Goals (MVP)

- No in-server tween animation (just immediate apply). Scheduling to be added incrementally.
- No complex conflict resolution beyond simple clamps/warnings.
- No auth/remote exposure (localhost v1 only, consistent with existing server).

## Current Setup (as of tools/api_server)

- Files (external to this repo checkout but in your monorepo):
  - `tools/api_server/main.py` (FastAPI app, routes, MCP mount)
  - `tools/api_server/controller.py` (in-memory state + IPC to test-tui via `/tmp/test_pattern_app.sock`)
  - `tools/api_server/models.py` (Window, Rect, AppState, etc.)
  - `tools/api_server/schemas.py` (Pydantic models for existing endpoints)
  - `tools/api_server/mcp_tools.py` (MCP tools like tui_create_window, tui_move_window, etc.)
  - `tools/api_server/README.md` (Run instructions, routes)

- Existing endpoints: create/move/focus/clone/close, cascade/tile/close_all, props/menu/workspace/screenshot, events.

## Proposed API (HTTP)

- POST `/windows/batch_layout`
  - One request to create/move/close/zorder many windows.
  - Supports macro ops (starting with `macro.create_grid`).
  - `dry_run` returns a plan without changing state.
  - `request_id` makes it idempotent.
  - Timeline fields accepted; MVP ignores them (immediate apply) but keeps schema stable.

- POST `/timeline/cancel` (stub for MVP)
- GET `/timeline/status?group_id=...` (stub for MVP)

### Request (BatchLayoutRequest)

```json
{
  "request_id": "grid-001",
  "dry_run": false,
  "group_id": "scene-A",
  "clock": "monotonic",
  "start_at_ms": 0,
  "defaults": { "view_type": "monster_cam", "z": "topmost", "duration_ms": 0 },
  "ops": [
    {
      "op": "macro.create_grid",
      "view_type": "monster_cam",
      "title": "Cam",
      "options": { "hud": true },
      "grid": {
        "cols": 5, "rows": 4,
        "cell_w": 32, "cell_h": 12,
        "gap_x": 2, "gap_y": 1,
        "origin": { "x": 1, "y": 1, "w": 0, "h": 0 },
        "order": "row_major"
      },
      "schedule": { "delay_ms": 0, "stagger_ms": 120 }
    }
  ]
}
```

### Response (BatchLayoutResponse)

```json
{
  "dry_run": false,
  "applied": true,
  "group_id": "scene-A",
  "op_results": [
    { "status": "applied", "window_id": "win_abc123", "final_bounds": {"x":1,"y":1,"w":32,"h":12} },
    { "status": "applied", "window_id": "win_abc124", "final_bounds": {"x":35,"y":1,"w":32,"h":12} }
  ],
  "warnings": [],
  "timeline_summary": { "t0_ms": 0, "t1_ms": 0, "counts": {"ops": 20} }
}
```

## Pydantic Models (schemas.py)

Add near existing models:

```python
from typing import Union
from pydantic import BaseModel, Field
from typing import Optional, List, Dict, Literal, Any

class ScheduleModel(BaseModel):
    at_ms: Optional[int] = None
    delay_ms: Optional[int] = None
    stagger_ms: Optional[int] = None
    duration_ms: Optional[int] = None
    easing: Optional[Literal["linear","ease_in","ease_out","ease_in_out"]] = "linear"

class BoundsModel(BaseModel):
    x: int; y: int; w: int; h: int

class GridMacro(BaseModel):
    cols: int; rows: int; cell_w: int; cell_h: int
    gap_x: int = 0; gap_y: int = 0
    origin: BoundsModel = Field(default_factory=lambda: BoundsModel(x=1, y=1, w=0, h=0))
    order: Literal["row_major","col_major"] = "row_major"

class RingMacro(BaseModel):
    cx: int; cy: int; radius: int; count: int; w: int; h: int
    rotate: bool = False; jitter: int = 0

class BatchOp(BaseModel):
    op: Literal["create","move_resize","close","zorder","macro.create_grid","macro.create_ring"]
    window_id: Optional[str] = None
    view_type: Optional[str] = None
    title: Optional[str] = None
    bounds: Optional[BoundsModel] = None
    z: Optional[Union[int, Literal["raise","lower","topmost"]]] = None
    options: Dict[str, Any] = Field(default_factory=dict)
    schedule: Optional[ScheduleModel] = None
    grid: Optional[GridMacro] = None
    ring: Optional[RingMacro] = None

class BatchDefaults(BaseModel):
    view_type: Optional[str] = None
    z: Optional[Union[int, Literal["raise","lower","topmost"]]] = None
    easing: Optional[str] = "linear"
    duration_ms: Optional[int] = 0

class BatchLayoutRequest(BaseModel):
    request_id: str
    dry_run: bool = False
    group_id: Optional[str] = None
    clock: Literal["monotonic","wall"] = "monotonic"
    start_at_ms: Optional[int] = None
    defaults: Optional[BatchDefaults] = None
    ops: List[BatchOp]

class BatchOpResult(BaseModel):
    status: Literal["scheduled","applied","rejected"]
    reason: Optional[str] = None
    window_id: Optional[str] = None
    effective_time_ms: Optional[int] = None
    final_bounds: Optional[BoundsModel] = None

class TimelineSummary(BaseModel):
    t0_ms: Optional[int] = None
    t1_ms: Optional[int] = None
    counts: Dict[str,int] = Field(default_factory=dict)

class BatchLayoutResponse(BaseModel):
    dry_run: bool
    applied: bool
    group_id: Optional[str]
    op_results: List[BatchOpResult]
    warnings: List[str] = Field(default_factory=list)
    timeline_summary: Optional[TimelineSummary] = None
```

## Controller Methods (controller.py)

Imports:

```python
from .schemas import (
    BatchLayoutRequest, BatchLayoutResponse, BatchOp, BatchOpResult,
    BoundsModel, TimelineSummary,
)
```

Init fields:

```python
self._requests: Dict[str, BatchLayoutResponse] = {}
self._timelines: Dict[str, List[asyncio.Task]] = {}
```

Core method (MVP immediate-apply):

```python
async def batch_layout(self, req: BatchLayoutRequest) -> BatchLayoutResponse:
    if req.request_id in self._requests:
        return self._requests[req.request_id]

    results: List[BatchOpResult] = []
    warnings: List[str] = []
    expanded_ops: List[BatchOp] = []

    # Macro expansion (grid only in MVP)
    for op in req.ops:
        if op.op == "macro.create_grid" and op.grid:
            g = op.grid
            vt = op.view_type or (req.defaults.view_type if req.defaults else None)
            if not vt:
                results.append(BatchOpResult(status="rejected", reason="missing view_type for macro"))
                continue
            idx = 0
            for r in range(g.rows):
                for c in range(g.cols):
                    x = g.origin.x + c*(g.cell_w + g.gap_x)
                    y = g.origin.y + r*(g.cell_h + g.gap_y)
                    expanded_ops.append(BatchOp(
                        op="create", view_type=vt, title=(op.title or f"{vt} #{idx+1}"),
                        bounds=BoundsModel(x=x, y=y, w=g.cell_w, h=g.cell_h),
                        options=dict(op.options or {}), schedule=op.schedule,
                    ))
                    idx += 1
        else:
            expanded_ops.append(op)

    # MVP: apply immediately (ignore schedule fields)
    for eop in expanded_ops:
        try:
            if eop.op == "create":
                from .models import WindowType, Rect
                wtype = WindowType(eop.view_type or (req.defaults.view_type if req.defaults else "test_pattern"))
                rect = Rect(eop.bounds.x, eop.bounds.y, eop.bounds.w, eop.bounds.h) if eop.bounds else None
                win = await self.create_window(wtype, eop.title, rect, eop.options or {})
                results.append(BatchOpResult(status="applied", window_id=win.id,
                    final_bounds=BoundsModel(x=win.rect.x, y=win.rect.y, w=win.rect.w, h=win.rect.h)))
            elif eop.op == "move_resize" and eop.window_id and eop.bounds:
                win = await self.move_resize(eop.window_id, x=eop.bounds.x, y=eop.bounds.y, w=eop.bounds.w, h=eop.bounds.h)
                results.append(BatchOpResult(status="applied", window_id=win.id,
                    final_bounds=BoundsModel(x=win.rect.x, y=win.rect.y, w=win.rect.w, h=win.rect.h)))
            elif eop.op == "close" and eop.window_id:
                await self.close(eop.window_id)
                results.append(BatchOpResult(status="applied", window_id=eop.window_id))
            else:
                results.append(BatchOpResult(status="rejected", reason="unsupported_or_invalid_op"))
        except Exception as ex:
            results.append(BatchOpResult(status="rejected", reason=str(ex)))

    resp = BatchLayoutResponse(
        dry_run=req.dry_run, applied=(not req.dry_run), group_id=req.group_id,
        op_results=results, warnings=warnings,
        timeline_summary=TimelineSummary(counts={"ops": len(results)}),
    )
    self._requests[req.request_id] = resp
    return resp

async def cancel_timeline(self, group_id: str) -> Dict[str, Any]:
    return {"ok": True, "group_id": group_id, "canceled": 0}

async def get_timeline_status(self, group_id: str) -> Dict[str, Any]:
    return {"group_id": group_id, "scheduled": 0, "pending": 0, "applied": 0}
```

## FastAPI Routes (main.py)

```python
from .schemas import BatchLayoutRequest, BatchLayoutResponse

@app.post("/windows/batch_layout", response_model=BatchLayoutResponse)
async def windows_batch_layout(payload: BatchLayoutRequest) -> BatchLayoutResponse:
    return await ctl.batch_layout(payload)

@app.post("/timeline/cancel")
async def timeline_cancel(body: Dict[str, Any]) -> Dict[str, Any]:
    gid = str((body or {}).get("group_id", ""))
    if not gid:
        raise HTTPException(status_code=400, detail="missing group_id")
    return await ctl.cancel_timeline(gid)

@app.get("/timeline/status")
async def timeline_status(group_id: str) -> Dict[str, Any]:
    return await ctl.get_timeline_status(group_id)
```

## MCP Tools (mcp_tools.py)

```python
@mcp.tool("tui_batch_layout")
async def tui_batch_layout(request: Dict[str, Any]) -> Dict[str, Any]:
    controller = get_controller()
    try:
        from .schemas import BatchLayoutRequest
        model = BatchLayoutRequest(**request)
        resp = await controller.batch_layout(model)
        return {"success": True, "result": resp.dict()}
    except Exception as e:
        return {"success": False, "error": str(e)}

@mcp.tool("tui_timeline_cancel")
async def tui_timeline_cancel(group_id: str) -> Dict[str, Any]:
    controller = get_controller()
    try:
        res = await controller.cancel_timeline(group_id)
        return {"success": True, **res}
    except Exception as e:
        return {"success": False, "error": str(e)}

@mcp.tool("tui_timeline_status")
async def tui_timeline_status(group_id: str) -> Dict[str, Any]:
    controller = get_controller()
    try:
        res = await controller.get_timeline_status(group_id)
        return {"success": True, **res}
    except Exception as e:
        return {"success": False, "error": str(e)}
```

## Examples (curl)

Spawn a 5×4 grid of Monster Cams (immediate apply):

```bash
curl -X POST localhost:8089/windows/batch_layout \
  -H 'Content-Type: application/json' \
  -d '{
    "request_id":"grid-001",
    "group_id":"scene-A",
    "defaults": {"view_type":"monster_cam"},
    "ops":[{
      "op":"macro.create_grid",
      "grid":{"cols":5,"rows":4,"cell_w":32,"cell_h":12,"gap_x":2,"gap_y":1,
               "origin":{"x":1,"y":1,"w":0,"h":0}},
      "options":{"hud":true}
    }] }'
```

Check timeline status (MVP stub):

```bash
curl "localhost:8089/timeline/status?group_id=scene-A"
```

## Implementation Notes

- Follow the style of existing routes and controller patterns; keep logic in `Controller` and emit events via `EventHub` when extended.
- Start with immediate-apply MVP: ignore schedule fields for now while accepting them in schema.
- Add `request_id` idempotency cache in Controller to avoid duplicate spawns on retries.
- Defer tween animation and true scheduling to a follow-up PR: use asyncio tasks keyed by `group_id`.
- Keep MCP tools thin wrappers over Controller; avoid loopback HTTP.

## Testing Plan

- Unit-test `Controller.batch_layout` macro expansion and immediate apply (mock `create_window` and `move_resize`).
- Live test: start FastAPI (`python -m tools.api_server`), run test-tui, call `/windows/batch_layout`, then `/state` and `/screenshot`.
- MCP: call `tui_batch_layout` from your MCP host; verify events and state.

## Risks / Mitigations

- Large batches: cap ops count (e.g., 256) and validate bounds to avoid off-screen thrash.
- Inconsistent IDs: Controller should sync with the real app via IPC if available to reconcile window IDs.
- Scheduling drift: when implemented, use monotonic clocks and per-slice atomicity; expose `/timeline/status` to debug.

## Deliverables Checklist

- [ ] schemas.py: new models
- [ ] controller.py: batch_layout, cancel_timeline, get_timeline_status
- [ ] main.py: three new routes
- [ ] mcp_tools.py: three new tools
- [ ] README.md: endpoint docs + examples

