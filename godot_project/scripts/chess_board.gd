extends Control


@onready var piece_grid: GridContainer = %PieceGrid
@onready var piece_manager: Node2D = %PieceManager

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


func _on_square_clicked(rank, file):
	var selected_square: Vector2i = Vector2i(rank, file)
	if prev_selected_square == default_square or selected_square == prev_selected_square:
		prev_selected_square = selected_square
	
	else:
		# TODO Add legality check
		piece_grid.move_piece(prev_selected_square, selected_square)
		
		board[selected_square.x][selected_square.y] = board[prev_selected_square.x][prev_selected_square.y]
		board[prev_selected_square.x][prev_selected_square.y] = null
		
		
		prev_selected_square = default_square
	
		
