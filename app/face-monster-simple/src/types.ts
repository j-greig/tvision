/**
 * Type definitions for the simplified Face Monster Mirror application
 * 
 * This file contains all the TypeScript interfaces and types used throughout
 * the application. Keeping types in a separate file makes the code more
 * maintainable and easier to understand when porting to other languages.
 */

/**
 * Represents a detected face from MediaPipe
 * 
 * MediaPipe returns face detection results with landmarks (facial feature points)
 * and a bounding box around the face. We also include a confidence score
 * to filter out low-quality detections.
 */
export interface FaceDetection {
  /** 
   * Array of 478 facial landmark points from MediaPipe
   * Each landmark has x, y, z coordinates (normalized 0-1)
   * We primarily use these to calculate the face bounding box
   */
  landmarks: Array<{ x: number; y: number; z?: number }>;
  
  /**
   * Bounding box around the detected face
   * Coordinates are normalized (0-1) relative to video dimensions
   */
  bounds: {
    x: number;      // Left edge of face (0-1)
    y: number;      // Top edge of face (0-1) 
    width: number;  // Width of face box (0-1)
    height: number; // Height of face box (0-1)
  };
  
  /**
   * Confidence score for this face detection (0-1)
   * Higher values indicate more reliable face detection
   * We typically filter out faces with confidence < 0.5
   */
  confidence: number;
}

/**
 * Position where ASCII art should be rendered on the canvas
 * 
 * Unlike FaceDetection coordinates which are normalized (0-1),
 * these are absolute pixel coordinates for canvas rendering.
 */
export interface RenderPosition {
  /** X coordinate in pixels from left edge of canvas */
  x: number;
  
  /** Y coordinate in pixels from top edge of canvas */
  y: number;
  
  /** Width in pixels for the ASCII art area */
  width: number;
  
  /** Height in pixels for the ASCII art area */  
  height: number;
}

/**
 * Configuration options for the face detection system
 * 
 * These settings control MediaPipe behavior and can be adjusted
 * to optimize performance vs accuracy for different use cases.
 */
export interface DetectionConfig {
  /**
   * Maximum number of faces to detect simultaneously
   * Set to 1 for this simplified version
   */
  maxFaces: number;
  
  /**
   * Minimum confidence score to accept a face detection (0-1)
   * Higher values = fewer false positives, but may miss some valid faces
   */
  minDetectionConfidence: number;
  
  /**
   * Minimum confidence for face presence in consecutive frames (0-1)
   * Helps with tracking stability across frames
   */
  minPresenceConfidence: number;
  
  /**
   * Minimum confidence for face tracking between frames (0-1)
   * Higher values provide more stable tracking but may lose faces during movement
   */
  minTrackingConfidence: number;
}

/**
 * Video camera dimensions and properties
 * 
 * Used for coordinate transformation from camera space to canvas space
 */
export interface CameraInfo {
  /** Video width in pixels */
  width: number;
  
  /** Video height in pixels */
  height: number;
  
  /** Whether the camera is currently active and streaming */
  isActive: boolean;
}

/**
 * Canvas rendering dimensions and context
 * 
 * Tracks the canvas state for ASCII art rendering
 */
export interface CanvasInfo {
  /** Canvas width in pixels */
  width: number;
  
  /** Canvas height in pixels */ 
  height: number;
  
  /** 2D rendering context for drawing operations */
  context: CanvasRenderingContext2D;
}