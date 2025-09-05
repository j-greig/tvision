from __future__ import annotations

import asyncio
import time
from typing import Any, Dict, Optional

from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware

from .controller import Controller
from .events import EventHub
from .models import Rect, WindowType

# MCP Integration
try:
    from fastapi_mcp import FastApiMCP
    MCP_AVAILABLE = True
except ImportError:
    MCP_AVAILABLE = False

from .schemas import (
    AppStateModel,
    CanvasInfo,
    Capabilities,
    MenuCommand,
    PatternMode,
    ScreenshotReq,
    WindowCreate,
    WindowMoveResize,
    WindowPropsUpdate,
    WindowState,
    WorkspaceOpen,
    WorkspaceSave,
    RectModel,
)


def make_app() -> FastAPI:
    app = FastAPI(title="Test Pattern Control API", version="v1")

    app.add_middleware(
        CORSMiddleware,
        allow_origins=["http://localhost", "http://127.0.0.1", "*"],
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )

    events = EventHub()
    ctl = Controller(events)

    # ----- Helpers -----
    def to_state_model() -> AppStateModel:
        st = asyncio.get_event_loop().run_until_complete(ctl.get_state())
        return AppStateModel(
            pattern_mode=st.pattern_mode,
            windows=[
                WindowState(
                    id=w.id,
                    type=w.type.value,
                    title=w.title,
                    rect=RectModel(x=w.rect.x, y=w.rect.y, w=w.rect.w, h=w.rect.h),
                    z=w.z,
                    focused=w.focused,
                    zoomed=w.zoomed,
                    props=w.props,
                )
                for w in st.windows
            ],
            last_workspace=st.last_workspace,
            last_screenshot=st.last_screenshot,
            uptime_sec=time.time() - st.started_at,
        )

    # ----- Routes -----
    @app.get("/health")
    async def health() -> Dict[str, Any]:
        return {"ok": True}

    @app.get("/capabilities", response_model=Capabilities)
    async def capabilities() -> Capabilities:
        return Capabilities(
            version="v1",
            window_types=[t.value for t in WindowType],
            commands=[
                "cascade",
                "tile",
                "close_all",
                "save_workspace",
                "open_workspace",
                "screenshot",
            ],
            properties={
                "frame_player": {"fps": {"type": "number", "min": 1, "max": 120}},
                "test_pattern": {"variant": {"type": "string"}},
            },
        )

    @app.get("/state", response_model=AppStateModel)
    async def state() -> AppStateModel:
        st = await ctl.get_state()
        return AppStateModel(
            pattern_mode=st.pattern_mode,
            windows=[
                WindowState(
                    id=w.id,
                    type=w.type.value,
                    title=w.title,
                    rect=RectModel(x=w.rect.x, y=w.rect.y, w=w.rect.w, h=w.rect.h),
                    z=w.z,
                    focused=w.focused,
                    zoomed=w.zoomed,
                    props=w.props,
                )
                for w in st.windows
            ],
            canvas=CanvasInfo(
                width=st.canvas_width,
                height=st.canvas_height,
                cols=st.canvas_width,
                rows=st.canvas_height
            ),
            last_workspace=st.last_workspace,
            last_screenshot=st.last_screenshot,
            uptime_sec=time.time() - st.started_at,
        )

    @app.post("/windows", response_model=WindowState)
    async def create_window(payload: WindowCreate) -> WindowState:
        try:
            wtype = WindowType(payload.type)
        except ValueError:
            raise HTTPException(status_code=400, detail="unknown window type")
        rect = Rect(**payload.rect.dict()) if payload.rect else None
        win = await ctl.create_window(wtype, title=payload.title, rect=rect, props=payload.props)
        return WindowState(
            id=win.id,
            type=win.type.value,
            title=win.title,
            rect=RectModel(x=win.rect.x, y=win.rect.y, w=win.rect.w, h=win.rect.h),
            z=win.z,
            focused=win.focused,
            zoomed=win.zoomed,
            props=win.props,
        )

    @app.post("/windows/{win_id}/move", response_model=WindowState)
    async def move(win_id: str, payload: WindowMoveResize) -> WindowState:
        try:
            win = await ctl.move_resize(win_id, x=payload.x, y=payload.y, w=payload.w, h=payload.h)
        except KeyError:
            raise HTTPException(status_code=404, detail="window not found")
        return WindowState(
            id=win.id,
            type=win.type.value,
            title=win.title,
            rect=RectModel(x=win.rect.x, y=win.rect.y, w=win.rect.w, h=win.rect.h),
            z=win.z,
            focused=win.focused,
            zoomed=win.zoomed,
            props=win.props,
        )

    @app.post("/windows/{win_id}/focus", response_model=WindowState)
    async def focus(win_id: str) -> WindowState:
        try:
            win = await ctl.focus(win_id)
        except KeyError:
            raise HTTPException(status_code=404, detail="window not found")
        return WindowState(
            id=win.id,
            type=win.type.value,
            title=win.title,
            rect=RectModel(x=win.rect.x, y=win.rect.y, w=win.rect.w, h=win.rect.h),
            z=win.z,
            focused=win.focused,
            zoomed=win.zoomed,
            props=win.props,
        )

    @app.post("/windows/{win_id}/clone", response_model=WindowState)
    async def clone(win_id: str) -> WindowState:
        try:
            win = await ctl.clone(win_id)
        except KeyError:
            raise HTTPException(status_code=404, detail="window not found")
        return WindowState(
            id=win.id,
            type=win.type.value,
            title=win.title,
            rect=RectModel(x=win.rect.x, y=win.rect.y, w=win.rect.w, h=win.rect.h),
            z=win.z,
            focused=win.focused,
            zoomed=win.zoomed,
            props=win.props,
        )

    @app.post("/windows/{win_id}/close")
    async def close(win_id: str) -> Dict[str, Any]:
        await ctl.close(win_id)
        return {"ok": True}

    @app.post("/windows/cascade")
    async def cascade() -> Dict[str, Any]:
        await ctl.cascade()
        return {"ok": True}

    @app.post("/windows/tile")
    async def tile(payload: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        cols = (payload or {}).get("cols", 2)
        await ctl.tile(cols)
        return {"ok": True}

    @app.post("/windows/close_all")
    async def close_all() -> Dict[str, Any]:
        await ctl.close_all()
        return {"ok": True}

    @app.post("/props/{win_id}")
    async def set_props(win_id: str, payload: WindowPropsUpdate) -> Dict[str, Any]:
        try:
            await ctl.set_props(win_id, payload.props)
        except KeyError:
            raise HTTPException(status_code=404, detail="window not found")
        return {"ok": True}

    @app.post("/menu/command")
    async def menu_command(payload: MenuCommand) -> Dict[str, Any]:
        res = await ctl.exec_command(payload.command, payload.args)
        if not res.get("ok"):
            raise HTTPException(status_code=400, detail=res.get("error", "command_failed"))
        return res

    @app.post("/pattern_mode")
    async def pattern_mode(payload: PatternMode) -> Dict[str, Any]:
        await ctl.set_pattern_mode(payload.mode)
        return {"ok": True}

    @app.post("/workspace/save")
    async def workspace_save(payload: WorkspaceSave) -> Dict[str, Any]:
        await ctl.save_workspace(payload.path)
        return {"ok": True, "path": payload.path}

    @app.post("/workspace/open")
    async def workspace_open(payload: WorkspaceOpen) -> Dict[str, Any]:
        await ctl.open_workspace(payload.path)
        return {"ok": True, "path": payload.path}

    @app.post("/screenshot")
    async def screenshot(payload: Optional[ScreenshotReq] = None) -> Dict[str, Any]:
        target = await ctl.screenshot((payload or ScreenshotReq()).path)
        return {"ok": True, "path": target}

    @app.websocket("/ws")
    async def ws(websocket: WebSocket) -> None:
        await websocket.accept()
        await events.add(websocket)
        try:
            while True:
                # Keep connection alive; ignore client messages for now
                await websocket.receive_text()
        except WebSocketDisconnect:
            await events.remove(websocket)

    # MCP Integration
    if MCP_AVAILABLE:
        mcp = FastApiMCP(app)
        mcp.mount_http()  # Mounts MCP server at /mcp with HTTP transport
        print("✅ MCP server mounted at /mcp")
        print("🔗 MCP URL: http://127.0.0.1:8089/mcp")
    else:
        print("⚠️  MCP not available - install 'fastapi-mcp' for MCP support")

    return app


app = make_app()


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="127.0.0.1", port=8089, log_level="info")
