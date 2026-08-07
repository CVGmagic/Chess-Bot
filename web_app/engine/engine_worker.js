Module = {
  // Capture std::cout output from C++ engine and forward it to script.js
  print: function(text) {
    postMessage({ type: 'stdout', data: text });
  },
  
  // Capture std::cerr errors (if any)
  printErr: function(text) {
    console.error('Wasm Engine Error:', text);
  },

  // Called automatically when the WebAssembly binary finishes loading
  onRuntimeInitialized: function() {
    postMessage({ type: 'status', data: 'READY' });
  }
};

// Imports my_chess_engine_wasm.js
importScripts('my_chess_engine_wasm.js');

// Listen for string commands sent from script.js (e.g., "position startpos", "go depth 6")
onmessage = function(e) {
  const command = e.data;

  // Use Emscripten's ccall to run script/pass stdin strings if engine is running a standard UCI loop
  if (typeof Module.ccall === 'function') {
    // Alternatively, if your uci_interface reads line-by-line via stdin:
    // Emscripten provides standard stream handling or custom exported C functions.
    Module.ccall("send_uci_command", null, ['string'], [command]);
  }
};
