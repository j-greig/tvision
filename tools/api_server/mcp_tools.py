"""MCP Tools for TUI Application Control

This module defines MCP (Model Context Protocol) tools that enable AI agents
like Claude Code to programmatically control TUI applications via standardized
tool interfaces.
"""

from typing import Optional, Dict, Any, List
from .controller import Controller
from .models import WindowType, Rect


# Note: Controller instance will be passed from main.py
_controller: Optional[Controller] = None

def set_controller(controller: Controller) -> None:
    """Set the controller instance (called from main.py)"""
    global _controller
    _controller = controller

def get_controller() -> Controller:
    """Get the controller instance"""
    if _controller is None:
        raise RuntimeError("Controller not initialized - call set_controller() first")
    return _controller

def serialize_window(window) -> Dict[str, Any]:
    """Serialize window object for MCP responses"""
    return {
        "id": window.id,
        "type": window.type.value,
        "title": window.title,
        "rect": {
            "x": window.rect.x,
            "y": window.rect.y, 
            "width": window.rect.w,
            "height": window.rect.h
        },
        "z": window.z,
        "focused": window.focused,
        "props": dict(window.props)
    }

def register_tui_tools(mcp):
    """Register all TUI control tools with the MCP server"""
    
    @mcp.tool("tui_get_state")
    async def get_tui_state() -> Dict[str, Any]:
        """Get current TUI application state and window list
        
        Returns:
            Dictionary containing current windows, pattern mode, and uptime
        """
        controller = get_controller()
        state = await controller.get_state()
        return {
            "windows": [serialize_window(w) for w in state.windows],
            "pattern_mode": state.pattern_mode,
            "uptime_sec": state.uptime_sec,
            "last_workspace": state.last_workspace,
            "last_screenshot": state.last_screenshot
        }
    
    @mcp.tool("tui_create_window")
    async def create_tui_window(
        window_type: str,
        title: Optional[str] = None,
        x: Optional[int] = None,
        y: Optional[int] = None,
        width: Optional[int] = None,
        height: Optional[int] = None,
        **props
    ) -> Dict[str, Any]:
        """Create a new TUI window
        
        Args:
            window_type: Type of window ('test_pattern', 'gradient', 'frame_player', 'text_view')
            title: Optional window title
            x, y: Optional position (defaults to cascade)
            width, height: Optional size (defaults to window type default)
            **props: Additional properties (e.g., gradient='radial', path='file.txt')
            
        Returns:
            Dictionary containing created window details
        """
        controller = get_controller()
        
        try:
            wtype = WindowType(window_type)
        except ValueError:
            return {
                "success": False,
                "error": f"Invalid window type: {window_type}",
                "valid_types": [t.value for t in WindowType]
            }
        
        rect = None
        if x is not None and y is not None:
            w = width or 40
            h = height or 12
            rect = Rect(x, y, w, h)
            
        try:
            window = await controller.create_window(wtype, title, rect, props)
            return {
                "success": True,
                "window": serialize_window(window)
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    @mcp.tool("tui_move_window")
    async def move_tui_window(
        window_id: str,
        x: Optional[int] = None,
        y: Optional[int] = None,
        width: Optional[int] = None,
        height: Optional[int] = None
    ) -> Dict[str, Any]:
        """Move or resize a TUI window
        
        Args:
            window_id: ID of window to move/resize
            x, y: New position (optional, keeps current if not specified)
            width, height: New size (optional, keeps current if not specified)
            
        Returns:
            Dictionary containing success status and updated window details
        """
        controller = get_controller()
        
        try:
            window = await controller.move_resize(
                window_id, x=x, y=y, w=width, h=height
            )
            return {
                "success": True,
                "window": serialize_window(window)
            }
        except KeyError:
            return {
                "success": False,
                "error": f"Window not found: {window_id}"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    @mcp.tool("tui_focus_window")
    async def focus_tui_window(window_id: str) -> Dict[str, Any]:
        """Focus a TUI window (bring to front)
        
        Args:
            window_id: ID of window to focus
            
        Returns:
            Dictionary containing success status and focused window details
        """
        controller = get_controller()
        
        try:
            window = await controller.focus(window_id)
            return {
                "success": True,
                "window": serialize_window(window)
            }
        except KeyError:
            return {
                "success": False,
                "error": f"Window not found: {window_id}"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    @mcp.tool("tui_close_window")
    async def close_tui_window(window_id: str) -> Dict[str, Any]:
        """Close a specific TUI window
        
        Args:
            window_id: ID of window to close
            
        Returns:
            Dictionary containing success status
        """
        controller = get_controller()
        
        try:
            await controller.close(window_id)
            return {
                "success": True,
                "message": f"Window {window_id} closed"
            }
        except KeyError:
            return {
                "success": False,
                "error": f"Window not found: {window_id}"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    @mcp.tool("tui_cascade_windows")
    async def cascade_tui_windows() -> Dict[str, Any]:
        """Arrange all windows in cascade layout
        
        Returns:
            Dictionary containing success status
        """
        controller = get_controller()
        
        try:
            await controller.cascade()
            return {
                "success": True,
                "message": "Windows arranged in cascade layout"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    @mcp.tool("tui_tile_windows")
    async def tile_tui_windows(columns: Optional[int] = 2) -> Dict[str, Any]:
        """Arrange all windows in tiled layout
        
        Args:
            columns: Number of columns for tiling (default: 2)
            
        Returns:
            Dictionary containing success status
        """
        controller = get_controller()
        
        try:
            await controller.tile(columns)
            return {
                "success": True,
                "message": f"Windows tiled in {columns} columns"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    @mcp.tool("tui_close_all_windows")
    async def close_all_tui_windows() -> Dict[str, Any]:
        """Close all TUI windows
        
        Returns:
            Dictionary containing success status
        """
        controller = get_controller()
        
        try:
            await controller.close_all()
            return {
                "success": True,
                "message": "All windows closed"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    @mcp.tool("tui_set_pattern_mode")
    async def set_tui_pattern_mode(mode: str) -> Dict[str, Any]:
        """Set the test pattern display mode
        
        Args:
            mode: Pattern mode ('continuous' or 'tiled')
            
        Returns:
            Dictionary containing success status
        """
        controller = get_controller()
        
        if mode not in ['continuous', 'tiled']:
            return {
                "success": False,
                "error": f"Invalid mode: {mode}",
                "valid_modes": ["continuous", "tiled"]
            }
        
        try:
            await controller.set_pattern_mode(mode)
            return {
                "success": True,
                "message": f"Pattern mode set to {mode}"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    @mcp.tool("tui_screenshot")
    async def take_tui_screenshot(path: Optional[str] = None) -> Dict[str, Any]:
        """Take a screenshot of the TUI application
        
        Args:
            path: Optional screenshot file path
            
        Returns:
            Dictionary containing success status and screenshot path
        """
        controller = get_controller()
        
        try:
            screenshot_path = await controller.screenshot(path)
            return {
                "success": True,
                "screenshot_path": screenshot_path
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }