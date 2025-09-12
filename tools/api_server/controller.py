from __future__ import annotations

import asyncio
import time
from typing import Any, Dict, List, Optional

from .events import EventHub
from .models import AppState, Rect, Window, WindowType, new_id
from .ipc_client import send_cmd
from .schemas import (
    BatchLayoutRequest, 
    BatchLayoutResponse, 
    BatchOp, 
    BatchOpResult,
    BoundsModel, 
    TimelineSummary,
)
import json


class Controller:
    """In-memory controller for the API MVP.

    Designed to be replaced by an IPC-backed adapter that forwards calls to
    the Turbo Vision app and reflects authoritative state back.
    """

    def __init__(self, events: EventHub) -> None:
        self._state = AppState()
        self._events = events
        self._lock = asyncio.Lock()
        # Batch layout support
        self._requests: Dict[str, BatchLayoutResponse] = {}
        self._timelines: Dict[str, List[asyncio.Task]] = {}

    # ----- Query -----
    async def get_state(self) -> AppState:
        await self._sync_state()
        return self._state

    async def _sync_state(self) -> None:
        """Sync in-memory state with the real C++ app via IPC"""
        try:
            resp = send_cmd("get_state")
            if resp and resp.strip():
                # Parse JSON response
                state_data = json.loads(resp.strip())
                
                async with self._lock:
                    # Update windows list with real IDs from C++
                    new_windows = []
                    for win_data in state_data.get("windows", []):
                        win = Window(
                            id=win_data["id"],
                            type=WindowType.test_pattern,  # Default for now
                            title=win_data["title"],
                            rect=Rect(
                                x=win_data["x"],
                                y=win_data["y"], 
                                w=win_data["width"],
                                h=win_data["height"]
                            ),
                            z=0,  # C++ doesn't provide z-order
                            focused=False,  # Will be determined later
                            props={}
                        )
                        new_windows.append(win)
                    
                    self._state.windows = new_windows
                    
            # Get canvas size separately
            canvas_resp = send_cmd("get_canvas_size")
            if canvas_resp and canvas_resp.strip():
                canvas_data = json.loads(canvas_resp.strip())
                self._state.canvas_width = canvas_data.get("width", 80)
                self._state.canvas_height = canvas_data.get("height", 25)
        except Exception:
            # If sync fails, keep existing state
            pass

    # ----- Windows -----
    async def create_window(
        self,
        wtype: WindowType,
        title: Optional[str],
        rect: Optional[Rect],
        props: Dict[str, Any],
    ) -> Window:
        # Try forwarding to the live app via IPC (best-effort)
        try:
            cmd_params = {"type": wtype.value}
            
            # Add positioning parameters if rect is provided
            if rect:
                cmd_params.update({
                    "x": str(rect.x),
                    "y": str(rect.y), 
                    "w": str(rect.w),
                    "h": str(rect.h)
                })
            
            if wtype == WindowType.test_pattern:
                send_cmd("create_window", cmd_params)
            elif wtype == WindowType.gradient:
                kind = str(props.get("gradient", "horizontal"))
                cmd_params["gradient"] = kind
                send_cmd("create_window", cmd_params)
            elif wtype in (WindowType.frame_player, WindowType.text_view):
                path = str(props.get("path", ""))
                if path:
                    cmd_params["path"] = path
                    send_cmd("create_window", cmd_params)
            elif wtype == WindowType.text_editor:
                send_cmd("create_window", cmd_params)
        except Exception:
            pass
        async with self._lock:
            win_id = new_id("win")
            if not rect:
                rect = Rect(3 + len(self._state.windows) * 2, 2 + len(self._state.windows) * 1, 40, 12)
            t = title or wtype.value
            win = Window(
                id=win_id,
                type=wtype,
                title=t,
                rect=rect,
                z=self._state.next_z,
                focused=True,
                props=props or {},
            )
            self._state.next_z += 1
            # unfocus others
            for w in self._state.windows:
                w.focused = False
            self._state.windows.append(win)
        await self._events.emit("window.created", self._serialize_window(win))
        return win

    async def move_resize(self, win_id: str, *, x=None, y=None, w=None, h=None) -> Window:
        # Sync state first to get current window info
        await self._sync_state()
        
        try:
            # Find the window after syncing
            win = self._require(win_id)
            
            if x is not None or y is not None:
                move_x = x if x is not None else win.rect.x
                move_y = y if y is not None else win.rect.y
                send_cmd("move_window", {"id": win_id, "x": str(move_x), "y": str(move_y)})
            
            if w is not None or h is not None:
                new_w = w if w is not None else win.rect.w
                new_h = h if h is not None else win.rect.h
                send_cmd("resize_window", {"id": win_id, "width": str(new_w), "height": str(new_h)})
        except Exception:
            pass
        
        # Sync state again and return updated window
        await self._sync_state()
        try:
            win = self._require(win_id)
            await self._events.emit("window.updated", self._serialize_window(win))
            return win
        except KeyError:
            raise KeyError(win_id)

    async def focus(self, win_id: str) -> Window:
        # Send focus command to C++ app
        try:
            send_cmd("focus_window", {"id": win_id})
        except Exception:
            pass
        
        # Sync state and return focused window
        await self._sync_state()
        try:
            win = self._require(win_id)
            await self._events.emit("window.updated", self._serialize_window(win))
            return win
        except KeyError:
            raise KeyError(win_id)

    async def clone(self, win_id: str) -> Window:
        async with self._lock:
            src = self._require(win_id)
            new_rect = Rect(src.rect.x + 2, src.rect.y + 1, src.rect.w, src.rect.h)
            clone = Window(
                id=new_id("win"),
                type=src.type,
                title=f"{src.title} (copy)",
                rect=new_rect,
                z=self._state.next_z,
                focused=True,
                props=dict(src.props),
            )
            self._state.next_z += 1
            for w in self._state.windows:
                w.focused = False
            self._state.windows.append(clone)
        await self._events.emit("window.created", self._serialize_window(clone))
        return clone

    async def close(self, win_id: str) -> None:
        # Send close command to C++ app
        try:
            send_cmd("close_window", {"id": win_id})
        except Exception:
            pass
        
        # Sync state and emit event
        await self._sync_state()
        await self._events.emit("window.closed", {"id": win_id})

    async def close_all(self) -> None:
        try:
            send_cmd("close_all")
        except Exception:
            pass
        async with self._lock:
            ids = [w.id for w in self._state.windows]
            self._state.windows.clear()
        for wid in ids:
            await self._events.emit("window.closed", {"id": wid})

    async def cascade(self) -> None:
        try:
            send_cmd("cascade")
        except Exception:
            pass
        async with self._lock:
            x, y = 3, 2
            for w in self._state.windows:
                w.rect.x, w.rect.y = x, y
                x += 2
                y += 1
        await self._events.emit("layout.cascade", {})

    async def tile(self, cols: int = 2) -> None:
        try:
            send_cmd("tile", {"cols": str(cols)})
        except Exception:
            pass
        async with self._lock:
            if not self._state.windows:
                return
            # simple grid tile
            col = 0
            row = 0
            base_w, base_h = 40, 12
            for w in self._state.windows:
                w.rect.x = col * (base_w + 2) + 2
                w.rect.y = row * (base_h + 1) + 1
                w.rect.w = base_w
                w.rect.h = base_h
                col += 1
                if col >= cols:
                    col = 0
                    row += 1
        await self._events.emit("layout.tile", {"cols": cols})

    # ----- Properties / Commands -----
    async def set_props(self, win_id: str, props: Dict[str, Any]) -> Window:
        async with self._lock:
            win = self._require(win_id)
            win.props.update(props)
        await self._events.emit("window.updated", self._serialize_window(win))
        return win

    async def send_text(self, win_id: str, content: str, mode: str = "append", position: str = "end") -> Dict[str, Any]:
        """Send text to a text editor window"""
        try:
            # Forward to the live app via IPC
            send_cmd("send_text", {
                "id": win_id,
                "content": content,
                "mode": mode,
                "position": position
            })
            
            # Update in-memory state (simplified)
            async with self._lock:
                win = self._require(win_id)
                if win.type == WindowType.text_editor:
                    if "content" not in win.props:
                        win.props["content"] = ""
                    
                    if mode == "replace":
                        win.props["content"] = content
                    elif mode == "append":
                        win.props["content"] += content
                    # For insert mode, we'd need cursor position - simplified for MVP
                    
            await self._events.emit("text.sent", {"window_id": win_id, "content": content, "mode": mode})
            return {"ok": True, "window_id": win_id}
            
        except Exception as e:
            return {"ok": False, "error": str(e)}

    async def send_figlet(self, win_id: str, text: str, font: str = "standard", width: int = 0, mode: str = "append") -> Dict[str, Any]:
        """Send figlet ASCII art to a text editor window"""
        try:
            # Forward to the live app via IPC
            send_cmd("send_figlet", {
                "id": win_id,
                "text": text,
                "font": font,
                "width": str(width) if width > 0 else "",
                "mode": mode
            })
            
            # Update in-memory state (simplified)
            async with self._lock:
                win = self._require(win_id)
                if win.type == WindowType.text_editor:
                    if "figlet_history" not in win.props:
                        win.props["figlet_history"] = []
                    win.props["figlet_history"].append({"text": text, "font": font, "width": width})
                    
            await self._events.emit("figlet.sent", {"window_id": win_id, "text": text, "font": font, "mode": mode})
            return {"ok": True, "window_id": win_id, "text": text, "font": font}
            
        except Exception as e:
            return {"ok": False, "error": str(e)}

    async def exec_command(self, name: str, args: Dict[str, Any]) -> Dict[str, Any]:
        # Simulate a few commands
        handled = {"command": name, "ok": True}
        if name == "cascade":
            await self.cascade()
        elif name == "tile":
            await self.tile(args.get("cols", 2))
        elif name == "close_all":
            await self.close_all()
        elif name == "save_workspace":
            try:
                p = str(args.get("path", "workspaces/last_workspace.json"))
                send_cmd("save_workspace", {"path": p})
            except Exception:
                pass
            await self.save_workspace(args.get("path", "workspace.json"))
        elif name == "open_workspace":
            try:
                p = str(args.get("path", "workspaces/last_workspace.json"))
                send_cmd("open_workspace", {"path": p})
            except Exception:
                pass
            await self.open_workspace(args.get("path", "workspace.json"))
        elif name == "screenshot":
            try:
                send_cmd("screenshot")
            except Exception:
                pass
            await self.screenshot(args.get("path"))
        else:
            handled["ok"] = False
            handled["error"] = "unknown_command"
        await self._events.emit("command.executed", handled)
        return handled

    async def set_pattern_mode(self, mode: str) -> None:
        try:
            send_cmd("pattern_mode", {"mode": mode})
        except Exception:
            pass
        async with self._lock:
            self._state.pattern_mode = mode
        await self._events.emit("pattern.mode", {"mode": mode})

    # ----- Workspace / Screenshot -----
    async def save_workspace(self, path: str) -> None:
        async with self._lock:
            self._state.last_workspace = path
        await self._events.emit("workspace.saved", {"path": path})

    async def open_workspace(self, path: str) -> None:
        async with self._lock:
            self._state.last_workspace = path
        await self._events.emit("workspace.opened", {"path": path})

    async def screenshot(self, path: Optional[str]) -> str:
        target = path or f"screenshot_{int(time.time())}.png"
        async with self._lock:
            self._state.last_screenshot = target
        await self._events.emit("screenshot.saved", {"path": target})
        return target

    # ----- Helpers -----
    def _require(self, win_id: str) -> Window:
        for w in self._state.windows:
            if w.id == win_id:
                return w
        raise KeyError(win_id)

    def _serialize_window(self, w: Window) -> Dict[str, Any]:
        return {
            "id": w.id,
            "type": w.type.value,
            "title": w.title,
            "rect": {"x": w.rect.x, "y": w.rect.y, "w": w.rect.w, "h": w.rect.h},
            "z": w.z,
            "focused": w.focused,
            "zoomed": w.zoomed,
            "props": dict(w.props),
        }

    # ----- Batch Layout -----
    async def batch_layout(self, req: BatchLayoutRequest) -> BatchLayoutResponse:
        """Batch create/move/close windows with macro expansion (MVP: immediate apply)"""
        # Idempotency check
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
                    results.append(BatchOpResult(
                        status="rejected", 
                        reason="missing view_type for macro.create_grid"
                    ))
                    continue
                
                # Expand grid macro into individual create operations
                idx = 0
                if g.order == "row_major":
                    for r in range(g.rows):
                        for c in range(g.cols):
                            x = g.origin.x + c * (g.cell_w + g.gap_x)
                            y = g.origin.y + r * (g.cell_h + g.gap_y)
                            expanded_ops.append(BatchOp(
                                op="create",
                                view_type=vt,
                                title=(op.title or f"{vt} #{idx+1}"),
                                bounds=BoundsModel(x=x, y=y, w=g.cell_w, h=g.cell_h),
                                options=dict(op.options or {}),
                                schedule=op.schedule,
                            ))
                            idx += 1
                else:  # col_major
                    for c in range(g.cols):
                        for r in range(g.rows):
                            x = g.origin.x + c * (g.cell_w + g.gap_x)
                            y = g.origin.y + r * (g.cell_h + g.gap_y)
                            expanded_ops.append(BatchOp(
                                op="create",
                                view_type=vt,
                                title=(op.title or f"{vt} #{idx+1}"),
                                bounds=BoundsModel(x=x, y=y, w=g.cell_w, h=g.cell_h),
                                options=dict(op.options or {}),
                                schedule=op.schedule,
                            ))
                            idx += 1
            else:
                # Pass through non-macro operations
                expanded_ops.append(op)

        # MVP: apply operations immediately (ignore schedule fields for now)
        if not req.dry_run:
            for eop in expanded_ops:
                try:
                    if eop.op == "create" and eop.view_type:
                        # Map view_type string to WindowType enum
                        try:
                            wtype = WindowType(eop.view_type)
                        except ValueError:
                            results.append(BatchOpResult(
                                status="rejected",
                                reason=f"unsupported view_type: {eop.view_type}"
                            ))
                            continue
                        
                        rect = None
                        if eop.bounds:
                            rect = Rect(eop.bounds.x, eop.bounds.y, eop.bounds.w, eop.bounds.h)
                        
                        win = await self.create_window(wtype, eop.title, rect, eop.options or {})
                        results.append(BatchOpResult(
                            status="applied",
                            window_id=win.id,
                            final_bounds=BoundsModel(x=win.rect.x, y=win.rect.y, w=win.rect.w, h=win.rect.h)
                        ))
                    
                    elif eop.op == "move_resize" and eop.window_id and eop.bounds:
                        win = await self.move_resize(
                            eop.window_id, 
                            x=eop.bounds.x, 
                            y=eop.bounds.y, 
                            w=eop.bounds.w, 
                            h=eop.bounds.h
                        )
                        results.append(BatchOpResult(
                            status="applied",
                            window_id=win.id,
                            final_bounds=BoundsModel(x=win.rect.x, y=win.rect.y, w=win.rect.w, h=win.rect.h)
                        ))
                    
                    elif eop.op == "close" and eop.window_id:
                        await self.close(eop.window_id)
                        results.append(BatchOpResult(
                            status="applied",
                            window_id=eop.window_id
                        ))
                    
                    else:
                        results.append(BatchOpResult(
                            status="rejected",
                            reason=f"unsupported or invalid operation: {eop.op}"
                        ))
                        
                except Exception as ex:
                    results.append(BatchOpResult(
                        status="rejected",
                        reason=str(ex)
                    ))
        else:
            # Dry run: simulate operations without applying
            for eop in expanded_ops:
                if eop.op == "create":
                    results.append(BatchOpResult(
                        status="scheduled",
                        reason="dry_run simulation",
                        final_bounds=eop.bounds
                    ))
                else:
                    results.append(BatchOpResult(
                        status="scheduled", 
                        reason="dry_run simulation"
                    ))

        resp = BatchLayoutResponse(
            dry_run=req.dry_run,
            applied=(not req.dry_run),
            group_id=req.group_id,
            op_results=results,
            warnings=warnings,
            timeline_summary=TimelineSummary(counts={"ops": len(results)})
        )
        
        # Cache response for idempotency
        self._requests[req.request_id] = resp
        return resp

    async def cancel_timeline(self, group_id: str) -> Dict[str, Any]:
        """Cancel scheduled timeline operations (MVP stub)"""
        return {
            "ok": True,
            "group_id": group_id,
            "canceled": 0
        }

    async def get_timeline_status(self, group_id: str) -> Dict[str, Any]:
        """Get timeline operation status (MVP stub)"""
        return {
            "group_id": group_id,
            "scheduled": 0,
            "pending": 0,
            "applied": 0
        }
