extends Node2D


@onready var piece_manager: Node2D = %PieceManager
@onready var piece_grid: GridContainer = %PieceGrid


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass


func move_to_notation(cur_square: Vector2i, target_square: Vector2i):
	var piece = piece_grid.pieces[cur_square.x][cur_square.y]


func notation_to_move(move_string: String):
	pass
