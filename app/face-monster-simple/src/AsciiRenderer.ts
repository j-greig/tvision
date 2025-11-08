/**
 * AsciiRenderer.ts - Canvas-based ASCII Art Rendering System
 * 
 * This class handles all ASCII art rendering on an HTML5 canvas. It takes care of:
 * - Converting face detection coordinates to canvas pixels
 * - Scaling ASCII art to match face size
 * - Positioning text correctly within detected face bounds
 * - Optimizing rendering performance for real-time use
 * 
 * Key concepts:
 * - HTML5 Canvas 2D context for hardware-accelerated drawing
 * - Monospace fonts ensure consistent character spacing for ASCII art
 * - Text positioning calculations to center art within face bounds
 * - Coordinate system transformation from normalized to pixel coordinates
 */

import type { FaceDetection, RenderPosition, CameraInfo, CanvasInfo } from './types.js';

export class AsciiRenderer {
  // Canvas and rendering context
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;
  
  // Font settings optimized for ASCII art display
  private fontSize: number = 8;           // Base font size in pixels
  private fontFamily: string = 'Courier New, monospace';  // Monospace for consistent spacing
  private textColor: string = 'white';    // Default text color
  
  // Canvas dimensions (updated when window resizes)
  private canvasWidth: number = 0;
  private canvasHeight: number = 0;

  /**
   * Initialize the ASCII renderer with a canvas element
   * 
   * @param canvas - HTML canvas element where ASCII art will be drawn
   */
  constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
    
    // Get 2D rendering context - this is where all drawing operations happen
    const context = canvas.getContext('2d');
    if (!context) {
      throw new Error('Cannot get 2D rendering context from canvas');
    }
    this.ctx = context;

    // Set up the canvas with proper dimensions and styling
    this.setupCanvas();
    
