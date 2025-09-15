"""
Monodraw JSON parser for TUI window generation.
Parses Monodraw JSON files and extracts layer data for window creation.
"""

import json
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass


@dataclass
class MonodrawLayer:
    """Represents a layer from Monodraw JSON that can become a TUI window."""
    name: str
    origin: Tuple[int, int]  # (x, y) coordinates
    frame_size: Tuple[int, int]  # (width, height)
    text_content: str
    object_id: str


class MonodrawParser:
    """Parses Monodraw JSON files and extracts window-ready layer data."""
    
    @staticmethod
    def parse_file(file_path: str) -> List[MonodrawLayer]:
        """Parse Monodraw JSON file and extract named text layers."""
        with open(file_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        return MonodrawParser.extract_layers(data)
    
    @staticmethod
    def extract_layers(data: Dict) -> List[MonodrawLayer]:
        """Extract named text layers from Monodraw JSON data."""
        layers = []
        object_list = data.get('object_list', [])
        
        # Build object lookup by ID for cross-references
        objects_by_id = {obj['object_id']: obj for obj in object_list}
        
        # Find named text frame objects (type_id 20 appears to be text frames)
        for obj in object_list:
            if (obj.get('type_id') == 20 and 
                obj.get('name') and 
                obj.get('name_edited', False)):
                
                layer = MonodrawParser._extract_layer(obj, objects_by_id)
                if layer:
                    layers.append(layer)
        
        return layers
    
    @staticmethod
    def _extract_layer(frame_obj: Dict, objects_by_id: Dict) -> Optional[MonodrawLayer]:
        """Extract single layer data from frame object and its references."""
        name = frame_obj.get('name')
        origin_str = frame_obj.get('origin', '0,0')
        frame_size_str = frame_obj.get('frame_size', '10,5')
        model_text_id = frame_obj.get('model_text')
        
        if not all([name, model_text_id]):
            return None
        
        # Parse origin coordinates
        try:
            x, y = map(int, origin_str.split(','))
        except (ValueError, AttributeError):
            x, y = 0, 0
        
        # Get text content from referenced model_text object
        text_content = ""
        if model_text_id in objects_by_id:
            text_obj = objects_by_id[model_text_id]
            text_content = text_obj.get('text', '')
        
        # Calculate size based on actual text content dimensions
        if text_content:
            lines = text_content.split('\n')
            # Remove empty lines from the end
            while lines and not lines[-1].strip():
                lines.pop()
            
            text_height = len(lines) if lines else 1
            text_width = max(len(line) for line in lines) if lines else 10
            
            # Add small padding for readability
            w = text_width + 2
            h = text_height + 1
        else:
            # Fallback to Monodraw frame size if no text
            try:
                w, h = map(int, frame_size_str.split(','))
            except (ValueError, AttributeError):
                w, h = 20, 5
        
        return MonodrawLayer(
            name=name,
            origin=(x, y),
            frame_size=(w, h),
            text_content=text_content,
            object_id=frame_obj.get('object_id', '')
        )
    
    @staticmethod
    def get_canvas_bounds(layers: List[MonodrawLayer]) -> Tuple[int, int]:
        """Calculate overall canvas bounds from all layers."""
        if not layers:
            return 80, 24
        
        max_x = max(layer.origin[0] + layer.frame_size[0] for layer in layers)
        max_y = max(layer.origin[1] + layer.frame_size[1] for layer in layers)
        
        return max_x, max_y


def scale_coordinates(layers: List[MonodrawLayer], 
                     scale_factor: float = 1.0,
                     terminal_size: Tuple[int, int] = (80, 24)) -> List[MonodrawLayer]:
    """Scale layer coordinates to fit terminal size. NOTE: Only scales position, not window size."""
    if scale_factor == 1.0 and terminal_size == (80, 24):
        return layers  # No scaling needed
    
    # Calculate current canvas bounds
    canvas_w, canvas_h = MonodrawParser.get_canvas_bounds(layers)
    term_w, term_h = terminal_size
    
    # Auto-scale if needed to fit terminal
    if scale_factor == 1.0:
        if canvas_w > term_w or canvas_h > term_h:
            scale_factor = min(term_w / canvas_w, term_h / canvas_h) * 0.9
    
    # Apply scaling to positions only, keep text-based window sizes
    scaled_layers = []
    for layer in layers:
        scaled_x = int(layer.origin[0] * scale_factor)
        scaled_y = int(layer.origin[1] * scale_factor)
        
        # Keep original text-based window size - don't scale it!
        scaled_layer = MonodrawLayer(
            name=layer.name,
            origin=(scaled_x, scaled_y),
            frame_size=layer.frame_size,  # Keep original size
            text_content=layer.text_content,
            object_id=layer.object_id
        )
        scaled_layers.append(scaled_layer)
    
    return scaled_layers