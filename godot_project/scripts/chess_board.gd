extends Control


@onready var piece_grid: GridContainer = %PieceGrid
@onready var piece_manager: Node2D = %PieceManager
@onready var engine_manager: Node = %EngineManager

var current_player: = 0 # White

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


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass


var default_square: Vector2i = Vector2i(-1, -1)
var prev_selected_square: Vector2i = default_square


func _on_square_clicked(rank, file) -> void:
	var selected_square: Vector2i = Vector2i(rank, file)
	if prev_selected_square == default_square or selected_square == prev_selected_square:
		prev_selected_square = selected_square
	
	else:
		make_move(prev_selected_square.x, prev_selected_square.y, selected_square.x, selected_square.y)
		prev_selected_square = default_square
	
		
func make_move(from_rank: int, from_file: int, to_rank: int, to_file: int) -> void:
	# TODO Add legality check
	piece_grid.move_piece(Vector2i(from_rank, from_file), Vector2i(to_rank, to_file))
	
	board[to_rank][to_file] = board[from_rank][from_file]
	board[from_rank][from_file] = 0
	
	current_player = 1 - current_player
	
	if current_player == 0:
		engine_manager.set_engine_board(board)
		engine_manager.make_random_engine_move()
