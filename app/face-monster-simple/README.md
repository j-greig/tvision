# Face Monster Mirror - Simplified Version

**tl;dr**: Webcam face tracking → replace face with ASCII art monster in real-time. Single face, single monster, heavily commented for easy porting to other languages.

## What it does

This app uses your webcam to detect faces and replaces them with a spooky ASCII art monster (bone-eye design) in real-time. It's a simplified, well-documented version of the main Face Monster Mirror project - perfect as a starting point for porting to other programming languages.

## Key Features

- **Single face tracking** using Google MediaPipe
- **One monster design** (monster-bone-eye.txt) 
- **Real-time rendering** on HTML5 canvas
- **Self-contained** - minimal dependencies
- **Extensively commented** - every file explains its purpose
- **TypeScript** with strict typing for clarity

## Quick Start

```bash
# Install dependencies
npm install

# Start development server
npm run dev

# Open browser to http://localhost:5173
# Grant camera permissions when prompted
```

## Architecture Overview

```
src/
├── main.ts          # App orchestration & camera setup
├── FaceDetector.ts  # MediaPipe face detection wrapper  
├── AsciiRenderer.ts # Canvas rendering for ASCII art
└── types.ts         # TypeScript interfaces
```

**Data Flow**: Camera → MediaPipe face detection → coordinate transformation → ASCII art rendering on canvas

## Dependencies

- **@mediapipe/tasks-vision**: Google's face detection AI
- **TypeScript + Vite**: Development tooling
- **monster.txt**: ASCII art file (46 lines of bone-eye monster)

## Porting Guide

This simplified version is designed for easy translation to other languages:

1. **FaceDetector.ts** - Replace MediaPipe with your language's face detection library
2. **AsciiRenderer.ts** - Replace HTML5 Canvas with your graphics/drawing library  
3. **main.ts** - Main loop coordinates between camera, detection, and rendering
4. **types.ts** - Data structures used throughout the app

## Browser Requirements

- **Camera access**: HTTPS or localhost required
- **WebRTC support**: Modern browsers only
- **Canvas 2D**: Hardware acceleration recommended

## File Outputs

- **face-monster-simple/README.md**