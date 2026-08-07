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

function renderBoard() {
    board.innerHTML = '';

    for (let r = 0; r < 8; r++) {
        for (let c = 0; c < 8; c++) {
            const square = document.createElement("div");

            const is_light = (r + c) % 2 === 0;
            square.className = `square ${is_light ? 'light' : 'dark'}`;
            
            if (selectedSquare && selectedSquare[0] === r && selectedSquare[1] === c) {
                square.classList.add('selected');
            }

            const pieceSymbol = boardState[r][c];
            square.innerText = PIECES[pieceSymbol];

            square.addEventListener('click', () => handleSquareClick(r, c));
            
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
            boardState[r][c] = boardState[fromRow][fromCol];
            boardState[fromRow][fromCol] = '';
            selectedSquare = null;
        }
    } else if (clickedPiece !== '') {
        selectedSquare = [r, c];
    }

    renderBoard();
}

renderBoard();