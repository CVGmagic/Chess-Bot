extends Control


@onready var piece_grid: GridContainer = %PieceGrid

func _ready() -> void:
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass


var default_square: Vector2i = Vector2i(-1, -1)
var prev_selected_square: Vector2i = default_square


func _on_square_clicked(rank, file):
	var selected_square: Vector2i = Vector2i(rank, file)
	if selected_square == default_square or selected_square == prev_selected_square:
		prev_selected_square = selected_square
	
	else:
		piece_grid.move_piece(prev_selected_square, selected_square)
	
		
