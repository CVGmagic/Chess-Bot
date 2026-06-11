extends Control


@onready var piece_grid: GridContainer = %PieceGrid
@onready var piece_manager: Node2D = %PieceManager
@onready var engine_manager: Node = %EngineManager
@onready var ui_manager: Node = %UIManager

const FLAG_KING_CASTLE = 2
const FLAG_QUEEN_CASTLE = 3
const FLAG_EN_PASSANT = 5
const FLAG_PROMOTION = 8

var current_turn: int = 0 # White
var player_color: int = 1

var board: Array[Array] = [[4, 2, 3, 5, 6, 3, 2, 4],
							[1, 1, 1, 1, 1, 1, 1, 1],
							[0, 0, 0, 0, 0, 0, 0, 0],
							[0, 0, 0, 0, 0, 0, 0, 0],
							[0, 0, 0, 0, 0, 0, 0, 0],
							[0, 0, 0, 0, 0, 0, 0, 0],
							[7, 7, 7, 7, 7, 7, 7, 7],
							[10, 8, 9, 11, 12, 9, 8, 10]]

# Coordinates correspond to (rank, file)

func _ready() -> void:
	piece_grid.generate_pieces(board, piece_manager.piece_map)



var default_square: Vector2i = Vector2i(-1, -1)
var prev_selected_square: Vector2i = default_square


func _on_square_clicked(rank, file) -> void:
	var selected_square: Vector2i
	if ui_manager.board_is_flipped:
		selected_square = Vector2i(7 - rank, 7 - file)
	else:
		selected_square = Vector2i(rank, file)
	
	# Cancel selected square
	if selected_square == prev_selected_square:
		prev_selected_square = default_square
		return
	
	# Select first square
	if prev_selected_square == default_square:
		prev_selected_square = selected_square
		return
	
	# Select second square
	make_move(prev_selected_square.x, prev_selected_square.y, selected_square.x, selected_square.y, 0)
	prev_selected_square = default_square


func make_move(from_rank: int, from_file: int, to_rank: int, to_file: int, flag: int) -> void:
	if current_turn != player_color: # Engine's turn
		update_piece(Vector2i(from_rank, from_file), Vector2i(to_rank, to_file))
		
		if current_turn == 0: # White
			if flag == FLAG_KING_CASTLE:
				update_piece(Vector2i(0, 7), Vector2i(0, 5))
			elif flag == FLAG_QUEEN_CASTLE:
				update_piece(Vector2i(0, 0), Vector2i(0, 3))
			elif flag == FLAG_EN_PASSANT:
				piece_grid.pieces[to_rank - 1][to_file].queue_free()
				board[to_rank - 1][to_file] = 0
			elif flag >= 8: # Promotion
				if flag == 0x8 or flag == 0xC: # Knight
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Knight White.svg")
				elif flag == 0x9 or flag == 0xD: # Bishop
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Bishop White.svg")
				elif flag == 0xA or flag == 0xE: # Rook
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Rook White.svg")
				elif flag == 0xB or flag == 0xF: # Queen
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Queen White.svg")
				else:
					printerr("Invalid Promotion Type encountered")
		else: # Black
			if flag == FLAG_KING_CASTLE:
				update_piece(Vector2i(7, 7), Vector2i(7, 5))
			elif flag == FLAG_QUEEN_CASTLE:
				update_piece(Vector2i(7, 0), Vector2i(7, 3))
			elif flag == FLAG_EN_PASSANT:
				piece_grid.pieces[to_rank + 1][to_file].queue_free()
				piece_grid.pieces[to_rank + 1][to_file] = null
				board[to_rank + 1][to_file] = 0
			elif flag >= 8: # Promotion
				if flag == 0x8 or flag == 0xC: # Knight
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Knight Black.svg")
				elif flag == 0x9 or flag == 0xD: # Bishop
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Bishop Black.svg")
				elif flag == 0xA or flag == 0xE: # Rook
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Rook Black.svg")
				elif flag == 0xB or flag == 0xF: # Queen
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Queen Black.svg")
				else:
					printerr("Invalid Promotion Type encountered")
		
		current_turn = 1 - current_turn
		return
	
	else: # Players turn
		var data: int = engine_manager.try_move(from_rank, from_file, to_rank, to_file)
		
		if data == 0:
			return

		flag = (data >> 12) # Redefine the parameter 'flag'

		update_piece(Vector2i(from_rank, from_file), Vector2i(to_rank, to_file))
		
		if current_turn == 0: # White
			if flag == FLAG_KING_CASTLE:
				update_piece(Vector2i(0, 7), Vector2i(0, 5))
			elif flag == FLAG_QUEEN_CASTLE:
				update_piece(Vector2i(0, 0), Vector2i(0, 3))
			elif flag == FLAG_EN_PASSANT:
				piece_grid.pieces[to_rank - 1][to_file].queue_free()
				board[to_rank - 1][to_file] = 0
			elif flag >= 8: # Promotion
				if flag == 0x8 or flag == 0xC: # Knight
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Knight White.svg")
				elif flag == 0x9 or flag == 0xD: # Bishop
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Bishop White.svg")
				elif flag == 0xA or flag == 0xE: # Rook
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Rook White.svg")
				elif flag == 0xB or flag == 0xF: # Queen
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Queen White.svg")
				else:
					printerr("Invalid Promotion Type encountered")
		else: # Black
			if flag == FLAG_KING_CASTLE:
				update_piece(Vector2i(7, 7), Vector2i(7, 5))
			elif flag == FLAG_QUEEN_CASTLE:
				update_piece(Vector2i(7, 0), Vector2i(7, 3))
			elif flag == FLAG_EN_PASSANT:
				piece_grid.pieces[to_rank + 1][to_file].queue_free()
				piece_grid.pieces[to_rank + 1][to_file] = null
				board[to_rank + 1][to_file] = 0
			elif flag >= 8: # Promotion
				if flag == 0x8 or flag == 0xC: # Knight
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Knight Black.svg")
				elif flag == 0x9 or flag == 0xD: # Bishop
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Bishop Black.svg")
				elif flag == 0xA or flag == 0xE: # Rook
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Rook Black.svg")
				elif flag == 0xB or flag == 0xF: # Queen
					piece_grid.pieces[to_rank][to_file].texture = load("res://assets/pieces/Queen Black.svg")
				else:
					printerr("Invalid Promotion Type encountered")
		
		current_turn = 1 - current_turn
		
		engine_manager.make_best_engine_move()


func update_piece(from_sq: Vector2i, to_sq: Vector2i) -> void:
	piece_grid.move_piece(Vector2i(from_sq.x, from_sq.y), Vector2i(to_sq.x, to_sq.y))
	
	board[to_sq.x][to_sq.y] = board[from_sq.x][from_sq.y]
	board[from_sq.x][from_sq.y] = 0
	return
