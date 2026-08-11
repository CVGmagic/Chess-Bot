const board = document.getElementById('chessboard');

// Unicode Chess Piece Symbols
const PIECES = {
  'r': '♜', 'n': '♞', 'b': '♝', 'q': '♛', 'k': '♚', 'p': '♟',
  'R': '♖', 'N': '♘', 'B': '♗', 'Q': '♕', 'K': '♔', 'P': '♙',
  '': ''
};

// Initial 8x8 Board Array
let boardState = [
  ['r','n','b','q','k','b','n','r'],
  ['p','p','p','p','p','p','p','p'],
  ['','','','','','','',''],
  ['','','','','','','',''],
  ['','','','','','','',''],
  ['','','','','','','',''],
  ['P','P','P','P','P','P','P','P'],
  ['R','N','B','Q','K','B','N','R']
];

let selectedSquare = null;

let engineMovedSquares = [];

function renderBoard() {
    board.innerHTML = '';

    for (let r = 0; r < 8; r++) {
        for (let c = 0; c < 8; c++) {
            const square = document.createElement("div");

            const is_light = (r + c) % 2 === 0;
            square.className = `square ${is_light ? 'light' : 'dark'}`;
            
            if (selectedSquare && selectedSquare[0] === 7 - r && selectedSquare[1] === c) {
                square.classList.add('selected');
            }
            const isEngineSquare = engineMovedSquares.some(
                ([mRow, mCol]) => mRow === (7 - r) && mCol === c
            );
            if (isEngineSquare) {
                square.classList.add('engine-moved');
            }

            const pieceSymbol = boardState[7 - r][c];
            square.innerText = PIECES[pieceSymbol];

            square.addEventListener('click', () => handleSquareClick(7 - r, c));
            
            board.appendChild(square);
            
        }
    }
}

function handleSquareClick(r, c) {
    const clickedPiece = boardState[r][c];

    if (selectedSquare) {
        const [fromRow, fromCol] = selectedSquare;

        if (fromRow === r && fromCol === c) {
            selectedSquare = null;
        } else {
            let is_legal = checkLegal(fromRow, fromCol, r, c);

            if (is_legal) {
                boardState[r][c] = boardState[fromRow][fromCol];
                boardState[fromRow][fromCol] = '';

                makeHumanMove(fromRow, fromCol, r, c);
            }

            selectedSquare = null;
        }
    } else if (clickedPiece !== '') {
        selectedSquare = [r, c];
    }

    renderBoard();
}

function makeEngineMoveOnUI(from_row, from_col, to_row, to_col) {
    boardState[to_row][to_col] = boardState[from_row][from_col];
    boardState[from_row][from_col] = '';

    engineMovedSquares = [];

    engineMovedSquares.push([to_row, to_col]);
    engineMovedSquares.push([from_row, from_col]);

    renderBoard();
}


renderBoard();

