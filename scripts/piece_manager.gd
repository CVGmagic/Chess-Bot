extends Node


var piece_map: Dictionary[int, String] = {
	1: "Pawn White", 2: "Knight White", 3: "Bishop White", 4: "Rook White", 5: "Queen White", 6: "King White", # White pieces
	7: "Pawn Black", 8: "Knight Black", 9: "Bishop Black",  10: "Rook Black", 11: "Queen Black", 12: "King Black" # Black pieces
}

@onready var piece_grid: GridContainer = %PieceGrid

var starting_position: Array[Array] = [[10, 8, 9, 11, 12, 9, 8, 10],
									[7, 7, 7, 7, 7, 7, 7, 7],
									[0, 0, 0, 0, 0, 0, 0, 0],
									[0, 0, 0, 0, 0, 0, 0, 0],
									[0, 0, 0, 0, 0, 0, 0, 0],
									[0, 0, 0, 0, 0, 0, 0, 0],
									[1, 1, 1, 1, 1, 1, 1, 1],
									[4, 2, 3, 5, 6, 3, 2, 4]]
									
									
# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	piece_grid.generate_pieces(starting_position, piece_map)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