    // Listen for window resize events to keep canvas properly sized
    window.addEventListener('resize', () => this.setupCanvas());
  }

  /**
   * Configure canvas dimensions and rendering properties
   * 
   * This method sets up the canvas to fill the entire browser window
   * for an immersive projection/mirror experience.
   */
  private setupCanvas(): void {
    // Set canvas to match window dimensions for full-screen display
    this.canvasWidth = window.innerWidth;
    this.canvasHeight = window.innerHeight;
    this.canvas.width = this.canvasWidth;
    this.canvas.height = this.canvasHeight;

    // Configure text rendering properties
    this.ctx.font = `${this.fontSize}px ${this.fontFamily}`;
    this.ctx.textAlign = 'center';          // Center text horizontally
    this.ctx.textBaseline = 'middle';       // Center text vertically
    this.ctx.fillStyle = this.textColor;

    // Fill canvas with black background (ideal for projection)
    this.clearCanvas();

    console.log(`Canvas configured: ${this.canvasWidth}x${this.canvasHeight}px`);
  }

  /**
   * Clear the entire canvas with a black background
   * 
   * Called before each frame to remove previous ASCII art
   */
  private clearCanvas(): void {
    this.ctx.fillStyle = 'black';
    this.ctx.fillRect(0, 0, this.canvasWidth, this.canvasHeight);
  }

  /**
   * Transform normalized face detection coordinates to canvas pixel coordinates
   * 
   * MediaPipe returns face coordinates normalized to 0-1 range relative to video dimensions.
   * We need to convert these to absolute pixel positions on our canvas.
   * 
   * @param face - Face detection with normalized coordinates
   * @param cameraInfo - Camera video dimensions for scaling
   * @returns Pixel coordinates for canvas rendering
   */
  private transformToCanvasCoordinates(face: FaceDetection, cameraInfo: CameraInfo): RenderPosition {
    // Convert normalized coordinates (0-1) to camera pixels
    const faceXPixels = face.bounds.x * cameraInfo.width;
    const faceYPixels = face.bounds.y * cameraInfo.height;
    const faceWidthPixels = face.bounds.width * cameraInfo.width;
    const faceHeightPixels = face.bounds.height * cameraInfo.height;

    // Scale from camera dimensions to canvas dimensions
    const scaleX = this.canvasWidth / cameraInfo.width;
    const scaleY = this.canvasHeight / cameraInfo.height;

    // Apply scaling to get final canvas coordinates
    return {
      x: faceXPixels * scaleX,
      y: faceYPixels * scaleY,
      width: faceWidthPixels * scaleX,
      height: faceHeightPixels * scaleY
    };
  }

  /**
   * Calculate optimal font scaling for ASCII art based on face size
   * 
   * Larger faces should have larger ASCII art for proper visual impact.
   * We scale the font size proportionally to the detected face dimensions.
   * 
   * @param faceWidth - Width of face in canvas pixels
   * @param faceHeight - Height of face in canvas pixels
   * @returns Scaling factor to apply to base font size
   */
  private calculateOptimalScale(faceWidth: number, faceHeight: number): number {
    // Calculate face size relative to canvas
    const widthRatio = faceWidth / this.canvasWidth;
    const heightRatio = faceHeight / this.canvasHeight;
    const faceSize = Math.max(widthRatio, heightRatio);

    // Base scaling calculation
    // Multiply by 8 to make ASCII art prominent and visible
    const baseScale = faceSize * 8;

    // Apply reasonable bounds to prevent tiny or huge ASCII art
    const minScale = 0.5;  // Minimum scale to keep text readable
    const maxScale = 4.0;  // Maximum scale to prevent overflow
    
    return Math.max(minScale, Math.min(maxScale, baseScale));
  }

  /**
   * Render ASCII art at the position of a detected face
   * 
   * This is the main rendering method that takes a face detection and ASCII art
   * string and draws it on the canvas at the correct position and scale.
   * 
   * @param asciiArt - Multi-line ASCII art string to render
   * @param face - Face detection data with position information
   * @param cameraInfo - Camera dimensions for coordinate transformation
   */
  renderFaceWithAscii(asciiArt: string, face: FaceDetection, cameraInfo: CameraInfo): void {
    // Transform normalized face coordinates to canvas pixels
    const position = this.transformToCanvasCoordinates(face, cameraInfo);
    
    // Calculate optimal scaling for this face size
    const scale = this.calculateOptimalScale(position.width, position.height);
    
    // Update font size based on calculated scale
    const scaledFontSize = this.fontSize * scale;
    this.ctx.font = `${scaledFontSize}px ${this.fontFamily}`;
    this.ctx.fillStyle = this.textColor;

    // Split ASCII art into individual lines for rendering
    const lines = asciiArt.split('\n').filter(line => line.trim().length > 0);
    if (lines.length === 0) {
      return; // Nothing to render
    }

    // Calculate vertical spacing between lines
    const lineHeight = scaledFontSize * 1.2; // 20% extra space between lines
    const totalTextHeight = lines.length * lineHeight;

    // Calculate starting position to center text within face bounds
    const centerX = position.x + (position.width / 2);
    const centerY = position.y + (position.height / 2);
    const startY = centerY - (totalTextHeight / 2);

    // Render each line of ASCII art
    lines.forEach((line, lineIndex) => {
      const lineY = startY + (lineIndex * lineHeight);
      
      // Only render if the line would be visible on canvas
      if (lineY >= -lineHeight && lineY <= this.canvasHeight + lineHeight) {
        this.ctx.fillText(line, centerX, lineY);
      }
    });

    // Debug: Log rendering info (remove in production)
    console.log(`Rendered ASCII art at position (${Math.round(centerX)}, ${Math.round(centerY)}) with scale ${scale.toFixed(2)}`);
  }

  /**
   * Render a complete frame with face detection and ASCII art
   * 
   * This method coordinates the entire rendering process:
   * 1. Clear the previous frame
   * 2. Render ASCII art for each detected face
   * 3. Handle the case where no faces are detected
   * 
   * @param asciiArt - ASCII art string to display
   * @param faces - Array of detected faces (we expect only one for simplified version)
   * @param cameraInfo - Camera stream information
   */
  renderFrame(asciiArt: string, faces: FaceDetection[], cameraInfo: CameraInfo): void {
    // Clear previous frame
    this.clearCanvas();

    // Render ASCII art for detected faces
    if (faces.length > 0 && cameraInfo.isActive) {
      // In simplified version, we only process the first detected face
      const primaryFace = faces[0];
      
      // Only render if face exists and has sufficient confidence
      if (primaryFace && primaryFace.confidence >= 0.5) {
        this.renderFaceWithAscii(asciiArt, primaryFace, cameraInfo);
      }
    }
    
    // Optional: Could add "no face detected" message here for debugging
    // In production exhibition, usually better to show nothing when no face detected
  }

  /**
   * Update rendering settings
   * 
   * @param settings - New rendering configuration
   */
  updateSettings(settings: {
    fontSize?: number;
    textColor?: string;
    fontFamily?: string;
  }): void {
    if (settings.fontSize !== undefined) {
      this.fontSize = settings.fontSize;
    }
    if (settings.textColor !== undefined) {
      this.textColor = settings.textColor;
    }
    if (settings.fontFamily !== undefined) {
      this.fontFamily = settings.fontFamily;
    }
    
    // Update canvas with new settings
    this.setupCanvas();
  }

  /**
   * Get current canvas information
   * 
   * @returns Current canvas state and dimensions
   */
  getCanvasInfo(): CanvasInfo {
    return {
      width: this.canvasWidth,
      height: this.canvasHeight,
      context: this.ctx
    };
  }

  /**
   * Clean up resources when renderer is no longer needed
   */
  destroy(): void {
    // Remove event listeners to prevent memory leaks
    window.removeEventListener('resize', this.setupCanvas);
    
    console.log('ASCII renderer destroyed');
  }
}