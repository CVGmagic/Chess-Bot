// Initialize the Web Worker thread pointing to engine_worker.js
const engineWorker = new Worker('engine/engine_worker.js');

let isEngineReady = false;

// Set up a listener for messages coming BACK from engine_worker.js
engineWorker.onmessage = function(event) {
  const { type, data } = event.data;

  // Handles the 'READY' signal sent by onRuntimeInitialized in engine_worker.js
  if (type === 'status' && data === 'READY') {
    console.log("Engine Wasm binary loaded and ready!");
    isEngineReady = true;

    // Send the initial UCI setup command
    sendUCICommand('uci');
  }

  // Handles standard output (cout) coming from C++ engine
  if (type === 'stdout') {
    console.log('[Engine Output]:', data);

    // Parse the bestmove command when search finishes
    if (data.startsWith('bestmove')) {
      const parts = data.split(' ');
      const bestMoveStr = parts[1]; // e.g., "e2e4"
      
      console.log('Bot played:', bestMoveStr);
      // Call frontend function to update the board state visually
      //makeEngineMoveOnUI(bestMoveStr);
    }
  }
};

// Helper function to send string commands TO the worker
function sendUCICommand(commandString) {
  if (!isEngineReady) {
    console.warn("Engine is not ready yet!");
    return;
  }
  
  // Sends the exact string to engine_worker.js's onmessage handler
  engineWorker.postMessage(commandString);
}

// Example usage when a human player finishes their turn:
function onHumanMoveFinished(currentFen) {
  // Tell engine to set position and search
  sendUCICommand(`position fen ${currentFen}`);
  sendUCICommand('go movetime 1000'); // Search for 1 second
}