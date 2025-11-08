/**
 * FaceDetector.ts - MediaPipe Face Detection Integration
 * 
 * This class wraps Google's MediaPipe Face Landmarker for real-time face detection.
 * MediaPipe is a framework for building multimodal applied ML pipelines and provides
 * robust face detection with 478 facial landmark points per detected face.
 * 
 * Key concepts:
 * - Uses WebAssembly (WASM) for fast execution in the browser
 * - GPU acceleration when available for better performance  
 * - Returns normalized coordinates (0-1) that need to be scaled to canvas pixels
 * - Designed for video streams with temporal consistency between frames
 */

import { FaceLandmarker, FilesetResolver } from '@mediapipe/tasks-vision';
import type { FaceDetection, DetectionConfig } from './types.js';

export class FaceDetector {
  // MediaPipe face detection instance - null until initialized
  private faceLandmarker: FaceLandmarker | null = null;
  
  // Reference to the HTML video element containing camera feed
  private videoElement: HTMLVideoElement | null = null;
  
  // Tracks whether MediaPipe has been successfully loaded and configured
  private isInitialized: boolean = false;
  
  // Detection configuration with sensible defaults for a single-face application
  private config: DetectionConfig = {
    maxFaces: 1,                    // Only detect one face for simplified version
    minDetectionConfidence: 0.6,    // Require decent confidence to avoid false positives
    minPresenceConfidence: 0.6,     // Require face to be clearly present in frame
    minTrackingConfidence: 0.5      // Allow moderate confidence for smooth tracking
  };

