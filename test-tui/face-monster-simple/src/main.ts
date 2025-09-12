/**
 * main.ts - Face Monster Mirror Application Entry Point
 * 
 * This is the main application file that orchestrates the entire Face Monster Mirror system.
 * It coordinates between camera input, face detection, and ASCII art rendering to create
 * a real-time interactive experience where detected faces are replaced with monster ASCII art.
 * 
 * Application Flow:
 * 1. Initialize camera and get video stream
 * 2. Set up MediaPipe face detection system  
 * 3. Load ASCII art monster from file
 * 4. Start animation loop that continuously:
 *    - Detects faces in video stream
 *    - Renders ASCII monster at face positions
 *    - Updates display at ~30 FPS
 * 
 * This simplified version focuses on a single face with one monster for clarity
 * and ease of porting to other programming languages.
 */

import { FaceDetector } from './FaceDetector.js';
import { AsciiRenderer } from './AsciiRenderer.js';
import type { CameraInfo, FaceDetection } from './types.js';

/**
 * Main application class that manages the entire Face Monster Mirror system
 */
class FaceMonsterMirror {
  // Core system components
  private faceDetector: FaceDetector | null = null;
  private asciiRenderer: AsciiRenderer | null = null;
  
  // DOM element references
  private videoElement: HTMLVideoElement | null = null;
  private canvasElement: HTMLCanvasElement | null = null;
  private statusElement: HTMLElement | null = null;
  
  // Camera and video stream state
  private mediaStream: MediaStream | null = null;
  private cameraInfo: CameraInfo = {
    width: 0,
    height: 0,
    isActive: false
  };
  
  // ASCII art content (loaded from monster.txt file)
  private monsterAsciiArt: string = '';
  
  // Animation and timing control
  private animationFrameId: number | null = null;
  private isRunning: boolean = false;
  private lastFrameTime: number = 0;
  private targetFPS: number = 30;
  private frameInterval: number = 1000 / this.targetFPS; // ~33ms between frames

  /**
   * Initialize the entire application
   */
  constructor() {
    console.log('Face Monster Mirror - Simplified Version');
    console.log('Starting initialization...');
    
    this.initializeApplication();
  }

  /**
   * Main initialization sequence
   * 
   * This method coordinates the startup of all system components in the correct order.
   * Each step must complete successfully before proceeding to the next.
   */
  private async initializeApplication(): Promise<void> {
    try {
      // Step 1: Get DOM element references
      this.updateStatus('Getting DOM elements...');
      this.initializeDOMElements();
      
      // Step 2: Load ASCII art monster from file
      this.updateStatus('Loading monster ASCII art...');
      await this.loadMonsterArt();
      
      // Step 3: Initialize camera and video stream
      this.updateStatus('Requesting camera access...');
      await this.initializeCamera();
      
      // Step 4: Set up face detection system
      this.updateStatus('Initializing face detection...');
      await this.initializeFaceDetection();
      
      // Step 5: Set up ASCII art rendering
      this.updateStatus('Initializing ASCII renderer...');
      this.initializeRenderer();
      
      // Step 6: Start the main application loop
      this.updateStatus('Starting face monster mirror...');
      this.startApplication();
      
      this.updateStatus('Face Monster Mirror is running! Look at the camera.');
      
    } catch (error) {
      console.error('Initialization failed:', error);
      this.updateStatus(`Error: ${error instanceof Error ? error.message : 'Unknown initialization error'}`);
    }
  }

  /**
   * Get references to required DOM elements
   * 
   * We need the video element for camera input and canvas for ASCII art output.
   * The status element is used for user feedback during initialization.
   */
  private initializeDOMElements(): void {
    // Get video element for camera display
    this.videoElement = document.getElementById('video') as HTMLVideoElement;
    if (!this.videoElement) {
      throw new Error('Video element not found in DOM');
    }

    // Get canvas element for ASCII art rendering
    this.canvasElement = document.getElementById('canvas') as HTMLCanvasElement;
    if (!this.canvasElement) {
      throw new Error('Canvas element not found in DOM');
    }

    // Get status element for user feedback
    this.statusElement = document.getElementById('status-text');
    if (!this.statusElement) {
      console.warn('Status text element not found - status updates will only appear in console');
    }

    console.log('DOM elements initialized successfully');
  }

