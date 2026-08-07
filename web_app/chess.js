// Initialize the Web Worker thread pointing to engine_worker.js
const engineWorker = new Worker('engine/engine_worker.js');

let isEngineReady = false;
let move_history = [];

let receiving_legal_moves = false;

let current_legal_moves = [];


function coordsToAlgebraic(row, col, topToBottom = false) {
  // 1. Convert column (0..7) to file ('a'..'h')
  // String.fromCharCode(97) is 'a'
  const file = String.fromCharCode(97 + col);

  // 2. Convert row (0..7) to rank ('1'..'8')
  // If row 0 is rank 8 (top-left indexed, like standard 2D arrays or FEN):
  const rank = topToBottom ? 8 - row : row + 1;

  return `${file}${rank}`;
}

function algebraicToCoords(square, topToBottom = false) {
  if (!square || square.length < 2) return null;

  const file = square[0].toLowerCase();
  const rank = parseInt(square[1], 10);

  // Validate file ('a'-'h') and rank (1-8)
  if (file < 'a' || file > 'h' || isNaN(rank) || rank < 1 || rank > 8) {
    return null;
  }

  // 1. Convert file ('a'..'h') to column index (0..7)
  // 'a'.charCodeAt(0) is 97
  const col = file.charCodeAt(0) - 97;

  // 2. Convert rank (1..8) to row index (0..7)
  const row = topToBottom ? 8 - rank : rank - 1;

  return [row, col];
}


// Set up a listener for messages coming BACK from engine_worker.js
engineWorker.onmessage = function(event) {
  const { type, data } = event.data;

  // Handles the 'READY' signal sent by onRuntimeInitialized in engine_worker.js
  if (type === 'status' && data === 'READY') {
    console.log("Engine Wasm binary loaded and ready!");
    isEngineReady = true;

    // Send the initial UCI setup command
    sendUCICommand('uci');
    sendUCICommand("position startpos");
    sendUCICommand("getlegal");
  }

  // Handles standard output (cout) coming from C++ engine
  if (type === 'stdout') {
    console.log('[Engine Output]:', data);

    // Parse the bestmove command when search finishes
    if (data.startsWith('bestmove')) {
      const parts = data.split(' ');
      const bestMoveStr = parts[1]; // e.g., "e2e4"
      
      console.log('Bot played:', bestMoveStr);
      move_history.push(bestMoveStr);
      updateEnginePosition();
      sendUCICommand("getlegal");

      let [from_row, from_col] = algebraicToCoords(bestMoveStr.slice(0, 2));
      let [to_row, to_col] = algebraicToCoords(bestMoveStr.slice(2, 4));
      
      makeEngineMoveOnUI(from_row, from_col, to_row, to_col);
    }
    else if (data === "legalmoves") {
        receiving_legal_moves = true;
        current_legal_moves = [];
    }
    else if (receiving_legal_moves) {
        const string_arr = data.split(" ");
        current_legal_moves.push(string_arr.map(Number));
    }
    else if (data === "endlegalmoves") {
        receiving_legal_moves = false;
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

function checkLegal(from_row, from_col, to_row, to_col) {
    let from_sq = from_row * 8 + from_col;
    let to_sq = to_row * 8 + to_col;

    for (const [legal_from, legal_to, flag] of current_legal_moves) {
        if (legal_from === from_sq && legal_to === to_sq) {
            return true;
        }
    }
    return false;
}

function makeHumanMove(from_row, from_col, to_row, to_col) {
    move_history.push(coordsToAlgebraic(from_row, from_col) + coordsToAlgebraic(to_row, to_col));

    getEngineMove();
}

function updateEnginePosition() {
    let cmd = "position startpos moves";
    for (const move of move_history) {
        cmd += " " + move;
    }

    sendUCICommand(cmd);
}

function getEngineMove() {
    updateEnginePosition();

    sendUCICommand("go movetime 100");
}