  /**
   * Initialize MediaPipe Face Landmarker with the camera video stream
   * 
   * This is an async operation because:
   * 1. MediaPipe WASM files must be downloaded from CDN
   * 2. GPU delegation setup takes time
   * 3. Model weights need to be loaded
   * 
   * @param videoElement - HTML video element showing camera feed
   * @param config - Optional configuration to override defaults
   */
  async initialize(videoElement: HTMLVideoElement, config?: Partial<DetectionConfig>): Promise<void> {
    // Prevent double initialization
    if (this.isInitialized) {
      console.log('FaceDetector already initialized');
      return;
    }

    // Store video element reference for detection calls
    this.videoElement = videoElement;
    
    // Merge custom config with defaults
    if (config) {
      this.config = { ...this.config, ...config };
    }

    try {
      console.log('Loading MediaPipe WASM files...');
      
      // Step 1: Load MediaPipe vision tasks WASM runtime from CDN
      // This downloads and initializes the WebAssembly runtime needed for face detection
      const vision = await FilesetResolver.forVisionTasks(
        "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@latest/wasm"
      );

      console.log('Creating Face Landmarker with GPU acceleration...');
      
      // Step 2: Create Face Landmarker instance with our configuration
      this.faceLandmarker = await FaceLandmarker.createFromOptions(vision, {
        baseOptions: {
          // Pre-trained model for face landmark detection (468 points per face)
          modelAssetPath: "https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task",
          
          // Use GPU acceleration if available, falls back to CPU automatically
          delegate: "GPU"
        },
        
        // Configure for video stream processing (vs single image)
        runningMode: "VIDEO",
        
        // Apply our detection thresholds
        numFaces: this.config.maxFaces,
        minFaceDetectionConfidence: this.config.minDetectionConfidence,
        minFacePresenceConfidence: this.config.minPresenceConfidence,
        minTrackingConfidence: this.config.minTrackingConfidence
      });

      this.isInitialized = true;
      console.log('Face detector initialized successfully');
      
    } catch (error) {
      console.error('Failed to initialize face detector:', error);
      throw new Error(`Face detector initialization failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  /**
   * Detect faces in the current video frame
   * 
   * This method should be called repeatedly (typically 30-60 FPS) to get
   * real-time face detection. MediaPipe uses the timestamp to maintain
   * tracking consistency between frames.
   * 
   * @param timestamp - Current time in milliseconds (from performance.now())
   * @returns Array of detected faces (empty if no faces found)
   */
  detectFaces(timestamp: number): FaceDetection[] {
    // Ensure we're properly initialized before attempting detection
    if (!this.faceLandmarker || !this.videoElement || !this.isInitialized) {
      return [];
    }

    try {
      // Run MediaPipe face detection on the current video frame
      // The timestamp helps MediaPipe maintain tracking between frames
      const results = this.faceLandmarker.detectForVideo(this.videoElement, timestamp);

      // Check if any faces were detected
      if (!results.faceLandmarks || results.faceLandmarks.length === 0) {
        return [];
      }

      // Convert MediaPipe results to our FaceDetection interface
      // We process each detected face and calculate a bounding box from landmarks
      return results.faceLandmarks.map((landmarks, index) => {
        // Calculate face bounding box from the 468 landmark points
        const bounds = this.calculateFaceBounds(landmarks);
        
        // Get confidence score if available, default to 0.7 for successful detections
        const confidence = results.faceBlendshapes?.[index]?.categories?.[0]?.score || 0.7;

        return {
          landmarks,
          bounds,
          confidence
        };
      });

    } catch (error) {
      // Log errors but don't crash - detection errors are common during initialization
      console.error('Face detection error:', error);
      return [];
    }
  }

  /**
   * Calculate face bounding box from MediaPipe landmarks
   * 
   * MediaPipe returns 468 landmark points for each face, covering features like
   * eyes, nose, mouth, and face contour. We find the minimum and maximum X/Y
   * coordinates to create a bounding box around the entire face.
   * 
   * The coordinates are normalized (0-1) relative to the video dimensions.
   * 
   * @param landmarks - Array of facial landmark points from MediaPipe
   * @returns Bounding box with normalized coordinates
   */
  private calculateFaceBounds(landmarks: Array<{ x: number; y: number; z?: number }>): {
    x: number; y: number; width: number; height: number;
  } {
    // Handle edge case of empty landmarks array
    if (!landmarks || landmarks.length === 0) {
      return { x: 0, y: 0, width: 0, height: 0 };
    }

    // Extract all X and Y coordinates from landmarks
    const xCoordinates = landmarks.map(landmark => landmark.x);
    const yCoordinates = landmarks.map(landmark => landmark.y);

    // Find the extreme points to create bounding box
    const minX = Math.min(...xCoordinates);
    const maxX = Math.max(...xCoordinates);
    const minY = Math.min(...yCoordinates);
    const maxY = Math.max(...yCoordinates);

    // Return bounding box with normalized coordinates (0-1)
    return {
      x: minX,                    // Left edge of face
      y: minY,                    // Top edge of face  
      width: maxX - minX,         // Face width
      height: maxY - minY         // Face height
    };
  }

  /**
   * Update detection configuration without reinitializing MediaPipe
   * 
   * Note: Some configuration changes may require reinitialization.
   * This method only updates our local config object.
   * 
   * @param newConfig - Partial configuration to merge with existing settings
   */
  updateConfig(newConfig: Partial<DetectionConfig>): void {
    this.config = { ...this.config, ...newConfig };
    console.log('Face detector config updated:', this.config);
  }

  /**
   * Get current detection configuration
   * 
   * @returns Current configuration object
   */
  getConfig(): DetectionConfig {
    return { ...this.config };
  }

  /**
   * Check if the face detector is ready for use
   * 
   * @returns True if initialized and ready to detect faces
   */
  isReady(): boolean {
    return this.isInitialized && this.faceLandmarker !== null && this.videoElement !== null;
  }

  /**
   * Clean up resources when the face detector is no longer needed
   * 
   * Important: MediaPipe instances should be properly disposed to free
   * GPU memory and prevent memory leaks. Always call this when shutting down.
   */
  destroy(): void {
    console.log('Destroying face detector...');
    
    // Close MediaPipe instance and free resources
    if (this.faceLandmarker) {
      this.faceLandmarker.close();
    }

    // Reset all state
    this.faceLandmarker = null;
    this.videoElement = null;
    this.isInitialized = false;
    
    console.log('Face detector destroyed');
  }
}