  /**
   * Load ASCII art monster from the monster.txt file
   * 
   * The monster art is stored in a separate file to make it easy to swap out
   * different monsters or edit the design without touching the code.
   */
  private async loadMonsterArt(): Promise<void> {
    try {
      // Fetch the monster ASCII art file
      const response = await fetch('/monster.txt');
      if (!response.ok) {
        throw new Error(`Failed to load monster art: ${response.status} ${response.statusText}`);
      }
      
      // Get the raw text content
      const monsterText = await response.text();
      if (!monsterText.trim()) {
        throw new Error('Monster art file is empty');
      }
      
      // Store the ASCII art for rendering
      // We extract only the ASCII art portion (everything after the ``` marker)
      const artStart = monsterText.indexOf('```');
      if (artStart !== -1) {
        this.monsterAsciiArt = monsterText.slice(artStart + 3).trim();
      } else {
        this.monsterAsciiArt = monsterText.trim();
      }
      
      console.log('Monster ASCII art loaded successfully');
      console.log(`Art has ${this.monsterAsciiArt.split('\n').length} lines`);
      
    } catch (error) {
      console.error('Failed to load monster art:', error);
      throw new Error(`Could not load monster ASCII art: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  /**
   * Initialize camera access and video stream
   * 
   * This method requests access to the user's camera and sets up the video stream.
   * Modern browsers require HTTPS or localhost for camera access.
   */
  private async initializeCamera(): Promise<void> {
    try {
      // Request camera access with optimal settings for face detection
      this.mediaStream = await navigator.mediaDevices.getUserMedia({
        video: {
          // Prefer higher resolution for better face detection accuracy
          width: { ideal: 1280, min: 640 },
          height: { ideal: 720, min: 480 },
          
          // Request higher frame rate for smoother detection
          frameRate: { ideal: 30, min: 15 },
          
          // Prefer front-facing camera for mirror effect
          facingMode: 'user'
        },
        // No audio needed for this application
        audio: false
      });

      // Connect media stream to video element
      if (!this.videoElement) {
        throw new Error('Video element not available');
      }
      
      this.videoElement.srcObject = this.mediaStream;
      
      // Wait for video metadata to load so we can get dimensions
      await new Promise<void>((resolve, reject) => {
        if (!this.videoElement) {
          reject(new Error('Video element not available'));
          return;
        }
        
        this.videoElement.onloadedmetadata = () => {
          // Update camera info with actual video dimensions
          this.cameraInfo = {
            width: this.videoElement!.videoWidth,
            height: this.videoElement!.videoHeight,
            isActive: true
          };
          
          console.log(`Camera initialized: ${this.cameraInfo.width}x${this.cameraInfo.height}px`);
          resolve();
        };
        
        this.videoElement.onerror = () => {
          reject(new Error('Failed to load video stream'));
        };
      });

    } catch (error) {
      console.error('Camera initialization failed:', error);
      throw new Error(`Camera access failed: ${error instanceof Error ? error.message : 'Unknown camera error'}`);
    }
  }

  /**
   * Initialize MediaPipe face detection system
   * 
   * This sets up the face detector with our video stream and optimal configuration
   * for single-face detection in a mirror application.
   */
  private async initializeFaceDetection(): Promise<void> {
    if (!this.videoElement) {
      throw new Error('Video element not initialized');
    }

    try {
      // Create and configure face detector
      this.faceDetector = new FaceDetector();
      
      // Initialize with optimized settings for single-face mirror use
      await this.faceDetector.initialize(this.videoElement, {
        maxFaces: 1,                      // Only detect one face for simplified version
        minDetectionConfidence: 0.6,      // Require good confidence to avoid false positives
        minPresenceConfidence: 0.6,       // Ensure face is clearly present
        minTrackingConfidence: 0.5        // Allow moderate tracking confidence for smooth motion
      });

      console.log('Face detection initialized successfully');
      
    } catch (error) {
      console.error('Face detection initialization failed:', error);
      throw new Error(`Face detection setup failed: ${error instanceof Error ? error.message : 'Unknown detection error'}`);
    }
  }

  /**
   * Initialize ASCII art renderer
   * 
   * This sets up the canvas-based rendering system that will draw the monster
   * ASCII art at the detected face positions.
   */
  private initializeRenderer(): void {
    if (!this.canvasElement) {
      throw new Error('Canvas element not initialized');
    }

    try {
      // Create ASCII renderer with our canvas
      this.asciiRenderer = new AsciiRenderer(this.canvasElement);
      
      // Configure renderer for optimal ASCII art display
      this.asciiRenderer.updateSettings({
        fontSize: 8,                    // Base font size for ASCII characters
        textColor: 'white',            // White text on black background for projection
        fontFamily: 'Courier New, monospace'  // Monospace font ensures consistent character spacing
      });

      console.log('ASCII renderer initialized successfully');
      
    } catch (error) {
      console.error('Renderer initialization failed:', error);
      throw new Error(`Renderer setup failed: ${error instanceof Error ? error.message : 'Unknown renderer error'}`);
    }
  }

  /**
   * Start the main application loop
   * 
   * This begins the real-time processing loop that continuously detects faces
   * and renders ASCII art. The loop runs at approximately 30 FPS.
   */
  private startApplication(): void {
    if (!this.faceDetector || !this.asciiRenderer) {
      throw new Error('Core components not initialized');
    }

    // Mark application as running
    this.isRunning = true;
    this.lastFrameTime = performance.now();

    // Start the animation loop
    this.animationLoop();
    
    console.log('Face Monster Mirror started successfully');
  }

  /**
   * Main animation loop - the heart of the real-time system
   * 
   * This method runs continuously while the application is active. Each frame:
   * 1. Gets current timestamp for MediaPipe
   * 2. Detects faces in the current video frame
   * 3. Renders ASCII monster at detected face positions
   * 4. Schedules the next frame
   * 
   * The loop is throttled to maintain consistent frame rate without overwhelming the system.
   */
  private animationLoop(): void {
    // Check if we should continue running
    if (!this.isRunning || !this.faceDetector || !this.asciiRenderer) {
      return;
    }

    const currentTime = performance.now();
    const deltaTime = currentTime - this.lastFrameTime;

    // Throttle frame rate to target FPS (30 FPS = ~33ms between frames)
    if (deltaTime >= this.frameInterval) {
      try {
        // Step 1: Detect faces in current video frame
        // MediaPipe uses timestamp to maintain tracking consistency
        const faces = this.faceDetector.detectFaces(currentTime);

        // Step 2: Render ASCII art for detected faces
        this.asciiRenderer.renderFrame(
          this.monsterAsciiArt,    // The monster ASCII art to display
          faces,                   // Array of detected faces (expect 0 or 1)
          this.cameraInfo         // Camera dimensions for coordinate transformation
        );

        // Optional: Log detection info for debugging (remove in production)
        if (faces.length > 0 && faces[0]) {
          const face = faces[0];
          console.log(`Face detected at (${face.bounds.x.toFixed(3)}, ${face.bounds.y.toFixed(3)}) confidence: ${face.confidence.toFixed(3)}`);
        }

        // Update frame timing
        this.lastFrameTime = currentTime;
        
      } catch (error) {
        console.error('Error in animation loop:', error);
        // Continue running despite errors to maintain stability
      }
    }

    // Schedule next frame
    this.animationFrameId = requestAnimationFrame(() => this.animationLoop());
  }

  /**
   * Update status message for user feedback
   * 
   * @param message - Status message to display
   */
  private updateStatus(message: string): void {
    console.log(`Status: ${message}`);
    
    if (this.statusElement) {
      this.statusElement.textContent = message;
    }
  }

  /**
   * Stop the application and clean up resources
   * 
   * This method should be called when the application is closing to properly
   * release camera, MediaPipe resources, and stop animation loops.
   */
  public stop(): void {
    console.log('Stopping Face Monster Mirror...');
    
    // Stop animation loop
    this.isRunning = false;
    if (this.animationFrameId) {
      cancelAnimationFrame(this.animationFrameId);
      this.animationFrameId = null;
    }

    // Stop camera stream
    if (this.mediaStream) {
      this.mediaStream.getTracks().forEach(track => track.stop());
      this.mediaStream = null;
    }

    // Clean up face detector
    if (this.faceDetector) {
      this.faceDetector.destroy();
      this.faceDetector = null;
    }

    // Clean up renderer
    if (this.asciiRenderer) {
      this.asciiRenderer.destroy();
      this.asciiRenderer = null;
    }

    // Update camera info
    this.cameraInfo.isActive = false;
    
    this.updateStatus('Face Monster Mirror stopped');
    console.log('Application stopped and resources cleaned up');
  }

  /**
   * Check if the application is currently running
   * 
   * @returns True if the application is active and processing frames
   */
  public isActive(): boolean {
    return this.isRunning && this.cameraInfo.isActive;
  }
}

/**
 * Application startup
 * 
 * Create and initialize the Face Monster Mirror when the page loads.
 * Also set up proper cleanup when the page is closed.
 */

// Wait for DOM to be fully loaded before starting
document.addEventListener('DOMContentLoaded', () => {
  console.log('DOM loaded, starting Face Monster Mirror...');
  
  // Create the main application instance
  const app = new FaceMonsterMirror();
  
  // Set up cleanup when page is closed/refreshed
  window.addEventListener('beforeunload', () => {
    console.log('Page unloading, cleaning up...');
    app.stop();
  });
  
  // Make app available globally for debugging (optional)
  (window as any).app = app;
  
  console.log('Face Monster Mirror initialization complete');
});