from __future__ import annotations

import asyncio
import time
from typing import Any, Dict, List, Optional

from .events import EventHub
from .models import AppState, Rect, Window, WindowType, new_id
from .ipc_client import send_cmd
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
            if wtype == WindowType.test_pattern:
                send_cmd("create_window", {"type": "test_pattern"})
            elif wtype == WindowType.gradient:
                kind = str(props.get("gradient", "horizontal"))
                send_cmd("create_window", {"type": "gradient", "gradient": kind})
            elif wtype in (WindowType.frame_player, WindowType.text_view):
                path = str(props.get("path", ""))
                if path:
                    send_cmd("create_window", {"type": wtype.value, "path": path})
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
