from __future__ import annotations

from typing import Any, Dict, List, Literal, Optional

from pydantic import BaseModel, Field


class RectModel(BaseModel):
    x: int
    y: int
    w: int
    h: int


class WindowCreate(BaseModel):
    type: Literal[
        "test_pattern",
        "gradient",
        "frame_player",
        "text_view",
        "wallpaper",
    ]
    title: Optional[str] = None
    rect: Optional[RectModel] = None
    props: Dict[str, Any] = Field(default_factory=dict)


class WindowRef(BaseModel):
    id: str


class WindowMoveResize(BaseModel):
    x: Optional[int] = None
    y: Optional[int] = None
    w: Optional[int] = None
    h: Optional[int] = None


class WindowPropsUpdate(BaseModel):
    props: Dict[str, Any]


class MenuCommand(BaseModel):
    command: str
    args: Dict[str, Any] = Field(default_factory=dict)


class PatternMode(BaseModel):
    mode: Literal["continuous", "tiled"]


class WorkspaceSave(BaseModel):
    path: str


class WorkspaceOpen(BaseModel):
    path: str


class ScreenshotReq(BaseModel):
    path: Optional[str] = None


class Capabilities(BaseModel):
    version: str
    window_types: List[str]
    commands: List[str]
    properties: Dict[str, Any]


class WindowState(BaseModel):
    id: str
    type: str
    title: str
    rect: RectModel
    z: int
    focused: bool
    zoomed: bool
    props: Dict[str, Any]


class AppStateModel(BaseModel):
    pattern_mode: str
    windows: List[WindowState]
    last_workspace: Optional[str] = None
    last_screenshot: Optional[str] = None
    uptime_sec: float

