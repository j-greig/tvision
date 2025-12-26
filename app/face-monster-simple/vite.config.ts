import { defineConfig } from 'vite';

export default defineConfig({
  server: {
    // Enable HTTPS for camera access during development
    // Note: Modern browsers require HTTPS (or localhost) for webcam access
    https: false, // Set to true if you need HTTPS in development
    port: 5173
  },
  build: {
    target: 'es2020',
    // Ensure ES modules are used for compatibility
    lib: undefined,
    rollupOptions: {
      // Keep external dependencies external in the bundle
      external: [],
    }
  }
});