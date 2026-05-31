extends Node2D

var white_square_color = Color("#f0d9b5")
var black_square_color = Color("#b58863")

@onready var board_grid = %BoardGrid

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	board_grid.generate_visual_board()


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
